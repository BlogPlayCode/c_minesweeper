#!/bin/bash

# Script to install dependencies if needed, compile, and run Minesweeper

# Function to check if a package is installed
is_installed() {
    dpkg -l "$1" &> /dev/null
    return $?
}

# Check and install build-essential
if ! is_installed build-essential; then
    echo "Installing build-essential..."
    sudo apt-get update
    sudo apt-get install -y build-essential
fi

# Check and install pkg-config
if ! is_installed pkg-config; then
    echo "Installing pkg-config..."
    sudo apt-get install -y pkg-config
fi

# Check and install libgtk-3-dev
if ! is_installed libgtk-3-dev; then
    echo "Installing libgtk-3-dev..."
    sudo apt-get install -y libgtk-3-dev
fi

# Clean previous build if exists
if [ -f minesweeper ]; then
    rm minesweeper
    echo "Cleaned previous build."
fi

# Compile the project
echo "Compiling minesweeper.c..."
gcc -o minesweeper minesweeper.c `pkg-config --cflags --libs gtk+-3.0`

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful. Running the program..."
    ./minesweeper
else
    echo "Compilation failed. Please check the errors above."
fi
