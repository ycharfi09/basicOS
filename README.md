# BasicOS

A comprehensive operating system built from scratch with advanced kernel features, real filesystem support, and a full desktop environment.

## 🎯 Project Overview

BasicOS is an educational **daily-driver capable** operating system that demonstrates modern OS concepts from bootloader to GUI applications. Initially a hobby project, it has evolved into a feature-rich OS with multitasking, virtual memory, filesystem support, and a complete desktop environment.

**Key Highlights:**
- Custom x86_64 kernel with virtual memory management
- Full multitasking with preemptive scheduler
- Real filesystem support (FAT32 via ATA disk driver)
- Hardware drivers (storage, keyboard, mouse, timer, graphics)
- Complete GUI desktop environment with window manager
- Working terminal with real commands
- Built-in applications

## ✨ Features

### Kernel (v2.0)
- **Bootloader**: Limine bootloader for modern UEFI/BIOS support
- **Architecture**: x86_64 long mode
- **Memory Management**: 
  - Physical memory manager (PMM) with bitmap allocator
  - Virtual memory manager (VMM) with 4-level paging
  - Improved heap allocator with proper kfree() and block merging
- **Process Management**:
  - Process Control Blocks (PCB) with CPU context
  - Round-robin scheduler with preemptive multitasking
  - Context switching (assembly implementation)
- **System Calls**: 11 syscalls (exit, fork, read, write, open, close, wait, exec, getpid, sleep, yield)
- **Logging**: Kernel logging system with multiple log levels
- **GDT/IDT**: Proper segment and interrupt descriptor tables
- **Interrupts**: PIC-based interrupt handling with scheduler integration

### Drivers
- **Framebuffer**: Direct framebuffer graphics with 8x8 bitmap font
- **Keyboard**: PS/2 keyboard driver with scancode translation
- **Mouse**: PS/2 mouse driver with button and position tracking
- **Timer**: PIT-based timer at 1000 Hz with scheduler integration
- **Storage**: ATA disk driver (PIO mode) for reading/writing sectors
- **Filesystem**: FAT32 driver with read support and directory listing

### Filesystem & VFS
- **Virtual File System (VFS)**: Unified file operations interface
- **FAT32 Support**: Read files and list directories
- **File Operations**: open, close, read, exists, file_size
- **Directory Operations**: List directory contents with metadata

### GUI Desktop Environment
- **Top Bar**: Launcher button, clock display, and status icons
- **Window Manager**: Three window modes:
  - Focused Mode (fullscreen)
  - Draggable Mode (floating windows)
  - Overlay Mode (overlapping windows)
- **Window Controls**: Close, focus, and move operations
- **Mouse Cursor**: Software-rendered cursor

### Built-in Applications
1. **Terminal**: Full-featured command-line interface with real commands:
   - `help` - Show available commands
   - `ls` - List directory contents
   - `cat` - Display file contents
   - `echo` - Echo text
   - `clear` - Clear screen
   - `pwd` - Print working directory
   - `uname` - System information
2. **Text Editor**: Basic text editing with keyboard input
3. **Settings**: UI for toggling color schemes
4. **File Manager**: Directory browser (filesystem integrated)
5. **Demo Game**: Placeholder for game (Fireboy & Watergirl themed)

## 🔧 Building BasicOS

### Prerequisites

#### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y build-essential nasm xorriso qemu-system-x86
```

#### Arch Linux:
```bash
sudo pacman -S base-devel nasm libisoburn qemu
```

#### macOS (with Homebrew):
```bash
brew install nasm xorriso qemu
```

### Build Instructions

1. **Clone the repository**:
```bash
git clone https://github.com/ycharfi09/basicOS.git
cd basicOS
```

2. **Build the OS**:
```bash
make
```

This will:
- Download and build the Limine bootloader
- Compile the kernel and all drivers
- Compile the GUI and applications
- Create a bootable ISO image at `build/basicOS.iso`

3. **Clean build artifacts** (optional):
```bash
make clean       # Clean build files
make distclean   # Clean everything including Limine
```

## 🚀 Running BasicOS

### QEMU (BIOS mode)
```bash
make run
```

Or manually:
```bash
qemu-system-x86_64 -cdrom build/basicOS.iso -m 256M -vga std
```

### QEMU (UEFI mode)
```bash
make run-uefi
```

Or manually (requires OVMF):
```bash
qemu-system-x86_64 -cdrom build/basicOS.iso -m 256M \
    -bios /usr/share/ovmf/OVMF.fd -vga std
