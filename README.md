# TinyOS - Minimal 32-bit x86 Operating System & Bootable ISO

A complete 32-bit x86 operating system (**TinyOS**) has been created, built, and packaged into a bootable ISO image (`tinyos.iso`).

---

## What Was Built

```
TinyOs/
├── boot/
│   └── boot.s          # 16-bit real mode bootloader & 32-bit protected mode handoff
├── kernel/
│   ├── entry.s         # Multiboot header & low-level ISR/IRQ assembly stubs
│   ├── types.h         # Fixed-width integer types and register frame definitions
│   ├── io.h            # Port I/O inline functions (inb, outb, inw, outw, cli, sti)
│   ├── string.h/.c     # String manipulation, memory routines, sprintf, and atoi
│   ├── vga.h/.c        # VGA 80x25 16-color text console & hardware cursor controller
│   ├── serial.h/.c     # 115200-baud COM1 UART driver with ANSI escape codes
│   ├── console.h/.c    # Dual console layer (mirrors output to both VGA and Serial)
│   ├── idt.h/.c        # 256-gate IDT, 8259 PIC remapping, and CPU panic register dumps
│   ├── timer.h/.c      # 8254 PIT timer driver (100 Hz), uptime counter, and sleep delays
│   ├── keyboard.h/.c   # PS/2 keyboard driver, scancode mapping, and circular FIFO buffer
│   ├── rtc.h/.c        # CMOS Real-Time Clock reader (decoding BCD date & time)
│   ├── fs.h/.c         # TinyFS RAM filesystem with Inode table, CRUD API, and default files
│   ├── shell.h/.c      # Interactive terminal shell with history, redirection, and apps
│   └── kernel.c        # Main kernel initialization sequence
├── tools/
│   └── make_iso.py     # Self-contained El Torito ISO 9660 generator
├── linker.ld           # Kernel linker script placing code at 1 MB (0x100000)
├── Makefile            # Build recipes for bootloader, kernel, and ISO image
└── README.md           # Documentation and usage guide
```

---

## Architecture Overview

```
+-------------------------------------------------------------------------+
|                              TinyOS Shell                               |
|   (Interactive CLI, Redirection >, History, Filesystem CRUD, Snake)     |
+-------------------------------------------------------------------------+
|                       TinyFS (RAM Filesystem)                           |
|       (Inode Table, File Allocation, CRUD API, Preloaded Files)         |
+-------------------------------------------------------------------------+
|                              Kernel Core                                |
|   +-----------------------+--------------------+--------------------+   |
|   |   Console Layer       |   IDT & 8259 PIC   |     PIT Timer      |   |
|   | (VGA 80x25 + COM1)    | (Vectors 32-47)    | (100 Hz Scheduler) |   |
|   +-----------------------+--------------------+--------------------+   |
|   |  PS/2 Keyboard Driver |   CMOS Real-Time   |    CPUID & Power   |   |
|   | (Scancodes & Buffer)  |    Clock (RTC)     | (ACPI/APM Shutdown)|   |
|   +-----------------------+--------------------+--------------------+   |
+-------------------------------------------------------------------------+
|                  16-bit Bootloader & El Torito ISO                      |
| (BIOS Floppy Emulation, A20 Enable, GDT, 32-bit Protected Mode Handoff) |
+-------------------------------------------------------------------------+
```

---

## Key System Components

1. **Self-Contained Bootable ISO (`tinyos.iso`)**:
   - Packaged with an **El Torito specification** boot catalog in floppy emulation mode.
   - Built via `tools/make_iso.py` without requiring external dependencies (`xorriso` or `genisoimage`).
   - Compatible with QEMU, Bochs, VirtualBox, VMware, and physical PC hardware.

2. **Custom 16-bit Bootloader (`boot/boot.s`)**:
   - Fits into the standard 512-byte MBR sector.
   - Reads the kernel sectors from disk using BIOS `INT 13h` with LBA-to-CHS conversion.
   - Enables the A20 line (via BIOS `INT 15h` and Fast A20 port `0x92`).
   - Sets up a flat 4GB Global Descriptor Table (GDT) and switches CPU to 32-bit Protected Mode.
   - Relocates the kernel to standard `0x100000` (1MB mark) and transfers execution to C.

3. **Dual Console (VGA & Serial)**:
   - Full VGA text-mode driver (`0xB8000`, 80 columns × 25 rows, 16 colors, hardware cursor manipulation via CRT controller ports `0x3D4`/`0x3D5`).
   - Mirrored to the **COM1 serial port** (`0x3F8`) with ANSI colors, allowing interaction via a graphical window or headlessly in a terminal.

4. **Hardware Drivers**:
   - **IDT & PIC**: 256-gate IDT with CPU exception dumps (0–31) and remapped 8259 PIC (IRQs 0–15 mapped to vectors 32–47).
   - **PIT Timer**: 100 Hz (10ms tick interval) driving `uptime` and `sleep_ms`.
   - **PS/2 Keyboard**: Scancode Set 1 interpreter supporting Shift, Caps Lock, backspace, and arrow keys.
   - **CMOS RTC**: Decodes RTC registers `0x70`/`0x71` for calendar date and time.

5. **In-Memory Filesystem (TinyFS)**:
   - Supports up to 64 files with dynamic content up to 8 KB each.
   - Inode metadata tracks filename, size, flags (`IN_USE`, `READONLY`, `SYSTEM`), and timestamps.
   - Pre-populated with `welcome.txt`, `readme.txt`, `help.txt`, `system.cfg`, `license.txt`, and `todo.txt`.
   - Full CRUD API: `fs_create`, `fs_read`, `fs_write`, `fs_append`, `fs_delete`, and `fs_stat`.

