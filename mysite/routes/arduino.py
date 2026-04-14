import datetime
from flask import Blueprint, request, make_response, current_app

from models import get_db_connection

arduino_bp = Blueprint('arduino', __name__)


def _db():
    return get_db_connection(current_app.config['DB_PATH'])


@arduino_bp.route('/<username>/post_from_arduino', methods=['POST'])
def post_from_arduino(username):
    """
    ESP8266 posts a message here.
    Stores it as sender='MODEM', then auto-sends SMS via SignalWire
    if the target user has a phone number on their account.
    """
    msg = request.get_data(as_text=True).strip()

    with _db() as conn:
        user = conn.execute(
            "SELECT id, phone FROM users WHERE username = ?", (username.lower(),)
        ).fetchone()

        if user and msg:
            conn.execute(
                "INSERT INTO messages (user_id, sender, content, timestamp) VALUES (?, ?, ?, ?)",
                (user['id'], 'MODEM', msg, datetime.datetime.now().isoformat())
            )
            conn.commit()

            # Auto-forward to the user's real phone if they have one
            if user['phone']:
                from routes.signalwire import send_sms
                send_sms(user['phone'], msg)

            resp = make_response("OK", 200)
            resp.headers['Content-Type'] = 'text/plain'
            resp.headers['Connection'] = 'close'
            return resp

    return make_response("FAIL", 400)


@arduino_bp.route('/get_for_arduino')
def get_for_arduino():
    """
    ESP8266 polls here for pending messages.
    Returns WEB messages (typed on the website) AND SMS_QUEUE messages
    (inbound SMS from named contacts, pre-formatted as [NAME]: body).
    """
    with _db() as conn:
        c = conn.cursor()
        c.execute("""
            SELECT m.content, m.id, u.username
            FROM messages m
            JOIN users u ON m.user_id = u.id
            WHERE m.sender IN ('WEB', 'SMS_QUEUE', 'SMS') AND m.read = 0
            ORDER BY m.id ASC
        """)
        rows = c.fetchall()

        if rows:
            output_list, ids_to_mark = [], []
            for row in rows:
                # SMS_QUEUE messages are already formatted as [NAME]: body
                # WEB messages need the username prefix
                if row['content'].startswith('['):
                    output_list.append(row['content'])
                else:
                    output_list.append(f"[{row['username'].upper()}]: {row['content']}")
                ids_to_mark.append(row['id'])

            placeholders = ','.join(['?'] * len(ids_to_mark))
            c.execute(f"UPDATE messages SET read=1 WHERE id IN ({placeholders})", ids_to_mark)
            conn.commit()
            return "\n".join(output_list)

    return "NO_MSG"


@arduino_bp.route('/get_phone/<username>')
def get_phone(username):
    with _db() as conn:
        user = conn.execute(
            "SELECT phone FROM users WHERE username = ?", (username.lower(),)
        ).fetchone()

    if user and user['phone']:
        resp = make_response(user['phone'], 200)
        resp.headers['Content-Type'] = 'text/plain'
        resp.headers['Connection'] = 'close'
        return resp

    return make_response("NOT_FOUND", 404)


@arduino_bp.route('/get_contacts')
def get_contacts():
    with _db() as conn:
        c = conn.cursor()
        c.execute("SELECT username FROM users ORDER BY username ASC")
        rows = c.fetchall()

        if rows:
            usernames = [row['username'].upper() for row in rows]
            return "\n".join(usernames)

    return ""
