#include "shell.h"
#include "console.h"
#include "fs.h"
#include "string.h"
#include "timer.h"
#include "rtc.h"
#include "io.h"
#include "keyboard.h"

#define SHELL_MAX_LINE    256
#define SHELL_MAX_ARGS    16
#define SHELL_HISTORY_MAX 10
#define REDIR_BUFFER_SIZE 4096

static char history[SHELL_HISTORY_MAX][SHELL_MAX_LINE];
static int history_count = 0;
static int history_index = 0;

// Output redirection buffer
static bool redir_active = false;
static bool redir_append = false;
static char redir_file[FS_MAX_NAME];
static char redir_buffer[REDIR_BUFFER_SIZE];
static size_t redir_pos = 0;

static void shell_out(const char* s) {
    if (redir_active) {
        size_t len = strlen(s);
        if (redir_pos + len < REDIR_BUFFER_SIZE - 1) {
            memcpy(redir_buffer + redir_pos, s, len);
            redir_pos += len;
            redir_buffer[redir_pos] = '\0';
        }
    } else {
        console_puts(s);
    }
}

static int shell_printf(const char* fmt, ...) {
    char buf[1024];
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    int ret = vsprintf(buf, fmt, args);
    __builtin_va_end(args);
    shell_out(buf);
    return ret;
}

static uint8_t parse_color(const char* name) {
    if (strcmp(name, "black") == 0)       return VGA_COLOR_BLACK;
    if (strcmp(name, "blue") == 0)        return VGA_COLOR_BLUE;
    if (strcmp(name, "green") == 0)       return VGA_COLOR_GREEN;
    if (strcmp(name, "cyan") == 0)        return VGA_COLOR_CYAN;
    if (strcmp(name, "red") == 0)         return VGA_COLOR_RED;
    if (strcmp(name, "magenta") == 0)     return VGA_COLOR_MAGENTA;
    if (strcmp(name, "brown") == 0)       return VGA_COLOR_BROWN;
    if (strcmp(name, "grey") == 0 || strcmp(name, "gray") == 0) return VGA_COLOR_LIGHT_GREY;
    if (strcmp(name, "dark_grey") == 0)   return VGA_COLOR_DARK_GREY;
    if (strcmp(name, "light_blue") == 0)  return VGA_COLOR_LIGHT_BLUE;
    if (strcmp(name, "light_green") == 0) return VGA_COLOR_LIGHT_GREEN;
    if (strcmp(name, "light_cyan") == 0)  return VGA_COLOR_LIGHT_CYAN;
    if (strcmp(name, "light_red") == 0)   return VGA_COLOR_LIGHT_RED;
    if (strcmp(name, "yellow") == 0)      return VGA_COLOR_YELLOW;
    if (strcmp(name, "white") == 0)       return VGA_COLOR_WHITE;
    return 0xFF; // Invalid
}

// Pseudo-random number generator
static uint32_t prng_state = 123456789;
static uint32_t rand(void) {
    prng_state = (prng_state * 1103515245 + 12345) & 0x7FFFFFFF;
    return prng_state;
}

static void cmd_help(int argc, char* argv[]) {
    (void)argc; (void)argv;
    shell_printf("\nAvailable TinyOS Commands:\n");
    shell_printf("==========================\n");
    shell_printf("Filesystem:\n");
    shell_printf("  ls, dir              List files with sizes and timestamps\n");
    shell_printf("  cat <file>           Display file contents\n");
    shell_printf("  touch <file>         Create an empty file\n");
    shell_printf("  write <file> <text>  Write text to file (overwrite)\n");
    shell_printf("  append <file> <text> Append text to end of file\n");
    shell_printf("  rm, del <file>       Delete a file\n");
    shell_printf("  stat <file>          Show detailed file attributes\n");
    shell_printf("\nSystem:\n");
    shell_printf("  clear, cls           Clear console screen\n");
    shell_printf("  echo <text>          Print text (supports > and >> to file)\n");
    shell_printf("  color <fg> [bg]      Change terminal text/background colors\n");
    shell_printf("  date, time           Show real-time clock\n");
    shell_printf("  uptime               Show system uptime\n");
    shell_printf("  mem, free            Show memory statistics\n");
    shell_printf("  cpu, sysinfo         Show CPU information and architecture\n");
    shell_printf("  calc <n1> <op> <n2>  Calculate integer arithmetic (+, -, *, /, %%)\n");
    shell_printf("  history              Show recent command history\n");
    shell_printf("  version              Show OS version banner\n");
    shell_printf("  reboot               Reboot the machine\n");
    shell_printf("  shutdown             Power off machine\n");
    shell_printf("\nGames & Demos:\n");
    shell_printf("  matrix               Matrix digital rain screen\n");
    shell_printf("  snake                Playable retro snake arcade game\n\n");
}

