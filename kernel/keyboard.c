#include "keyboard.h"
#include "idt.h"
#include "io.h"

#define KBD_BUFFER_SIZE 256

static volatile char kbd_buffer[KBD_BUFFER_SIZE];
static volatile int kbd_head = 0;
static volatile int kbd_tail = 0;

static bool shift_pressed = false;
static bool caps_lock = false;
static bool extended_mode = false;

// Standard US QWERTY Scancode Set 1 (unmodified)
static const char kbd_map[128] = {
    0,   27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   /* Ctrl */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   /* Left Shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0,   /* Right Shift */
    '*',
    0,   /* Alt */
    ' ', /* Space */
    0,   /* Caps Lock */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* F1 - F10 */
    0,   /* Num Lock */
    0,   /* Scroll Lock */
    0,   /* Home */
    0,   /* Up Arrow */
    0,   /* Page Up */
    '-',
    0,   /* Left Arrow */
    0,
    0,   /* Right Arrow */
    '+',
    0,   /* End */
    0,   /* Down Arrow */
    0,   /* Page Down */
    0,   /* Insert */
    0,   /* Delete */
    0, 0, 0,
    0,   /* F11 */
    0,   /* F12 */
    0
};

// Shifted US QWERTY Scancode Set 1
static const char kbd_shift_map[128] = {
    0,   27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,   /* Ctrl */
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,   /* Left Shift */
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0,   /* Right Shift */
    '*',
    0,   /* Alt */
    ' ', /* Space */
    0,   /* Caps Lock */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* F1 - F10 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

static void buffer_put(char c) {
    int next = (kbd_head + 1) % KBD_BUFFER_SIZE;
    if (next != kbd_tail) {
        kbd_buffer[kbd_head] = c;
        kbd_head = next;
    }
}

static void process_scancode(uint8_t scancode) {
    if (scancode == 0xE0) {
        extended_mode = true;
        return;
    }

    if (extended_mode) {
        extended_mode = false;
        // Check for arrow keys
        if (scancode == 0x48) {
            buffer_put(KEY_UP);
            return;
        } else if (scancode == 0x50) {
            buffer_put(KEY_DOWN);
            return;
        } else if (scancode == 0x4B) {
            buffer_put(KEY_LEFT);
            return;
        } else if (scancode == 0x4D) {
            buffer_put(KEY_RIGHT);
            return;
        }
        return;
    }

    // Key release (break code: bit 7 set)
    if (scancode & 0x80) {
        uint8_t released = scancode & 0x7F;
        if (released == 0x2A || released == 0x36) {
            shift_pressed = false;
        }
        return;
    }

    // Key press (make code)
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = true;
        return;
    }

    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
        return;
    }

    if (scancode < 128) {
        char ch = 0;
        bool use_shift = shift_pressed;

        char normal_ch = kbd_map[scancode];
        if (normal_ch >= 'a' && normal_ch <= 'z') {
            if (caps_lock) use_shift = !use_shift;
        }

        if (use_shift) {
            ch = kbd_shift_map[scancode];
        } else {
            ch = kbd_map[scancode];
        }

        if (ch != 0) {
            buffer_put(ch);
        }
    }
}

static void keyboard_interrupt_handler(registers_t* regs) {
    (void)regs;
    while (inb(0x64) & 1) {
        uint8_t scancode = inb(0x60);
        process_scancode(scancode);
    }
}

void keyboard_init(void) {
    register_interrupt_handler(33, keyboard_interrupt_handler);

    // Flush any pending data in keyboard buffer
    while (inb(0x64) & 1) {
        inb(0x60);
    }
}

bool keyboard_has_key(void) {
    return kbd_head != kbd_tail;
}

char keyboard_getchar(void) {
    while (!keyboard_has_key()) {
        hlt();
    }
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    return c;
}
