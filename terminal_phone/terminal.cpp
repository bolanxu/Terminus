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
: _sprite(&tft), _lines(nullptr), _lineColor(nullptr),
  _lineCount(1), _curCol(0), _textColor(TFT_WHITE)
{
  _posx = posx;
  _posy = posy;
  _maxWidth = maxWidth;
  _maxHeight = maxHeight;

  _maxLines = _maxHeight / CHAR_H;
  _maxChar  = _maxWidth  / CHAR_W;

  allocBuffers();

  _sprite.setColorDepth(8);
  _sprite.createSprite(_maxWidth, _maxHeight);
  _sprite.setTextSize(1);
  _sprite.setTextWrap(false);

  redraw();
}

Terminal::~Terminal()
{
  _sprite.deleteSprite();
  freeBuffers();
}

void Terminal::reinit(int posx, int posy, int maxWidth, int maxHeight)
{
  _sprite.deleteSprite();
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

  _sprite.setColorDepth(8);
  _sprite.createSprite(_maxWidth, _maxHeight);
  _sprite.setTextSize(1);
  _sprite.setTextWrap(false);

  redraw();
}

void Terminal::redraw()
{
  if (_autoRedraw)
  {
    _sprite.fillSprite(TFT_BLACK);
  
    for (int i = 0; i < _lineCount; i++)
    {
      _sprite.setCursor(0, i * CHAR_H);
      _sprite.setTextColor(_lineColor[i]);
      _sprite.print(_lines[i]);
    }
  
    _sprite.pushSprite(_posx, _posy);
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
  if (c == '\n' || c == '\r')
  {
    if (_lineCount < _maxLines) _lineCount++;
    else scrollUp();

    _curCol = 0;
    
    redraw();
    return;
  }

  if (c == '\b' || c == 127)
  {
    if (_curCol > 0)
    {
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

  redraw();
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