static void cmd_ls(int argc, char* argv[]) {
    (void)argc; (void)argv;
    shell_printf("\n  Type     Size   Modified             Name\n");
    shell_printf("  ---------------------------------------------------\n");

    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        file_entry_t* f = fs_get_file_by_index(i);
        if (f) {
            count++;
            const char* type_str = "FILE ";
            if (f->flags & FS_FLAG_READONLY) type_str = "RO   ";
            if (f->flags & FS_FLAG_SYSTEM)   type_str = "SYS  ";

            shell_printf("  [%s]  %5u B  %s  %s\n",
                         type_str, f->size, f->timestamp, f->name);
        }
    }
    shell_printf("  ---------------------------------------------------\n");
    shell_printf("  Total: %d file(s), %u bytes used, %u bytes free\n\n",
                 count, fs_used_bytes(), fs_total_bytes() - fs_used_bytes());
}

static void cmd_cat(int argc, char* argv[]) {
    if (argc < 2) {
        shell_printf("Usage: cat <filename>\n");
        return;
    }
    file_entry_t* f = fs_get_file(argv[1]);
    if (!f) {
        shell_printf("cat: %s: No such file\n", argv[1]);
        return;
    }
    shell_printf("%s", f->data);
    if (f->size > 0 && f->data[f->size - 1] != '\n') {
        shell_printf("\n");
    }
}

static void cmd_touch(int argc, char* argv[]) {
    if (argc < 2) {
        shell_printf("Usage: touch <filename>\n");
        return;
    }
    int res = fs_create(argv[1]);
    if (res == -3) {
        // Already exists: update timestamp
        file_entry_t* f = fs_get_file(argv[1]);
        if (f) rtc_format(f->timestamp);
    } else if (res < 0) {
        shell_printf("touch: cannot create file '%s' (error %d)\n", argv[1], res);
    }
}

static void cmd_write(int argc, char* argv[]) {
    if (argc < 3) {
        shell_printf("Usage: write <filename> <text...>\n");
        return;
    }
    // Concatenate all remaining arguments with spaces
    char buffer[FS_MAX_CONTENT];
    buffer[0] = '\0';
    for (int i = 2; i < argc; i++) {
        if (i > 2) strcat(buffer, " ");
        strcat(buffer, argv[i]);
    }
    strcat(buffer, "\n");

    int res = fs_write(argv[1], buffer, strlen(buffer));
    if (res < 0) {
        shell_printf("write: error writing to '%s' (code %d)\n", argv[1], res);
    } else {
        shell_printf("Wrote %d bytes to '%s'\n", res, argv[1]);
    }
}

static void cmd_append(int argc, char* argv[]) {
    if (argc < 3) {
        shell_printf("Usage: append <filename> <text...>\n");
        return;
    }
    char buffer[FS_MAX_CONTENT];
    buffer[0] = '\0';
    for (int i = 2; i < argc; i++) {
        if (i > 2) strcat(buffer, " ");
        strcat(buffer, argv[i]);
    }
    strcat(buffer, "\n");

    int res = fs_append(argv[1], buffer, strlen(buffer));
    if (res < 0) {
        shell_printf("append: error writing to '%s' (code %d)\n", argv[1], res);
    } else {
        shell_printf("Appended %d bytes to '%s'\n", res, argv[1]);
    }
}

