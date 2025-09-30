del minesweeper.exe -f > NULL
gcc minesweeper.c -o minesweeper.exe -lgdi32 -luser32
start minesweeper.exe
