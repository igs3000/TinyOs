#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

// Special key codes
#define KEY_UP    0x80
#define KEY_DOWN  0x81
#define KEY_LEFT  0x82
#define KEY_RIGHT 0x83
#define KEY_ESC   0x1B

void keyboard_init(void);
bool keyboard_has_key(void);
char keyboard_getchar(void);

#endif