static void cmd_rm(int argc, char* argv[]) {
    if (argc < 2) {
        shell_printf("Usage: rm <filename>\n");
        return;
    }
    int res = fs_delete(argv[1]);
    if (res == -1) {
        shell_printf("rm: %s: No such file\n", argv[1]);
    } else if (res == -2) {
        shell_printf("rm: %s: File is read-only\n", argv[1]);
    } else {
        shell_printf("Removed '%s'\n", argv[1]);
    }
}

static void cmd_stat(int argc, char* argv[]) {
    if (argc < 2) {
        shell_printf("Usage: stat <filename>\n");
        return;
    }
    file_entry_t* f = fs_get_file(argv[1]);
    if (!f) {
        shell_printf("stat: %s: No such file\n", argv[1]);
        return;
    }
    shell_printf("  File: %s\n", f->name);
    shell_printf("  Size: %u bytes\n", f->size);
    shell_printf("  Flags: 0x%02x (%s%s%s)\n", f->flags,
                 (f->flags & FS_FLAG_IN_USE) ? "IN_USE " : "",
                 (f->flags & FS_FLAG_READONLY) ? "READONLY " : "",
                 (f->flags & FS_FLAG_SYSTEM) ? "SYSTEM " : "");
    shell_printf("  Modified: %s\n", f->timestamp);
    shell_printf("  Buffer Addr: %p\n", f->data);
}

static void cmd_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (i > 1) shell_printf(" ");
        shell_printf("%s", argv[i]);
    }
    shell_printf("\n");
}

static void cmd_clear(int argc, char* argv[]) {
    (void)argc; (void)argv;
    console_clear();
}

static void cmd_color(int argc, char* argv[]) {
    if (argc < 2) {
        shell_printf("Usage: color <fg> [bg]\n");
        shell_printf("Colors: black, blue, green, cyan, red, magenta, brown, grey, dark_grey,\n");
        shell_printf("        light_blue, light_green, light_cyan, light_red, yellow, white\n");
        return;
    }
    uint8_t fg = parse_color(argv[1]);
    if (fg == 0xFF) {
        shell_printf("Invalid foreground color '%s'\n", argv[1]);
        return;
    }
    uint8_t bg = VGA_COLOR_BLACK;
    if (argc >= 3) {
        bg = parse_color(argv[2]);
        if (bg == 0xFF) {
            shell_printf("Invalid background color '%s'\n", argv[2]);
            return;
        }
    }
    console_set_color(fg, bg);
}

static void cmd_date(int argc, char* argv[]) {
    (void)argc; (void)argv;
    char time_str[32];
    rtc_format(time_str);
    shell_printf("Current RTC Time: %s UTC\n", time_str);
}

static void cmd_uptime(int argc, char* argv[]) {
    (void)argc; (void)argv;
    uint32_t sec = uptime_seconds();
    uint32_t min = sec / 60;
    uint32_t hr  = min / 60;
    shell_printf("Uptime: %02d:%02d:%02d (%u seconds, %u ticks)\n",
                 hr, min % 60, sec % 60, sec, timer_get_ticks());
}

static void cmd_mem(int argc, char* argv[]) {
    (void)argc; (void)argv;
    shell_printf("\nTinyOS Memory Statistics:\n");
    shell_printf("-------------------------\n");
    shell_printf("  Kernel Load Address: 0x100000 (1 MB)\n");
    shell_printf("  Kernel Stack:        0x90000 - 0x98000 (32 KB)\n");
    shell_printf("  VGA Text Buffer:     0xB8000 (4 KB)\n");
    shell_printf("  TinyFS Inode Count:  %d / %d slots\n", fs_count(), FS_MAX_FILES);
    shell_printf("  TinyFS Storage:      %u / %u bytes (%u%% used)\n",
                 fs_used_bytes(), fs_total_bytes(),
                 (fs_used_bytes() * 100) / fs_total_bytes());
    shell_printf("  Free RAM in pool:    ~480 KB conventional + Extended RAM\n\n");
}

