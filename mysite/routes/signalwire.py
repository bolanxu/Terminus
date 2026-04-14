"""
SignalWire SMS bridge.

Inbound webhook:  POST /sms/inbound
  - Called by SignalWire when someone texts your SignalWire number.
  - If the number has a named contact -> store message + forward to ESP8266.
  - If unknown number -> create nameless contact, store message, show in
    admin only (NOT forwarded to ESP8266 until admin adds a name).

Outbound helper:  send_sms(to, body)
  - Called by arduino.py when the ESP8266 posts a message and the
    target user has a phone number on their account.

Admin contact API:
  GET  /sms/contacts           - list all contacts (admin only)
  POST /sms/save_contact       - add name to a contact (admin only)
  POST /sms/delete_contact     - remove a contact (admin only)
"""

import datetime
import os
import requests
from flask import Blueprint, request, jsonify, current_app
from flask_login import login_required, current_user

from models import get_db_connection

signalwire_bp = Blueprint('signalwire', __name__, url_prefix='/sms')


# ── helpers ───────────────────────────────────────────────────────────────────

def _db():
    return get_db_connection(current_app.config['DB_PATH'])


def _sw_cfg():
    return (
        current_app.config.get('SW_SPACE_URL',   ''),
        current_app.config.get('SW_PROJECT_ID',  ''),
        current_app.config.get('SW_AUTH_TOKEN',  ''),
        current_app.config.get('SW_FROM_NUMBER', ''),
    )


def send_sms(to_number: str, body: str) -> bool:
    """Send an SMS via SignalWire. Returns True on success."""
    space_url, project_id, auth_token, from_number = _sw_cfg()
    if not all([space_url, project_id, auth_token, from_number]):
        current_app.logger.warning('SignalWire not configured — skipping SMS send')
        return False
    url = f'https://{space_url}/api/laml/2010-04-01/Accounts/{project_id}/Messages.json'
    try:
        resp = requests.post(
            url,
            auth=(project_id, auth_token),
            data={'From': from_number, 'To': to_number, 'Body': body},
            timeout=10,
        )
        if resp.status_code in (200, 201):
            return True
        current_app.logger.error('SignalWire send failed: %s %s', resp.status_code, resp.text)
        return False
    except requests.RequestException as exc:
        current_app.logger.error('SignalWire request error: %s', exc)
        return False


def _get_admin_user_id(conn):
    """Return the id of the first admin user."""
    row = conn.execute(
        "SELECT id FROM users WHERE is_admin = 1 ORDER BY id ASC LIMIT 1"
    ).fetchone()
    return row['id'] if row else 1


def _forward_to_esp(conn, username: str, sender_label: str, body: str):
    """
    Store a message as sender='SMS' so the ESP8266 will pick it up via
    /get_for_arduino. We piggyback on the existing unread queue.
    The content is prefixed with the sender name so the ESP knows who it's from.
    """
    admin_id = _get_admin_user_id(conn)
    conn.execute(
        '''INSERT INTO messages (user_id, sender, content, timestamp)
           VALUES (?, 'SMS', ?, ?)''',
        (admin_id, f'[{sender_label}]: {body}', datetime.datetime.now().isoformat())
    )


# ── routes ────────────────────────────────────────────────────────────────────

@signalwire_bp.route('/inbound', methods=['POST'])
def inbound():
    """
    SignalWire webhook. Set this URL in your SignalWire phone number settings.
    POST fields: From, To, Body  (standard Twilio-compatible LaML)
    """
    from_number = request.form.get('From', '').strip()
    body        = request.form.get('Body', '').strip()

    if not from_number or not body:
        return ('', 204)

    from_number = str(from_number).strip()

    print("THE NUMBER IS", from_number)


    with _db() as conn:
        admin_id = _get_admin_user_id(conn)

        # Look up contact
        contact = conn.execute(
            'SELECT username FROM users WHERE phone = ?', (from_number,)
        ).fetchone()

        print("CONTACT NAME:",contact['username']);

        if contact is None:
            # Totally unknown number — create nameless contact, store for admin
            conn.execute(
                'INSERT OR IGNORE INTO contacts (phone, name) VALUES (?, NULL)',
                (from_number,)
            )
            # Store message under admin but do NOT forward to ESP8266 yet
            conn.execute(
                '''INSERT INTO messages
                   (user_id, sender, content, timestamp, from_number)
                   VALUES (?, 'SMS', ?, ?, ?)''',
                (admin_id, body, datetime.datetime.now().isoformat(), from_number)
            )
            conn.commit()

        elif contact['username'] is None:
            # Known number but still nameless — store for admin, still no ESP forward
            conn.execute(
                '''INSERT INTO messages
                   (user_id, sender, content, timestamp, from_number)
                   VALUES (?, 'SMS', ?, ?, ?)''',
                (admin_id, body, datetime.datetime.now().isoformat(), from_number)
            )
            conn.commit()

        else:
            # Named contact — store AND forward to ESP8266

            user = conn.execute(
                "SELECT id FROM users WHERE username = ?", (str(contact['username']).lower(),)
            ).fetchone()

            conn.execute(
                '''INSERT INTO messages
                   (user_id, sender, content, timestamp, from_number)
                   VALUES (?, 'SMS', ?, ?, ?)''',
                (user['id'], body, datetime.datetime.now().isoformat(), from_number)
            )

            conn.commit()

    return (
        '<?xml version="1.0" encoding="UTF-8"?><Response></Response>',
        200,
        {'Content-Type': 'text/xml'},
    )


# ── contact management (admin only) ──────────────────────────────────────────

def _admin_required():
    from flask_login import current_user
    if not current_user.is_authenticated or not current_user.is_admin:
        return jsonify(status='error', message='Admin required'), 403
    return None


@signalwire_bp.route('/contacts', methods=['GET'])
@login_required
def list_contacts():
    err = _admin_required()
    if err:
        return err
    with _db() as conn:
        rows = conn.execute(
            'SELECT phone, name FROM contacts ORDER BY name IS NULL DESC, name ASC'
        ).fetchall()
    return jsonify([{'phone': r['phone'], 'name': r['name']} for r in rows])


@signalwire_bp.route('/save_contact', methods=['POST'])
@login_required
def save_contact():
    err = _admin_required()
    if err:
        return err
    phone = request.form.get('phone', '').strip()
    name  = request.form.get('name',  '').strip()
    if not phone or not name:
        return jsonify(status='error', message='phone and name required'), 400

    with _db() as conn:
        conn.execute(
            '''INSERT INTO contacts (phone, name) VALUES (?, ?)
               ON CONFLICT(phone) DO UPDATE SET name=excluded.name''',
            (phone, name)
        )
        conn.commit()
    return jsonify(status='ok')


@signalwire_bp.route('/delete_contact', methods=['POST'])
@login_required
def delete_contact():
    err = _admin_required()
    if err:
        return err
    phone = request.form.get('phone', '').strip()
    if not phone:
        return jsonify(status='error', message='phone required'), 400
    with _db() as conn:
        conn.execute('DELETE FROM contacts WHERE phone = ?', (phone,))
        conn.commit()
    return jsonify(status='ok')