```

### VirtualBox

1. Create a new virtual machine:
   - Type: Other
   - Version: Other/Unknown (64-bit)
   - Memory: 256 MB minimum
   - No hard disk needed

2. Mount the ISO:
   - Settings → Storage → Add Optical Drive
   - Select `build/basicOS.iso`

3. Start the VM

### Recommended QEMU Options

For better performance and features:
```bash
qemu-system-x86_64 \
    -cdrom build/basicOS.iso \
    -m 512M \
    -vga std \
    -display gtk \
    -device usb-mouse \
    -device usb-kbd
```

## 🎮 Using BasicOS

### Interacting with the OS

1. **Launch Applications**: Click the "Apps" button in the top-left corner
2. **Open Windows**: Click on any app in the launcher menu
3. **Type in Windows**: Click on a window and start typing (Terminal, Editor)
4. **Close Windows**: Click the red "X" button in the window title bar
5. **Move Mouse**: The mouse cursor follows your movements

### Keyboard Shortcuts

Currently, there are no special keyboard shortcuts - the OS responds to direct keyboard input in focused windows.

## 📁 Project Structure

```
basicOS/
├── bootloader/         # Limine bootloader configuration
│   └── limine.cfg      # Bootloader config
├── kernel/             # Core kernel code
│   ├── include/        # Kernel headers
│   ├── boot.asm        # Assembly entry point
│   ├── main.c          # Kernel main function
│   ├── gdt.c           # Global Descriptor Table
│   ├── idt.c           # Interrupt Descriptor Table
│   ├── memory.c        # Heap memory management
│   ├── paging.c        # Virtual memory (VMM/PMM)
│   ├── process.c       # Process management
│   ├── syscall.c       # System call interface
│   ├── vfs.c           # Virtual File System
│   ├── log.c           # Kernel logging
│   ├── isr.c           # Interrupt service routines
│   └── context_switch.asm  # Context switching
├── drivers/            # Hardware drivers
│   ├── include/        # Driver headers
│   ├── framebuffer.c   # Graphics driver
│   ├── keyboard.c      # Keyboard driver
│   ├── mouse.c         # Mouse driver
│   ├── timer.c         # Timer driver with scheduler
│   ├── pic.c           # Interrupt controller
│   ├── ata.c           # ATA disk driver
│   └── fat32.c         # FAT32 filesystem driver
├── gui/                # GUI framework
│   ├── include/        # GUI headers
│   └── gui.c           # Window manager
├── apps/               # Built-in applications
│   ├── terminal.c      # Terminal app with commands
│   ├── editor.c        # Text editor app
│   ├── settings.c      # Settings app
│   ├── files.c         # File manager app
│   └── game.c          # Demo game app
├── lib/                # Standard library headers
│   └── include/        # stdint, stddef, stdbool, stdarg
├── build/              # Build output directory
├── Makefile            # Build system
├── linker.ld           # Linker script
└── README.md           # This file
```

## 🔍 Technical Details

### Boot Process

1. **BIOS/UEFI**: Loads Limine bootloader from ISO
2. **Limine**: Switches to 64-bit long mode, sets up framebuffer
3. **Kernel Entry**: `boot.asm` sets up stack and calls `kernel_main()`
4. **Initialization**:
   - Initialize GDT (segment descriptors)
   - Initialize IDT (interrupt handlers)
   - Initialize memory management (heap, paging)
   - Initialize framebuffer driver
   - Initialize PIC (interrupt controller)
   - Initialize hardware drivers (timer, keyboard, mouse, ATA disk)
   - Initialize logging system
   - Initialize VFS and FAT32 filesystem
   - Initialize process management and scheduler
   - Initialize system call interface
   - Enable interrupts
   - Initialize GUI framework
5. **Main Loop**: Update and render GUI at ~60 FPS

### Memory Layout

- **Kernel**: Loaded at `0xFFFFFFFF80100000` (higher half)
- **Heap**: 16 MB allocator with proper free support
- **Paging**: 4-level page tables (PML4)
- **Stack**: 8 KB per process
- **Framebuffer**: Directly mapped by Limine

### Interrupt Handling

- **ISRs 0-31**: CPU exceptions (divide by zero, page fault, etc.)
- **IRQ 0 (INT 32)**: Timer interrupt (1000 Hz, scheduler tick)
- **IRQ 1 (INT 33)**: Keyboard interrupt
- **IRQ 12 (INT 44)**: Mouse interrupt
- **PIC**: Remapped to avoid conflicts with CPU exceptions

### Process Management

- **PCB Structure**: Stores process state, CPU context, page tables
- **Scheduler**: Round-robin preemptive scheduler
- **Context Switching**: Assembly-level CPU state save/restore
- **Time Slicing**: 10ms time slices for fair CPU distribution

### Filesystem Architecture

- **VFS Layer**: Unified interface for file operations
- **FAT32 Driver**: Reads boot sector, FAT entries, directories
- **ATA Driver**: PIO mode disk I/O at sector level
- **File Operations**: open(), read(), close(), exists(), file_size()
- **Directory Operations**: list_directory() with metadata

### GUI Architecture

- **Window Manager**: Manages window creation, focus, and rendering
- **Event Loop**: Processes keyboard and mouse input
- **Rendering**: Direct framebuffer access, no GPU acceleration
- **Font**: 8x8 bitmap font embedded in framebuffer driver

## 🛠️ Development

### Adding New Features

To add a new application:

1. Create `apps/myapp.c`
2. Implement render and event handlers
3. Add `app_myapp_create()` function
4. Add to launcher menu in `gui/gui.c`

### Debugging

Enable verbose QEMU output:
```bash
qemu-system-x86_64 -cdrom build/basicOS.iso -m 256M \
    -serial stdio -d int,cpu_reset