static void cmd_cpu(int argc, char* argv[]) {
    (void)argc; (void)argv;
    uint32_t eax, ebx, ecx, edx;
    char vendor[13];

    // CPUID EAX=0: get CPU vendor
    __asm__ volatile ("cpuid"
                      : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                      : "a"(0));

    *(uint32_t*)&vendor[0] = ebx;
    *(uint32_t*)&vendor[4] = edx;
    *(uint32_t*)&vendor[8] = ecx;
    vendor[12] = '\0';

    shell_printf("\nCPU & System Information:\n");
    shell_printf("-------------------------\n");
    shell_printf("  OS:             TinyOS v1.0.0\n");
    shell_printf("  Architecture:   x86 (32-bit Protected Mode)\n");
    shell_printf("  CPU Vendor:     %s\n", vendor);
    shell_printf("  CPUID Max In:   0x%x\n", eax);
    shell_printf("  Boot Media:     El Torito Bootable CD-ROM ISO\n");
    shell_printf("  Filesystem:     TinyFS In-Memory RAM Filesystem\n\n");
}

static void cmd_calc(int argc, char* argv[]) {
    if (argc != 4) {
        shell_printf("Usage: calc <num1> <op> <num2>\n");
        shell_printf("Supported operators: +, -, *, /, %%\n");
        return;
    }
    int n1 = atoi(argv[1]);
    char op = argv[2][0];
    int n2 = atoi(argv[3]);
    int result = 0;

    switch (op) {
        case '+': result = n1 + n2; break;
        case '-': result = n1 - n2; break;
        case '*': result = n1 * n2; break;
        case '/':
            if (n2 == 0) {
                shell_printf("calc: error: division by zero\n");
                return;
            }
            result = n1 / n2;
            break;
        case '%':
            if (n2 == 0) {
                shell_printf("calc: error: division by zero\n");
                return;
            }
            result = n1 % n2;
            break;
        default:
            shell_printf("calc: unknown operator '%c'\n", op);
            return;
    }
    shell_printf("%d %c %d = %d\n", n1, op, n2, result);
}

static void cmd_history(int argc, char* argv[]) {
    (void)argc; (void)argv;
    shell_printf("Command History:\n");
    for (int i = 0; i < history_count; i++) {
        shell_printf("  %2d: %s\n", i + 1, history[i]);
    }
}

static void cmd_version(int argc, char* argv[]) {
    (void)argc; (void)argv;
    shell_printf("\n  _______ _             ____   _____\n");
    shell_printf(" |__   __(_)           / __ \\ / ____|\n");
    shell_printf("    | |   _ _ __  _   | |  | | (___\n");
    shell_printf("    | |  | | '_ \\| | | | |  | |\\___ \\\n");
    shell_printf("    | |  | | | | | |_| | |__| |____) |\n");
    shell_printf("    |_|  |_|_| |_|\\__, |\\____/|_____/\n");
    shell_printf("                   __/ |\n");
    shell_printf("                  |___/\n");
    shell_printf(" TinyOS Version 1.0 (x86-32 Protected Mode)\n");
    shell_printf(" Built for Bootable El Torito ISO\n\n");
}

static void cmd_reboot(int argc, char* argv[]) {
    (void)argc; (void)argv;
    console_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    printf("Rebooting TinyOS...\n");
    sleep_ms(500);

    // Pulse CPU reset line via 8042 keyboard controller
    uint8_t good = 0x02;
    while (good & 0x02) good = inb(0x64);
    outb(0x64, 0xFE);

    // Fallback: Triple fault
    __asm__ volatile ("lidt (%esp)");
    __asm__ volatile ("int3");
    while (1) hlt();
}

static void cmd_shutdown(int argc, char* argv[]) {
    (void)argc; (void)argv;
    console_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    printf("Powering off TinyOS...\n");
    sleep_ms(500);

    // QEMU modern ACPI shutdown
    outw(0x604, 0x2000);
    // Bochs and older QEMU
    outw(0xB004, 0x2000);
    // VirtualBox
    outw(0x4004, 0x3400);

    printf("System halted. It is now safe to close your emulator.\n");
    cli();
    while (1) hlt();
}

