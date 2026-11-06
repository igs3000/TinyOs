#!/usr/bin/env python3
"""
El Torito Bootable ISO 9660 Generator for TinyOS
Creates a standard ISO image that boots on BIOS/SeaBIOS/UEFI CSM.
"""

import struct
import sys
import os

SECTOR_SIZE = 2048
FLOPPY_SIZE = 1474560  # 1.44 MB floppy disk image (2880 * 512)

def both_endian_16(val):
    return struct.pack('<H', val) + struct.pack('>H', val)

def both_endian_32(val):
    return struct.pack('<I', val) + struct.pack('>I', val)

def create_iso(boot_bin_path, kernel_bin_path, output_iso_path):
    print(f"Reading bootloader: {boot_bin_path}")
    with open(boot_bin_path, 'rb') as f:
        boot_code = f.read()

    print(f"Reading kernel: {kernel_bin_path}")
    with open(kernel_bin_path, 'rb') as f:
        kernel_code = f.read()

    if len(boot_code) > 512:
        raise ValueError(f"Bootloader too large: {len(boot_code)} bytes (max 512)")

    # Prepare 1.44MB floppy image
    floppy = bytearray(FLOPPY_SIZE)
    floppy[0:len(boot_code)] = boot_code

    # Kernel is placed at LBA 1 (offset 512)
    max_kernel_size = 128 * 512  # 64 KB
    if len(kernel_code) > max_kernel_size:
        raise ValueError(f"Kernel too large for 128 sectors: {len(kernel_code)} bytes (max {max_kernel_size})")

    floppy[512:512+len(kernel_code)] = kernel_code

    # Calculate ISO sector layout
    # Sector 0..15: System area (32 KB)
    # Sector 16: Primary Volume Descriptor
    # Sector 17: El Torito Boot Record
    # Sector 18: Volume Descriptor Set Terminator
    # Sector 19: Boot Catalog
    # Sector 20..739: Floppy image (720 CD sectors)
    load_rba = 20
    floppy_cd_sectors = FLOPPY_SIZE // SECTOR_SIZE  # 720
    total_iso_sectors = load_rba + floppy_cd_sectors + 4

    iso = bytearray(16 * SECTOR_SIZE)

    # Sector 16: Primary Volume Descriptor (PVD)
    pvd = bytearray(SECTOR_SIZE)
    pvd[0] = 0x01                                    # Type: PVD
    pvd[1:6] = b'CD001'                              # Standard identifier
    pvd[6] = 0x01                                    # Version
    pvd[8:40] = b'TINYOS'.ljust(32)                  # System Identifier
    pvd[40:72] = b'TINYOS_BOOTABLE_CD'.ljust(32)     # Volume Identifier
    pvd[80:88] = both_endian_32(total_iso_sectors)   # Volume Space Size
    pvd[120:124] = both_endian_16(1)                 # Volume Set Size
    pvd[124:128] = both_endian_16(1)                 # Volume Sequence Number
    pvd[128:132] = both_endian_16(SECTOR_SIZE)       # Logical Block Size
    pvd[881] = 0x01                                  # File Structure Version
    iso.extend(pvd)

    # Sector 17: Boot Record Volume Descriptor (El Torito)
    brvd = bytearray(SECTOR_SIZE)
    brvd[0] = 0x00                                   # Type: Boot Record
    brvd[1:6] = b'CD001'
    brvd[6] = 0x01
    brvd[7:39] = b'EL TORITO SPECIFICATION'.ljust(32, b'\x00')
    boot_catalog_sector = 19
    brvd[71:75] = struct.pack('<I', boot_catalog_sector)
    iso.extend(brvd)

    # Sector 18: Volume Descriptor Set Terminator
    vdt = bytearray(SECTOR_SIZE)
    vdt[0] = 0xFF                                    # Terminator
    vdt[1:6] = b'CD001'
    vdt[6] = 0x01
    iso.extend(vdt)

    # Sector 19: Boot Catalog
    boot_cat = bytearray(SECTOR_SIZE)

    # Validation Entry (32 bytes)
    val_entry = bytearray(32)
    val_entry[0] = 0x01                              # Header ID
    val_entry[1] = 0x00                              # Platform ID: 80x86
    val_entry[4:28] = b'TINYOS'.ljust(24, b'\x00')   # ID string
    val_entry[30] = 0x55                             # Key 0x55
    val_entry[31] = 0xAA                             # Key 0xAA

    # Checksum: 16-bit word sum across 32 bytes must equal 0
    words = struct.unpack('<16H', val_entry)
    chk = sum(words) & 0xFFFF
    val_checksum = (0x10000 - chk) & 0xFFFF
    val_entry[28:30] = struct.pack('<H', val_checksum)
    boot_cat[0:32] = val_entry

    # Initial / Default Boot Entry (32 bytes)
    boot_entry = bytearray(32)
    boot_entry[0] = 0x88                             # Bootable
    boot_entry[1] = 0x02                             # 1.44M floppy emulation
    boot_entry[2:4] = struct.pack('<H', 0x0000)      # Load Segment (0 -> 0x07C0)
    boot_entry[4] = 0x00                             # System Type
    boot_entry[6:8] = struct.pack('<H', 1)           # Virtual sector count
    boot_entry[8:12] = struct.pack('<I', load_rba)   # Load RBA (Sector 20)
    boot_cat[32:64] = boot_entry
    iso.extend(boot_cat)

    # Sector 20+: Floppy Disk Image
    iso.extend(floppy)

    # Pad to total sectors
    while len(iso) < total_iso_sectors * SECTOR_SIZE:
        iso.extend(b'\x00' * SECTOR_SIZE)

    with open(output_iso_path, 'wb') as f:
        f.write(iso)

    print(f"[ OK ] Bootable ISO generated: {output_iso_path} ({len(iso)} bytes)")

if __name__ == '__main__':
    boot_bin = sys.argv[1] if len(sys.argv) > 1 else 'boot/boot.bin'
    kernel_bin = sys.argv[2] if len(sys.argv) > 2 else 'kernel/kernel.bin'
    out_iso = sys.argv[3] if len(sys.argv) > 3 else 'tinyos.iso'

    create_iso(boot_bin, kernel_bin, out_iso)