6. **Terminal Shell & Commands**:
   - Interactive prompt `tinyos:/> ` with line editing.
   - **Command History**: Cycle through previous commands using the **Up / Down arrow keys**.
   - **Output Redirection**: UNIX-style `>` (overwrite) and `>>` (append) redirection (e.g. `echo hello > file.txt`).
   - Built-in commands:
     - **Filesystem**: `ls`, `dir`, `cat <file>`, `touch <file>`, `write <file> <text>`, `append <file> <text>`, `rm <file>`, `stat <file>`
     - **System**: `clear`, `cls`, `echo <text>`, `color <fg> [bg]`, `date`, `time`, `uptime`, `mem`, `cpu`, `calc <n1> <op> <n2>`, `history`, `version`, `reboot`, `shutdown`
     - **Games & Demos**: `matrix` (green digital rain), `snake` (playable arcade game using W/A/S/D)

---

## Shell Commands Reference

| Command | Syntax | Description |
|---|---|---|
| `help` | `help` | Display list of all shell commands |
| `ls` / `dir` | `ls` | List files with sizes, types, and timestamps |
| `cat` | `cat <file>` | Print content of a file |
| `touch` | `touch <file>` | Create a new empty file |
| `write` | `write <file> <text...>` | Write text to a file (creates or overwrites) |
| `append` | `append <file> <text...>` | Append text to the end of a file |
| `rm` / `del` | `rm <file>` | Delete a file from the filesystem |
| `stat` | `stat <file>` | Show detailed inode metadata (size, flags, address) |
| `echo` | `echo <text...>` | Print text (supports `>` and `>>` redirection) |
| `clear` / `cls` | `clear` | Clear the terminal screen |
| `color` | `color <fg> [bg]` | Change terminal text and background colors |
| `date` / `time` | `date` | Display CMOS Real-Time Clock date and time |
| `uptime` | `uptime` | Display uptime in `HH:MM:SS` format |
| `mem` / `free` | `mem` | Display memory layout and filesystem capacity |
| `cpu` / `sysinfo` | `cpu` | Display CPUID vendor string and architecture |
| `calc` | `calc <n1> <op> <n2>` | Perform integer arithmetic (`+`, `-`, `*`, `/`, `%`) |
| `history` | `history` | Display recent command history |
| `matrix` | `matrix` | Green digital rain animation (press any key to exit) |
| `snake` | `snake` | Playable retro Snake arcade game (W/A/S/D to move, Q to quit) |
| `version` | `version` | Display TinyOS version banner |
| `reboot` | `reboot` | Soft reset via keyboard controller port `0x64` |
| `shutdown` | `shutdown` | Power off via ACPI/QEMU ports |

---

## How to Run

From the project root directory (`/home/jason/TinyOs`):

### 1. Run in QEMU (Graphical Window)
```bash
make run
```
Launches TinyOS in a standard QEMU window with color VGA display and keyboard controls.

### 2. Run in Terminal (Serial Mirror Mode)
```bash
make run-serial
```
Runs TinyOS directly within your terminal window via the COM1 serial mirror.

### 3. Clean and Rebuild at Any Time
```bash
make clean && make
```

### 4. Running on Other Emulators or Hardware
- **VirtualBox**: Create a new VM (Type: "Other", Version: "Other/Unknown (32-bit)"), attach `tinyos.iso` as a virtual optical CD/DVD drive, and start.
- **VMware Workstation / Player**: Create a custom VM with `tinyos.iso` as the CD-ROM boot image.
- **Real Hardware / USB**: Burn `tinyos.iso` to a CD/DVD or create a bootable USB using Rufus/Ventoy to boot on BIOS-compatible x86 systems.

---

## Verified Test Run

```text
================================================================================
                     TinyOS - Minimal 32-bit x86 OS                             
================================================================================
[ OK ] Initializing Hardware Abstraction Layer...
[ OK ] IDT loaded & 8259 PIC remapped (Vectors 32-47)
[ OK ] PIT Timer initialized at 100 Hz
[ OK ] PS/2 Keyboard driver active
[ OK ] Hardware interrupts enabled
[ OK ] TinyFS mounted with default files (6 files loaded)

Welcome to TinyOS!
Type 'help' to list commands, 'ls' to view files, or 'cat readme.txt' for system info.

tinyos:/> ls
  Type     Size   Modified             Name
  ---------------------------------------------------
  [RO   ]    617 B  2026-11-06 07:05:42  readme.txt
  [RO   ]   1132 B  2026-11-06 07:05:42  help.txt
  [FILE ]    259 B  2026-11-06 07:05:42  welcome.txt
  [SYS  ]    121 B  2026-11-06 07:05:42  system.cfg
  [FILE ]    614 B  2026-11-06 07:05:42  license.txt
  [FILE ]    259 B  2026-11-06 07:05:42  todo.txt
  ---------------------------------------------------
  Total: 6 file(s), 3002 bytes used, 521286 bytes free

tinyos:/> echo "Hello TinyOS!" > note.txt
tinyos:/> cat note.txt
"Hello TinyOS!"
tinyos:/> calc 42 * 8
42 * 8 = 336
tinyos:/> date
Current RTC Time: 2026-11-06 07:05:44 UTC
```