// Matrix digital rain effect
static void cmd_matrix(int argc, char* argv[]) {
    (void)argc; (void)argv;
    console_clear();
    console_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);

    uint8_t drops[VGA_WIDTH];
    for (int i = 0; i < VGA_WIDTH; i++) {
        drops[i] = rand() % VGA_HEIGHT;
    }

    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz#$%&*+<>";
    size_t charset_len = strlen(charset);

    while (!console_has_char()) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            if (rand() % 4 == 0) {
                uint8_t y = drops[x];
                char c = charset[rand() % charset_len];
                vga_write_at(c, (VGA_COLOR_BLACK << 4) | VGA_COLOR_WHITE, y, x);

                if (y > 0) {
                    vga_write_at(charset[rand() % charset_len],
                                 (VGA_COLOR_BLACK << 4) | VGA_COLOR_LIGHT_GREEN, y - 1, x);
                }
                if (y > 3) {
                    vga_write_at(charset[rand() % charset_len],
                                 (VGA_COLOR_BLACK << 4) | VGA_COLOR_GREEN, y - 3, x);
                }
                if (y > 6) {
                    vga_write_at(' ', (VGA_COLOR_BLACK << 4) | VGA_COLOR_BLACK, y - 6, x);
                }

                drops[x]++;
                if (drops[x] >= VGA_HEIGHT) {
                    drops[x] = 0;
                }
            }
        }
        sleep_ms(40);
    }

    // Consume exit key
    console_getchar();
    console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    console_clear();
}

// Playable retro Snake Game!
static void cmd_snake(int argc, char* argv[]) {
    (void)argc; (void)argv;
    console_clear();

    const int BOARD_W = 32;
    const int BOARD_H = 16;
    const int OFFSET_X = 24;
    const int OFFSET_Y = 4;

    int snake_x[256];
    int snake_y[256];
    int snake_len = 3;

    int dir_x = 1;
    int dir_y = 0;

    int head_x = BOARD_W / 2;
    int head_y = BOARD_H / 2;

    for (int i = 0; i < snake_len; i++) {
        snake_x[i] = head_x - i;
        snake_y[i] = head_y;
    }

    int food_x = 5;
    int food_y = 5;
    int score = 0;
    bool game_over = false;

    // Draw frame
    console_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    for (int x = 0; x < BOARD_W + 2; x++) {
        vga_write_at('-', (VGA_COLOR_BLACK << 4) | VGA_COLOR_YELLOW, OFFSET_Y - 1, OFFSET_X + x);
        vga_write_at('-', (VGA_COLOR_BLACK << 4) | VGA_COLOR_YELLOW, OFFSET_Y + BOARD_H, OFFSET_X + x);
    }
    for (int y = 0; y < BOARD_H; y++) {
        vga_write_at('|', (VGA_COLOR_BLACK << 4) | VGA_COLOR_YELLOW, OFFSET_Y + y, OFFSET_X);
        vga_write_at('|', (VGA_COLOR_BLACK << 4) | VGA_COLOR_YELLOW, OFFSET_Y + y, OFFSET_X + BOARD_W + 1);
    }

    vga_set_cursor(1, OFFSET_X);
    printf("=== RETRO SNAKE === [Q to Quit]");
    vga_set_cursor(2, OFFSET_X);
    printf("Controls: W/A/S/D or Arrows");

    while (!game_over) {
        // Display score
        vga_set_cursor(OFFSET_Y + BOARD_H + 1, OFFSET_X);
        console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        printf("Score: %d   Length: %d  ", score, snake_len);

        // Check key input
        if (console_has_char()) {
            char key = console_getchar();
            if (key == 'q' || key == 'Q' || key == KEY_ESC) {
                break;
            }
            if ((key == 'w' || key == 'W' || (uint8_t)key == KEY_UP) && dir_y != 1) {
                dir_x = 0; dir_y = -1;
            } else if ((key == 's' || key == 'S' || (uint8_t)key == KEY_DOWN) && dir_y != -1) {
                dir_x = 0; dir_y = 1;
            } else if ((key == 'a' || key == 'A' || (uint8_t)key == KEY_LEFT) && dir_x != 1) {
                dir_x = -1; dir_y = 0;
            } else if ((key == 'd' || key == 'D' || (uint8_t)key == KEY_RIGHT) && dir_x != -1) {
                dir_x = 1; dir_y = 0;
            }
        }

        // Move snake
        int new_head_x = snake_x[0] + dir_x;
        int new_head_y = snake_y[0] + dir_y;

        // Check wall collision
        if (new_head_x < 0 || new_head_x >= BOARD_W ||
            new_head_y < 0 || new_head_y >= BOARD_H) {
            game_over = true;
            break;
        }

        // Check self collision
        for (int i = 0; i < snake_len; i++) {
            if (snake_x[i] == new_head_x && snake_y[i] == new_head_y) {
                game_over = true;
                break;
            }
        }
        if (game_over) break;

        // Erase old tail
        vga_write_at(' ', (VGA_COLOR_BLACK << 4) | VGA_COLOR_BLACK,
                     OFFSET_Y + snake_y[snake_len - 1], OFFSET_X + 1 + snake_x[snake_len - 1]);

        // Shift body
        for (int i = snake_len - 1; i > 0; i--) {
            snake_x[i] = snake_x[i - 1];
            snake_y[i] = snake_y[i - 1];
        }
        snake_x[0] = new_head_x;
        snake_y[0] = new_head_y;

        // Check food collision
        if (new_head_x == food_x && new_head_y == food_y) {
            score += 10;
            if (snake_len < 250) {
                snake_len++;
                snake_x[snake_len - 1] = snake_x[snake_len - 2];
                snake_y[snake_len - 1] = snake_y[snake_len - 2];
            }
            // Spawn new food
            food_x = rand() % BOARD_W;
            food_y = rand() % BOARD_H;
        }

        // Draw food
        vga_write_at('*', (VGA_COLOR_BLACK << 4) | VGA_COLOR_LIGHT_RED,
                     OFFSET_Y + food_y, OFFSET_X + 1 + food_x);

        // Draw snake
        vga_write_at('@', (VGA_COLOR_BLACK << 4) | VGA_COLOR_YELLOW,
                     OFFSET_Y + snake_y[0], OFFSET_X + 1 + snake_x[0]);
        for (int i = 1; i < snake_len; i++) {
            vga_write_at('O', (VGA_COLOR_BLACK << 4) | VGA_COLOR_LIGHT_GREEN,
                         OFFSET_Y + snake_y[i], OFFSET_X + 1 + snake_x[i]);
        }

        sleep_ms(110);
    }

    vga_set_cursor(OFFSET_Y + BOARD_H + 2, OFFSET_X);
    console_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    printf("GAME OVER! Final Score: %d\n", score);
    printf("Press any key to return to terminal...");
    console_getchar();

    console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    console_clear();
}

