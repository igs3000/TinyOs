# TinyOS — Minimal 32-bit x86 Operating System & Bootable ISO

![Architecture](https://img.shields.io/badge/Architecture-x86--32%20Protected%20Mode-blue.svg)
![Boot Media](https://img.shields.io/badge/Boot%20Media-El%20Torito%20ISO-success.svg)
![Languages](https://img.shields.io/badge/Languages-C%20%7C%20GNU%20Assembly-orange.svg)
![License](https://img.shields.io/badge/License-MIT-lightgrey.svg)

---

## 1. Introduction

**TinyOS** is a lightweight, bare-metal 32-bit x86 operating system engineered entirely from scratch in C and GNU Assembly. It boots directly on PC hardware and emulators without relying on external bootloaders (like GRUB) or third-party operating system utilities (such as `xorriso` or `genisoimage`).

The goal of TinyOS is to provide an accessible yet mathematically and architecturally complete operating system implementation. It illustrates the complete journey of a computer system:
1. **Firmware Bootstrap**: Booting from BIOS media in 16-bit Real Mode.
2. **CPU Mode Transition**: Unlocking the A20 line, configuring Global Descriptor Tables (GDT), and switching to 32-bit Protected Mode.
3. **Core Subsystems**: Handling CPU faults, remapping the 8259 PIC, scheduling via the 8254 PIT timer, and capturing keystrokes via PS/2 interrupt handlers.
4. **Storage Layer**: Implementing an in-memory RAM filesystem (**TinyFS**) with an inode table, file attributes, and CRUD operations.
5. **Human Interface**: Offering a full-featured terminal shell supporting command history, arrow key navigation, output redirection (`>` and `>>`), system monitoring utilities, and a playable arcade game.

TinyOS produces a standard **El Torito bootable ISO** (`tinyos.iso`) that runs out of the box in QEMU, VirtualBox, VMware, or real hardware via a bootable USB drive or CD-ROM.

---

## 2. System Architecture

```
+-----------------------------------------------------------------------------------+
|                                  TinyOS Shell                                     |
|    Command Parser • History (Up/Down) • Output Redirection (> / >>) • Games      |
+-----------------------------------------------------------------------------------+
|                                     TinyFS                                        |
|        In-Memory RAM Filesystem • Inode Table • Permissions • CRUD API            |
+-----------------------------------------------------------------------------------+
|                                  Kernel Core                                      |
|  +--------------------+---------------------+------------------+----------------+  |
|  |   Dual Console     |   IDT & 8259 PIC    |    8254 Timer    | PS/2 Keyboard  |  |
|  | (VGA 80x25 + COM1) |  (Vectors 32–47)    | (100 Hz Ticks)   | (IRQ1 FIFO)    |  |
|  +--------------------+---------------------+------------------+----------------+  |
|  |    CMOS RTC        |   CPU Diagnostics   |  ACPI Shutdown   | Flat Memory    |  |
|  |  (BCD Date/Time)   |   (CPUID Vendor)    |   & Reset Port   | (4 GB Space)   |  |
|  +--------------------+---------------------+------------------+----------------+  |
+-----------------------------------------------------------------------------------+
|                        16-bit Real-Mode Bootloader                                |
|     BIOS INT 13h Sector Loader • Fast A20 Gate • GDT Setup • Protected Mode Handoff |
+-----------------------------------------------------------------------------------+
|                         El Torito ISO 9660 Media                                  |
|            Primary Volume Descriptor • Boot Catalog • Virtual Floppy Image        |
+-----------------------------------------------------------------------------------+
```

---

## 3. Deep-Dive: System Aspects

### A. Bootstrapping & Protected Mode Handoff (`boot/boot.s`)
- **Master Boot Record (MBR)**: Contained entirely within the first 512 bytes with the signature `0xAA55` at offset 510.
- **Disk Reader**: Reads the kernel sectors from the virtual floppy disk across cylinders, heads, and sectors using BIOS `INT 13h, AH=02h` with dynamic LBA-to-CHS conversion.
- **A20 Line Gate**: Enables access to memory addresses above 1 MB using both BIOS `INT 15h, AX=2401h` and the Fast A20 keyboard controller port `0x92`.
- **Global Descriptor Table (GDT)**: Constructs a flat 4 GB memory space with:
  - `0x08`: 32-bit Kernel Code Segment (Base: 0, Limit: 4 GB, Ring 0, Exec/Read)
  - `0x10`: 32-bit Kernel Data Segment (Base: 0, Limit: 4 GB, Ring 0, Read/Write)
- **Transition**: Activates the Protection Enable bit (`CR0 |= 1`), performs a far jump to flush the CPU pipeline, sets up the stack at `0x90000`, relocates the kernel to `0x100000` (1 MB mark), and jumps to C execution.

### B. Hardware Abstraction & Interrupt Handling
- **Interrupt Descriptor Table (IDT)**: Defines a 256-gate table (`kernel/idt.c`). Dedicated ISR stubs catch CPU exceptions 0–31 (Page Faults, General Protection Faults, Divide Errors) and print register dumps instead of silently hanging.
- **8259 PIC Remapping**: Reconfigures the master and slave Programmable Interrupt Controllers to vectors 32–47, preventing hardware interrupts from colliding with CPU exceptions.
- **8254 PIT Timer (`kernel/timer.c`)**: Configured to 100 Hz (10 ms interval). Maintains a monotonic tick counter, computes system uptime, and powers millisecond-accurate `sleep_ms()` routines.
- **PS/2 Keyboard Driver (`kernel/keyboard.c`)**: Pure interrupt-driven driver connected to IRQ1. Translates Scancode Set 1 make/break codes, tracks Shift and Caps Lock states, handles arrow keys, and feeds an asynchronous circular FIFO buffer without polling race conditions.
- **CMOS RTC Clock (`kernel/rtc.c`)**: Reads RTC ports `0x70` and `0x71`, converts BCD values to standard binary, and outputs formatted date and time (`YYYY-MM-DD HH:MM:SS`).

### C. Dual Console System (`kernel/console.c`)
- **VGA Text Mode Console (`kernel/vga.c`)**: Direct memory mapping to `0xB8000` (80 columns × 25 rows). Supports 16 colors, hardware cursor updates via CRT controller ports `0x3D4`/`0x3D5`, backspacing, tab indentation, and screen scrolling.
- **COM1 Serial Mirror (`kernel/serial.c`)**: All console output is mirrored to the 16550 UART serial port `0x3F8` at 115200 baud, 8N1, complete with ANSI color sequences. This allows TinyOS to run in both graphical windows and headless terminal environments.

### D. TinyFS — In-Memory Filesystem (`kernel/fs.c`)
TinyFS is an in-memory filesystem built into the kernel:
- **Capacity**: Manages up to 64 file inodes with up to 8,192 bytes of content per file.
- **Metadata**: Each inode tracks file name, size, modification timestamp, data buffer, and flags (`IN_USE`, `READONLY`, `SYSTEM`).
- **CRUD Operations**: Complete POSIX-style kernel API (`fs_create`, `fs_read`, `fs_write`, `fs_append`, `fs_delete`, `fs_stat`, `fs_list`).
- **Preloaded System Files**:
  - `welcome.txt`: Quick-start overview.
  - `readme.txt`: System overview and architecture details.
  - `help.txt`: Built-in manual of all terminal commands.
  - `system.cfg`: System settings and environment attributes.
  - `license.txt`: MIT Open-Source License.
  - `todo.txt`: Embedded project task list.

### E. Interactive Terminal Shell (`kernel/shell.c`)
- **Command Line Editor**: Prompt `tinyos:/> ` with character buffering and backspacing.
- **Command History**: Preserves previous commands; use **Up Arrow** and **Down Arrow** to cycle through history.
- **UNIX Output Redirection**: Supports writing to files with `>` (truncate/overwrite) and `>>` (append). For example:
  ```text
  tinyos:/> echo "System initialized" > log.txt
  tinyos:/> echo "Second event" >> log.txt
  tinyos:/> cat log.txt
  ```
- **Built-in Applications**:
  - `calc`: Integer arithmetic engine (`+`, `-`, `*`, `/`, `%`).
  - `matrix`: Falling green digital rain animation on the VGA text console.
  - `snake`: Playable retro arcade Snake game (steered with W/A/S/D or arrow keys, Q to quit).
  - `reboot` & `shutdown`: Keyboard controller reset and ACPI/QEMU power-off routines.

### F. Standalone El Torito ISO Generator (`tools/make_iso.py`)
Rather than depending on heavy external disk burning packages, TinyOS includes a self-contained Python builder that writes a standard ISO 9660 filesystem:
- Sectors 0–15: Reserved system area.
- Sector 16: Primary Volume Descriptor (PVD).
- Sector 17: Boot Record Volume Descriptor with the `"EL TORITO SPECIFICATION"` identifier.
- Sector 18: Volume Descriptor Set Terminator.
- Sector 19: Boot Catalog (Validation Entry and Initial Boot Entry for 1.44 MB floppy emulation).
- Sector 20+: The bootloader and kernel disk image.

---

## 4. Shell Commands Reference

| Command | Syntax | Description |
|---|---|---|
| **`help`** | `help` | Displays the categorized list of all shell commands |
| **`ls`** / **`dir`** | `ls` | Lists files with permission flags, sizes, timestamps, and free space |
| **`cat`** | `cat <file>` | Displays the contents of a file |
| **`touch`** | `touch <file>` | Creates a new empty file or updates its timestamp |
| **`write`** | `write <file> <text...>` | Writes text to a file (creates if absent, overwrites if present) |
| **`append`** | `append <file> <text...>` | Appends text to the end of a file |
| **`rm`** / **`del`** | `rm <file>` | Deletes a file from TinyFS |
| **`stat`** | `stat <file>` | Displays inode metadata, memory buffer address, and flags |
| **`echo`** | `echo <text...>` | Prints text (supports `>` and `>>` file redirection) |
| **`clear`** / **`cls`** | `clear` | Clears the terminal screen |
| **`color`** | `color <fg> [bg]` | Changes console text and background colors |
| **`date`** / **`time`** | `date` | Displays real-time clock (RTC) UTC date and time |
| **`uptime`** | `uptime` | Shows total system uptime in `HH:MM:SS` format and ticks |
| **`mem`** / **`free`** | `mem` | Displays memory layout and TinyFS capacity usage |
| **`cpu`** / **`sysinfo`** | `cpu` | Queries CPUID for processor vendor and architecture information |
| **`calc`** | `calc <n1> <op> <n2>` | Evaluates integer arithmetic (`+`, `-`, `*`, `/`, `%`) |
| **`history`** | `history` | Displays recent command history |
| **`matrix`** | `matrix` | Launches green digital rain screen (press any key to exit) |
| **`snake`** | `snake` | Interactive retro Snake game (W/A/S/D to move, Q to quit) |
| **`version`** | `version` | Displays the TinyOS ASCII banner and build info |
| **`reboot`** | `reboot` | Resets the system via keyboard controller port `0x64` |
| **`shutdown`** | `shutdown` | Powers down the system via ACPI/QEMU I/O ports |

---

## 5. Directory Structure

```
TinyOs/
├── boot/
│   └── boot.s          # 16-bit real mode bootloader, A20, GDT, & 32-bit switch
├── kernel/
│   ├── entry.s         # Multiboot header & assembly ISR/IRQ stubs
│   ├── types.h         # Fixed-width integer types and register frame
│   ├── io.h            # In/Out port assembly macros (inb, outb, inw, outw)
│   ├── string.h/.c     # String manipulation, sprintf, atoi, itoa, memory copy
│   ├── vga.h/.c        # VGA 80x25 text driver with scrolling and cursor
│   ├── serial.h/.c     # COM1 UART 115200 baud serial driver
│   ├── console.h/.c    # Unified console abstraction (VGA + Serial mirror)
│   ├── idt.h/.c        # IDT gates setup, PIC remap, and exception dumps
│   ├── timer.h/.c      # 8254 PIT timer driver (100 Hz) and sleep delays
│   ├── keyboard.h/.c   # PS/2 keyboard driver with circular FIFO buffer
│   ├── rtc.h/.c        # CMOS Real-Time Clock reader (BCD converter)
│   ├── fs.h/.c         # TinyFS RAM filesystem implementation
│   ├── shell.h/.c      # Terminal shell, line editor, history, and apps
│   └── kernel.c        # Main kernel initialization sequence
├── tools/
│   └── make_iso.py     # Pure Python El Torito ISO 9660 generator
├── linker.ld           # Linker script locating kernel at 1 MB (0x100000)
├── Makefile            # Build recipes for bootloader, kernel, and ISO image
└── README.md           # Documentation
```

---

## 6. How to Compile and Run

### Prerequisites

To build and run TinyOS on a Linux host (e.g. Debian, Ubuntu, Fedora, Arch):

```bash
# Debian / Ubuntu:
sudo apt-get install build-essential gcc-multilib qemu-system-x86 python3
```

Ensure the following tools are available in your `PATH`:
- `gcc` (with 32-bit compilation support via `-m32`)
- GNU `binutils` (`as`, `ld`, `objcopy`)
- `python3`
- `qemu-system-x86_64` (or `qemu-system-i386`)

---

### Step 1: Compiling the OS and Generating the ISO

Run `make` from the project root:

```bash
make
```

This performs the following steps automatically:
1. Assembles `boot/boot.s` into the 512-byte raw bootsector `boot/boot.bin`.
2. Compiles the freestanding C kernel source files and `kernel/entry.s` into `kernel/kernel.elf`.
3. Extracts the flat binary image `kernel/kernel.bin`.
4. Executes `tools/make_iso.py` to package the bootloader and kernel into the bootable CD-ROM image **`tinyos.iso`**.

To clean all build artifacts and perform a fresh build:
```bash
make clean && make
```

---

### Step 2: Running the Operating System

#### Option A: In a Graphical Window (VGA Console)
```bash
make run
```
*Or directly via QEMU:*
```bash
qemu-system-x86_64 -cdrom tinyos.iso -boot d
```
This opens the standard QEMU window with color VGA text display and full PS/2 keyboard support.

#### Option B: Directly in your Host Terminal (Serial Mirror)
```bash
make run-serial
```
*Or directly via QEMU:*
```bash
qemu-system-x86_64 -cdrom tinyos.iso -boot d -nographic
```
This runs TinyOS inside your active terminal using the COM1 serial interface.
> **Note**: To exit QEMU in `-nographic` mode, press `Ctrl+A` then `X`, or simply type `shutdown` in TinyOS.

---

### Step 3: Running on Other Virtual Machines or Real Hardware

- **VirtualBox**:
  1. Create a new virtual machine (Type: **Other**, Version: **Other/Unknown (32-bit)**).
  2. Under **Storage**, attach `tinyos.iso` to the virtual CD/DVD drive.
  3. Start the virtual machine.
- **VMware Workstation / Player**:
  1. Create a custom VM with **Other (32-bit)** OS.
  2. Set the CD-ROM drive to use the `tinyos.iso` file and boot from CD.
- **Physical Hardware (USB Drive / CD-R)**:
  - Write `tinyos.iso` to a USB drive using [Rufus](https://rufus.ie/) (in DD mode) or [Ventoy](https://www.ventoy.net/).
  - Boot your PC in **BIOS / Legacy CSM mode** and select the USB drive.

---

## 7. Verified Terminal Session

Below is a transcript of an interactive session running on `tinyos.iso`:

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

tinyos:/> echo "TinyOS is running on bare metal!" > greeting.txt
tinyos:/> append greeting.txt "Built from scratch with C and Assembly."
Appended 40 bytes to 'greeting.txt'

tinyos:/> cat greeting.txt
"TinyOS is running on bare metal!"
"Built from scratch with C and Assembly."

tinyos:/> calc 1024 * 16
1024 * 16 = 16384

tinyos:/> date
Current RTC Time: 2026-11-06 07:05:44 UTC

tinyos:/> uptime
Uptime: 00:00:28 (28 seconds, 2800 ticks)

tinyos:/> cpu

CPU & System Information:
-------------------------
  OS:             TinyOS v1.0.0
  Architecture:   x86 (32-bit Protected Mode)
  CPU Vendor:     AuthenticAMD
  CPUID Max In:   0xd
  Boot Media:     El Torito Bootable CD-ROM ISO
  Filesystem:     TinyFS In-Memory RAM Filesystem

tinyos:/> shutdown
Powering off TinyOS...
```

---

## 8. License

TinyOS is open-source software licensed under the **[MIT License](LICENSE)**.
You are free to use, modify, and distribute this software for educational and personal projects.