```

### Known Limitations

- **Write Support**: FAT32 write operations not yet implemented
- **Networking**: No network stack or drivers (planned)
- **SMP**: Single-core only (multi-core support planned)
- **User/Kernel Separation**: No privilege separation yet
- **Limited Drivers**: Basic PS/2 input, ATA disk only
- **GUI**: Software rendering only, no GPU acceleration
- **File Creation**: Cannot create new files yet
- **IPC**: Inter-process communication not implemented

### Implemented Features (v2.0)

✅ Virtual memory management with paging  
✅ Process management with scheduler  
✅ System call interface  
✅ FAT32 filesystem (read support)  
✅ ATA disk driver  
✅ Kernel logging system  
✅ Real terminal with commands  
✅ File operations (open, read, close)  
✅ Directory listing  
✅ Memory allocator with free support  

## 📚 Resources

### Learning OS Development

- [OSDev Wiki](https://wiki.osdev.org/)
- [Writing an OS in Rust](https://os.phil-opp.com/)
- [The Little Book About OS Development](https://littleosbook.github.io/)
- [Limine Boot Protocol](https://github.com/limine-bootloader/limine)

### x86_64 Architecture

- [Intel Software Developer Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [AMD64 Architecture Programmer's Manual](https://www.amd.com/en/support/tech-docs)

## 📄 License

This is an educational project created for learning purposes. Feel free to use and modify as you wish.

## 🙏 Acknowledgments

- **Limine Bootloader**: Modern, easy-to-use bootloader for hobby OSes
- **OSDev Community**: Extensive documentation and support
- **QEMU**: Essential tool for OS development and testing

## 🤝 Contributing

This is a personal learning project, but suggestions and improvements are welcome! Feel free to:
- Report bugs or issues
- Suggest new features
- Share your own OS development experiences

---

**Happy OS Development!** 🎉

Built with ❤️ for learning and fun.