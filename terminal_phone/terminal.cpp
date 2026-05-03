#include "terminal.h"
#include "globals.h"

void Terminal::allocBuffers()
{
  _lines = new char*[_maxLines];
  _lineColor = new uint16_t[_maxLines];

  for (int i = 0; i < _maxLines; i++)
  {
    _lines[i] = new char[_maxChar + 1];
    _lines[i][0] = '\0';
    _lineColor[i] = TFT_WHITE;
  }
}

void Terminal::freeBuffers()
{
  if (_lines)
  {
    for (int i = 0; i < _maxLines; i++)
      delete[] _lines[i];
    delete[] _lines;
    _lines = nullptr;
  }

  if (_lineColor)
  {
    delete[] _lineColor;
    _lineColor = nullptr;
  }
}

Terminal::Terminal(int posx, int posy, int maxWidth, int maxHeight)
: _lines(nullptr), _lineColor(nullptr),
  _lineCount(1), _curCol(0), _textColor(TFT_WHITE)
{
  _posx = posx;
  _posy = posy;
  _maxWidth = maxWidth;
  _maxHeight = maxHeight;

  _maxLines = _maxHeight / CHAR_H;
  _maxChar  = _maxWidth  / CHAR_W;

  allocBuffers();
  redraw();
}

Terminal::~Terminal()
{
  freeBuffers();
}

void Terminal::reinit(int posx, int posy, int maxWidth, int maxHeight, bool cursor_en)
{
  // erase old region before changing dimensions
  tft.fillRect(_posx, _posy, _maxWidth, _maxHeight, TFT_BLACK);

  freeBuffers();

  _posx = posx;
  _posy = posy;
  _maxWidth = maxWidth;
  _maxHeight = maxHeight;

  _maxLines = _maxHeight / CHAR_H;
  _maxChar  = _maxWidth  / CHAR_W;

  _lineCount = 1;
  _curCol = 0;
  _textColor = TFT_WHITE;

  allocBuffers();

  setCursorEnabled(cursor_en);
  // setCursorEnabled calls redraw() internally
}

void Terminal::redraw()
{
  if (!_autoRedraw) return;

  // clear region
  tft.fillRect(_posx, _posy, _maxWidth, _maxHeight, TFT_BLACK);

  tft.setTextSize(1);
  tft.setTextWrap(false);

  for (int i = 0; i < _lineCount; i++)
  {
    tft.setCursor(_posx, _posy + i * CHAR_H);
    tft.setTextColor(_lineColor[i]);
    tft.print(_lines[i]);
  }

  if (_cursorEnabled && _cursorVisible)
  {
    int col = min(_curCol, _maxChar - 1);
    int cursorX = _posx + col * CHAR_W;
    int cursorY = _posy + (_lineCount - 1) * CHAR_H;
    tft.fillRect(cursorX, cursorY, CHAR_W, CHAR_H, TFT_BLACK);
    char ch = _lines[_lineCount - 1][col];
    if (ch != '\0')
    {
      tft.setCursor(cursorX, cursorY);
      tft.setTextColor(_lineColor[_lineCount - 1]);
      tft.setTextSize(1);
      tft.setTextWrap(false);
      tft.print(ch);
    }
    _cursorVisible = false;
  }
}

void Terminal::setCursorEnabled(bool enabled)
{
  _cursorEnabled = enabled;
  _cursorVisible = false;
  redraw();
}

void Terminal::tickCursor()
{
  if (!_cursorEnabled) return;

  _cursorVisible = !_cursorVisible;

  // cursor draws AT _curCol (the next write position)
  int cursorX = _posx + _curCol * CHAR_W;
  int cursorY = _posy + (_lineCount - 1) * CHAR_H;

  if (_cursorVisible)
  {
    tft.fillRect(cursorX, cursorY, CHAR_W, CHAR_H, TFT_WHITE);
  }
  else
  {
    tft.fillRect(cursorX, cursorY, CHAR_W, CHAR_H, TFT_BLACK);

    // restore character AT _curCol if one exists
    char ch = _lines[_lineCount - 1][_curCol];
    if (ch != '\0')
    {
      tft.setCursor(cursorX, cursorY);
      tft.setTextColor(_lineColor[_lineCount - 1]);
      tft.setTextSize(1);
      tft.setTextWrap(false);
      tft.print(ch);
    }
  }
}