typedef struct {
    const char* name;
    void (*func)(int argc, char* argv[]);
} command_entry_t;

static const command_entry_t commands[] = {
    {"help",     cmd_help},
    {"ls",       cmd_ls},
    {"dir",      cmd_ls},
    {"cat",      cmd_cat},
    {"touch",    cmd_touch},
    {"write",    cmd_write},
    {"append",   cmd_append},
    {"rm",       cmd_rm},
    {"del",      cmd_rm},
    {"stat",     cmd_stat},
    {"echo",     cmd_echo},
    {"clear",    cmd_clear},
    {"cls",      cmd_clear},
    {"color",    cmd_color},
    {"date",     cmd_date},
    {"time",     cmd_date},
    {"uptime",   cmd_uptime},
    {"mem",      cmd_mem},
    {"free",     cmd_mem},
    {"cpu",      cmd_cpu},
    {"sysinfo",  cmd_cpu},
    {"calc",     cmd_calc},
    {"history",  cmd_history},
    {"version",  cmd_version},
    {"reboot",   cmd_reboot},
    {"shutdown", cmd_shutdown},
    {"matrix",   cmd_matrix},
    {"snake",    cmd_snake},
    {NULL,       NULL}
};

static void execute_command(char* line) {
    // Trim leading whitespace
    while (*line == ' ' || *line == '\t') line++;
    if (*line == '\0') return;

    // Check for redirection '>' or '>>'
    redir_active = false;
    redir_append = false;
    redir_pos = 0;
    redir_file[0] = '\0';
    redir_buffer[0] = '\0';

    char* redir_pos_ptr = strchr(line, '>');
    if (redir_pos_ptr) {
        *redir_pos_ptr = '\0';
        redir_pos_ptr++;
        if (*redir_pos_ptr == '>') {
            redir_append = true;
            redir_pos_ptr++;
        }
        while (*redir_pos_ptr == ' ' || *redir_pos_ptr == '\t') redir_pos_ptr++;
        // Copy target filename
        int fi = 0;
        while (*redir_pos_ptr && *redir_pos_ptr != ' ' && *redir_pos_ptr != '\t' && fi < FS_MAX_NAME - 1) {
            redir_file[fi++] = *redir_pos_ptr++;
        }
        redir_file[fi] = '\0';
        if (redir_file[0] != '\0') {
            redir_active = true;
        }
    }

    // Tokenize arguments
    char* argv[SHELL_MAX_ARGS];
    int argc = 0;
    char* p = line;

    while (*p && argc < SHELL_MAX_ARGS) {
        while (*p == ' ' || *p == '\t') {
            *p++ = '\0';
        }
        if (*p == '\0') break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t') {
            p++;
        }
    }

    if (argc == 0) return;

    // Match command
    bool found = false;
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].func(argc, argv);
            found = true;
            break;
        }
    }

    if (!found) {
        shell_printf("tinyos: command not found: %s. Type 'help' for available commands.\n", argv[0]);
    }

    // Process output redirection if active
    if (redir_active && redir_file[0] != '\0') {
        if (redir_append) {
            fs_append(redir_file, redir_buffer, redir_pos);
        } else {
            fs_write(redir_file, redir_buffer, redir_pos);
        }
        redir_active = false;
    }
}

