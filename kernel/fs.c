#include "fs.h"
#include "string.h"
#include "rtc.h"

static file_entry_t files[FS_MAX_FILES];

void fs_init(void) {
    memset(files, 0, sizeof(files));

    // Populate initial files
    const char* readme_content =
        "TinyOS v1.0 - Minimal 32-bit x86 Operating System\n"
        "==================================================\n\n"
        "Features:\n"
        " - Custom 16-bit Bootloader with El Torito ISO boot\n"
        " - 32-bit Protected Mode Kernel with Flat Address Space\n"
        " - VGA 80x25 Color Console & Dual COM1 Serial Mirroring\n"
        " - In-Memory RAM Filesystem (TinyFS) with CRUD API\n"
        " - Interactive Terminal Shell with Rich Command Set\n"
        " - Command History & Arrow Key Navigation\n"
        " - Output Redirection (> and >>)\n"
        " - Programmable Interrupt Controller (PIC) & PIT Timer\n"
        " - CMOS Real-Time Clock (RTC)\n"
        " - Built-in Retro Snake Game & Matrix Rain\n\n"
        "Type 'help' to view all commands.\n";

    const char* help_content =
        "TinyOS Shell Commands Manual:\n"
        "-----------------------------\n"
        "Filesystem Commands:\n"
        "  ls, dir              List files and disk usage\n"
        "  cat <file>           Display content of a file\n"
        "  touch <file>         Create an empty file\n"
        "  write <file> <text>  Write text to file (overwrite)\n"
        "  append <file> <text> Append text to end of file\n"
        "  rm, del <file>       Delete a file\n"
        "  stat <file>          Show detailed file metadata\n\n"
        "System Commands:\n"
        "  clear, cls           Clear screen\n"
        "  echo <text>          Print text (supports > and >> redirection)\n"
        "  color <fg> [bg]      Change console colors\n"
        "  date, time           Display RTC clock time\n"
        "  uptime               Show system uptime\n"
        "  mem, free            Display RAM and filesystem memory\n"
        "  cpu, sysinfo         Display CPU model and system info\n"
        "  calc <n1> <op> <n2>  Integer arithmetic calculator\n"
        "  history              Display recent command history\n"
        "  reboot               Reboot system\n"
        "  shutdown             Power off virtual machine\n"
        "  version              Display TinyOS version\n\n"
        "Games & Demos:\n"
        "  matrix               Digital rain animation\n"
        "  snake                Playable retro snake game\n";

    const char* welcome_content =
        "Welcome to TinyOS!\n"
        "A lightweight, fully bootable 32-bit x86 operating system.\n\n"
        "Quick Start:\n"
        "  - Type 'help' to see available commands\n"
        "  - Type 'ls' to view files in TinyFS\n"
        "  - Type 'cat readme.txt' to read about the OS\n"
        "  - Type 'snake' to play the mini game!\n";

    const char* cfg_content =
        "# TinyOS System Configuration\n"
        "hostname=tinyos\n"
        "version=1.0.0\n"
        "arch=x86_32\n"
        "default_color=light_grey\n"
        "shell_prompt=tinyos:/> \n";

    const char* license_content =
        "MIT License\n\n"
        "Copyright (c) 2026 TinyOS Contributors\n\n"
        "Permission is hereby granted, free of charge, to any person obtaining a copy\n"
        "of this software and associated documentation files (the \"Software\"), to deal\n"
        "in the Software without restriction, including without limitation the rights\n"
        "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
        "copies of the Software, and to permit persons to whom the Software is\n"
        "furnished to do so, subject to the following conditions:\n\n"
        "The above copyright notice and this permission notice shall be included in all\n"
        "copies or substantial portions of the Software.\n";

    const char* todo_content =
        "[X] Bootloader and El Torito ISO generation\n"
        "[X] 32-bit Protected mode kernel\n"
        "[X] VGA text driver and serial COM1 driver\n"
        "[X] IDT, PIC, PIT timer and Keyboard drivers\n"
        "[X] Minimal RAM filesystem (TinyFS)\n"
        "[X] Terminal command shell\n"
        "[ ] Have fun exploring TinyOS!\n";

    fs_write("readme.txt", readme_content, strlen(readme_content));
    fs_write("help.txt", help_content, strlen(help_content));
    fs_write("welcome.txt", welcome_content, strlen(welcome_content));
    fs_write("system.cfg", cfg_content, strlen(cfg_content));
    fs_write("license.txt", license_content, strlen(license_content));
    fs_write("todo.txt", todo_content, strlen(todo_content));

    // Mark system files
    file_entry_t* f;
    if ((f = fs_get_file("readme.txt"))) f->flags |= FS_FLAG_READONLY;
    if ((f = fs_get_file("help.txt")))   f->flags |= FS_FLAG_READONLY;
    if ((f = fs_get_file("system.cfg"))) f->flags |= FS_FLAG_SYSTEM;
}

