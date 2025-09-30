#!/bin/bash

# Script to install dependencies, compile, and run Minesweeper on Linux

# Function to detect the Linux distribution
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO=$ID
        echo "Detected distribution: $DISTRO"
    else
        echo "Cannot detect distribution. Assuming Debian/Ubuntu."
        DISTRO="debian"
    fi
}

# Function to check if a command is available
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to install dependencies based on the distribution
install_dependencies() {
    case $DISTRO in
        ubuntu|debian)
            echo "Installing dependencies for Ubuntu/Debian..."
            sudo apt-get update
            sudo apt-get install -y build-essential pkg-config libgtk-3-dev
            ;;
        fedora)
            echo "Installing dependencies for Fedora..."
            sudo dnf install -y gcc pkgconf gtk3-devel
            ;;
        arch|manjaro)
            echo "Installing dependencies for Arch Linux..."
            sudo pacman -Syu --noconfirm base-devel pkgconf gtk3
            ;;
        *)
            echo "Unsupported distribution: $DISTRO"
            echo "Please manually install the following packages:"
            echo "- GCC compiler (e.g., build-essential or gcc)"
            echo "- pkg-config (e.g., pkgconf)"
            echo "- GTK+ 3 development libraries (e.g., libgtk-3-dev or gtk3-devel)"
            exit 1
            ;;
    esac
}

# Function to check if pkg-config can find gtk+-3.0
check_pkg_config() {
    if ! pkg-config --modversion gtk+-3.0 >/dev/null 2>&1; then
        echo "Error: gtk+-3.0 not found in pkg-config."
        echo "Attempting to install GTK+ 3.0 development libraries..."
        install_dependencies
        if ! pkg-config --modversion gtk+-3.0 >/dev/null 2>&1; then
            echo "Error: gtk+-3.0 still not found after installation."
            echo "Please ensure libgtk-3-dev (or equivalent) is installed and pkg-config is correctly set up."
            echo "You may need to set PKG_CONFIG_PATH. For example:"
            echo "export PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig"
            exit 1
        fi
    fi
}

# Detect the Linux distribution
detect_distro

# Check for required tools
if ! command_exists gcc; then
    echo "GCC is not installed."
    install_dependencies
fi

if ! command_exists pkg-config; then
    echo "pkg-config is not installed."
    install_dependencies
fi

# Check for GTK+ 3.0
check_pkg_config

# Clean previous build if exists
if [ -f minesweeper.o ]; then
    rm minesweeper.o
    echo "Cleaned previous build."
fi

# Compile the project
echo "Compiling minesweeper.c..."
gcc -o minesweeper.o linux_minesweeper.c `pkg-config --cflags --libs gtk+-3.0`

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful. Running the program..."
    chmod +x ./minesweeper.o
    ./minesweeper.o
else
    echo "Compilation failed. Please check the errors above."
    echo "Common issues:"
    echo "- Ensure libgtk-3-dev (Ubuntu/Debian), gtk3-devel (Fedora), or gtk3 (Arch) is installed."
    echo "- Check if PKG_CONFIG_PATH includes the directory containing gtk+-3.0.pc."
    exit 1
fi