static void add_history(const char* line) {
    if (line[0] == '\0') return;
    if (history_count > 0 && strcmp(history[history_count - 1], line) == 0) {
        return; // Don't duplicate last command
    }
    if (history_count < SHELL_HISTORY_MAX) {
        strcpy(history[history_count++], line);
    } else {
        for (int i = 0; i < SHELL_HISTORY_MAX - 1; i++) {
            strcpy(history[i], history[i + 1]);
        }
        strcpy(history[SHELL_HISTORY_MAX - 1], line);
    }
    history_index = history_count;
}

void shell_init(void) {
    history_count = 0;
    history_index = 0;
}

void shell_run(void) {
    char line[SHELL_MAX_LINE];
    int pos = 0;

    console_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printf("tinyos:/> ");
    console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    while (1) {
        char c = console_getchar();

        if (c == '\n' || c == '\r') {
            line[pos] = '\0';
            console_putc('\n');

            if (pos > 0) {
                add_history(line);
                execute_command(line);
            }

            pos = 0;
            history_index = history_count;

            console_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
            printf("tinyos:/> ");
            console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        } else if (c == '\b' || c == 0x7F) {
            if (pos > 0) {
                pos--;
                console_backspace();
            }
        } else if ((uint8_t)c == KEY_UP) {
            // History previous
            if (history_count > 0 && history_index > 0) {
                history_index--;
                // Clear current line on screen
                while (pos > 0) {
                    console_backspace();
                    pos--;
                }
                strcpy(line, history[history_index]);
                pos = strlen(line);
                console_puts(line);
            }
        } else if ((uint8_t)c == KEY_DOWN) {
            // History next
            if (history_index < history_count - 1) {
                history_index++;
                while (pos > 0) {
                    console_backspace();
                    pos--;
                }
                strcpy(line, history[history_index]);
                pos = strlen(line);
                console_puts(line);
            } else if (history_index == history_count - 1) {
                history_index++;
                while (pos > 0) {
                    console_backspace();
                    pos--;
                }
                line[0] = '\0';
                pos = 0;
            }
        } else if (c >= ' ' && c <= '~') {
            if (pos < SHELL_MAX_LINE - 1) {
                line[pos++] = c;
                console_putc(c);
            }
        }
    }
}
