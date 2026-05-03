#ifndef TERMINAL_H
#define TERMINAL_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "settings.h"

class Terminal
{
  private:
    char** _lines;
    uint16_t* _lineColor;

    int _lineCount;
    int _curCol;
    uint16_t _textColor;

    int _maxWidth;
    int _maxHeight;
    int _maxLines;
    int _maxChar;

    int _posx;
    int _posy;

    bool _autoRedraw = true;

    bool _cursorVisible = false;
    bool _cursorEnabled = false;

    void allocBuffers();
    void freeBuffers();
    void scrollUp();

  public:
    Terminal(int posx, int posy, int maxWidth, int maxHeight);
    ~Terminal();

    void setAutoRedraw(bool enabled) { _autoRedraw = enabled; }

    void redraw();

    void setCursorEnabled(bool enabled);
    void tickCursor();

    void reinit(int posx, int posy, int maxWidth, int maxHeight, bool cursor_en);

    void printChar(char c);

    void print(char c);
    void print(const String& text);
    void print(const char* text);

    void print(const String& text, uint16_t color);
    void print(const char* text, uint16_t color);

    void println();
    void println(const String& text);
    void println(const char* text);
    void println(const String& text, uint16_t color);
    void println(const char* text, uint16_t color);

    void clear();

    void setTextColor(uint16_t color) { _textColor = color; }
};

#endif