void Terminal::scrollUp()
{
  for (int i = 0; i < _maxLines - 1; i++)
  {
    strncpy(_lines[i], _lines[i + 1], _maxChar);
    _lines[i][_maxChar] = '\0';
    _lineColor[i] = _lineColor[i + 1];
  }

  _lines[_maxLines - 1][0] = '\0';
  _lineColor[_maxLines - 1] = TFT_WHITE;
}

void Terminal::printChar(char c)
{
  if (_cursorEnabled && _cursorVisible)
  {
    int cursorX = _posx + _curCol * CHAR_W;
    int cursorY = _posy + (_lineCount - 1) * CHAR_H;
    tft.fillRect(cursorX, cursorY, CHAR_W, CHAR_H, TFT_BLACK);
    _cursorVisible = false;
  }
  if (c == '\n' || c == '\r')
  {
    if (_lineCount < _maxLines) _lineCount++;
    else scrollUp();

    _curCol = 0;

    redraw();
    return;
  }

  else if (c == '\b' || c == 127)
  {
    if (_curCol > 0)
    {
      _curCol--;
      _lines[_lineCount - 1][_curCol] = '\0';
      tft.fillRect(
        _posx + _curCol * CHAR_W,
        _posy + (_lineCount - 1) * CHAR_H,
        CHAR_W, CHAR_H,
        TFT_BLACK
      );
    }
    else if (_lineCount > 1)
    {
      // erase current empty line
      _lines[_lineCount - 1][0] = '\0';
      _lineCount--;
      // move to end of previous line
      _curCol = strlen(_lines[_lineCount - 1]);
      // erase last char of previous line
      _curCol--;
      _lines[_lineCount - 1][_curCol] = '\0';
      redraw();
    }
    return;
  }

  if (_curCol >= _maxChar)
  {
    if (_lineCount < _maxLines) _lineCount++;
    else scrollUp();

    _curCol = 0;
    _lines[_lineCount - 1][0] = '\0';
    _lineColor[_lineCount - 1] = _textColor;

  }

  if (_curCol == 0)
    _lineColor[_lineCount - 1] = _textColor;

  _lines[_lineCount - 1][_curCol++] = c;
  _lines[_lineCount - 1][_curCol] = '\0';

  // draw just the new character directly
  if (_autoRedraw)
  {
    tft.setCursor(
      _posx + (_curCol - 1) * CHAR_W,
      _posy + (_lineCount - 1) * CHAR_H
    );
    tft.setTextColor(_textColor);
    tft.setTextSize(1);
    tft.setTextWrap(false);
    tft.print(c);
  }
}

void Terminal::print(char c)
{
  printChar(c);
}

void Terminal::print(const String& text)
{
  for (unsigned int i = 0; i < text.length(); i++)
    printChar(text.charAt(i));
}

void Terminal::print(const char* text)
{
  while (*text) printChar(*text++);
}

void Terminal::print(const String& text, uint16_t color)
{
  uint16_t prev = _textColor;
  _textColor = color;
  print(text);
  _textColor = prev;
}

void Terminal::print(const char* text, uint16_t color)
{
  uint16_t prev = _textColor;
  _textColor = color;
  print(text);
  _textColor = prev;
}

void Terminal::println()
{
  printChar('\n');
}

void Terminal::println(const String& text)
{
  print(text);
  printChar('\n');
}

void Terminal::println(const char* text)
{
  print(text);
  printChar('\n');
}

void Terminal::println(const String& text, uint16_t color)
{
  print(text, color);
  printChar('\n');
}

void Terminal::println(const char* text, uint16_t color)
{
  print(text, color);
  printChar('\n');
}

void Terminal::clear()
{
  for (int i = 0; i < _maxLines; i++)
  {
    _lines[i][0] = '\0';
    _lineColor[i] = TFT_WHITE;
  }

  _lineCount = 1;
  _curCol = 0;
  redraw();
}