file_entry_t* fs_get_file(const char* name) {
    if (!name || name[0] == '\0') return NULL;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if ((files[i].flags & FS_FLAG_IN_USE) && strcmp(files[i].name, name) == 0) {
            return &files[i];
        }
    }
    return NULL;
}

file_entry_t* fs_get_file_by_index(int idx) {
    if (idx < 0 || idx >= FS_MAX_FILES) return NULL;
    if (!(files[idx].flags & FS_FLAG_IN_USE)) return NULL;
    return &files[idx];
}

int fs_create(const char* name) {
    if (!name || name[0] == '\0') return -1;
    if (strlen(name) >= FS_MAX_NAME) return -2;

    // Check if already exists
    if (fs_get_file(name) != NULL) {
        return -3; // File already exists
    }

    // Find free slot
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!(files[i].flags & FS_FLAG_IN_USE)) {
            memset(&files[i], 0, sizeof(file_entry_t));
            strncpy(files[i].name, name, FS_MAX_NAME - 1);
            files[i].size = 0;
            files[i].flags = FS_FLAG_IN_USE;
            rtc_format(files[i].timestamp);
            return i;
        }
    }
    return -4; // Filesystem full
}

int fs_write(const char* name, const char* data, size_t len) {
    if (!name) return -1;
    if (len > FS_MAX_CONTENT - 1) len = FS_MAX_CONTENT - 1;

    file_entry_t* f = fs_get_file(name);
    if (!f) {
        int idx = fs_create(name);
        if (idx < 0) return idx;
        f = &files[idx];
    } else {
        if (f->flags & FS_FLAG_READONLY) {
            return -5; // Read-only file
        }
    }

    memcpy(f->data, data, len);
    f->data[len] = '\0';
    f->size = len;
    rtc_format(f->timestamp);
    return (int)len;
}

int fs_append(const char* name, const char* data, size_t len) {
    if (!name || !data) return -1;
    file_entry_t* f = fs_get_file(name);
    if (!f) {
        return fs_write(name, data, len);
    }
    if (f->flags & FS_FLAG_READONLY) {
        return -5;
    }

    size_t available = (FS_MAX_CONTENT - 1) - f->size;
    if (len > available) len = available;

    memcpy(f->data + f->size, data, len);
    f->size += len;
    f->data[f->size] = '\0';
    rtc_format(f->timestamp);
    return (int)len;
}

int fs_read(const char* name, char* buf, size_t max_len) {
    file_entry_t* f = fs_get_file(name);
    if (!f) return -1;

    size_t read_bytes = f->size;
    if (read_bytes > max_len - 1) {
        read_bytes = max_len - 1;
    }
    memcpy(buf, f->data, read_bytes);
    buf[read_bytes] = '\0';
    return (int)read_bytes;
}

int fs_delete(const char* name) {
    file_entry_t* f = fs_get_file(name);
    if (!f) return -1;
    if (f->flags & FS_FLAG_READONLY) return -2;

    f->flags = 0;
    f->size = 0;
    f->name[0] = '\0';
    f->data[0] = '\0';
    return 0;
}

int fs_count(void) {
    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].flags & FS_FLAG_IN_USE) {
            count++;
        }
    }
    return count;
}

uint32_t fs_total_bytes(void) {
    return FS_MAX_FILES * FS_MAX_CONTENT;
}

uint32_t fs_used_bytes(void) {
    uint32_t used = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].flags & FS_FLAG_IN_USE) {
            used += files[i].size;
        }
    }
    return used;
}
