#ifndef SETTINGS_H
#define SETTINGS_H

/*  Network Settings  */

#ifndef STASSID
#define STASSID "benji"
#define STAPSK  "benji2021"
#endif

#define QUOTE_HOST       "djxmmx.net"
#define SERVER_HOSTNAME  "bolanxu.pythonanywhere.com"

/*  TFT Settings  */

#define SCREEN_W  160
#define SCREEN_H  128
#define CHAR_W    6
#define CHAR_H    8
#define MAX_LINES (SCREEN_H / CHAR_H)
#define MAX_COLS  (SCREEN_W / CHAR_W)

//#define TFT_WHITE   0xFF
//#define TFT_GREEN   0x1C   // was TFT_GREEN
//#define TFT_RED     0xE0   // was TFT_RED
//#define TFT_YELLOW  0xFC   // was TFT_YELLOW
//#define TFT_ORANGE  0xF4   // was TFT_ORANGE

/*  Terminal Settings  */
#define PROMPT "> "

#endif
