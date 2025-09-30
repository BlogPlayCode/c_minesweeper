#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define MINE -1
#define BUTTON_WIDTH 32
#define BUTTON_HEIGHT 32
#define BUTTON_ID_BASE 1000
#define CONTROL_PANEL_HEIGHT 40

int COLS = 15, ROWS = 8, MINES_COUNT;
int user_cols = 15, user_rows = 8, user_mines = 20;
char WINDOW_TITLE[] = "Minesweeper";
int gameover = 0;
COLORREF colors[10] = {
    RGB(255, 0, 0),     // X
    RGB(32, 32, 32),    // 0
    RGB(124, 199, 255), // 1
    RGB(102, 194, 102), // 2
    RGB(255, 119, 136), // 3
    RGB(238, 136, 255), // 4
    RGB(221, 170, 34),  // 5
    RGB(102, 204, 204), // 6
    RGB(180, 180, 180), // 7
    RGB(255, 255, 255), // 8
};
char view_symbols[] = "X 12345678";
char flag[] = "F";
int *field = NULL;
char *user_field = NULL;
bool *isClicked = NULL;
WNDPROC originalButtonProc = NULL;

// Объявления элементов управления
HWND hRestartButton = NULL;
HWND hColsEdit = NULL, hRowsEdit = NULL, hMinesEdit = NULL;

// Объявление функции рестарта
void restartGame();

LRESULT CALLBACK ButtonSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) {
        int id = GetDlgCtrlID(hwnd);
        if (id >= BUTTON_ID_BASE && id < BUTTON_ID_BASE + ROWS * COLS) {
            int button_id = id - BUTTON_ID_BASE;
            int row = button_id / COLS;
            int col = button_id % COLS;
            
            if (!isClicked[button_id]) {
                if (user_field[button_id] == '?') {
                    user_field[button_id] = 'F';
                } else if (user_field[button_id] == 'F') {
                    user_field[button_id] = '?';
                }
                
                char label[8];
                if (user_field[button_id] == 'F') {
                    snprintf(label, sizeof(label), " %s ", flag);
                } else {
                    snprintf(label, sizeof(label), " ? ");
                }
                SetWindowText(hwnd, label);
                InvalidateRect(hwnd, NULL, TRUE);
            }
            
            return 0;
        }
    }
    
    return CallWindowProc(originalButtonProc, hwnd, msg, wParam, lParam);
}

void ClickButtonAt(HWND hwnd, int row, int col, bool rightClick) {
    int button_id = BUTTON_ID_BASE + row * COLS + col;
    HWND hButton = GetDlgItem(hwnd, button_id);
    
    if (hButton) {
        if (rightClick) {
            SendMessage(hButton, WM_RBUTTONDOWN, 0, 0);
            SendMessage(hButton, WM_RBUTTONUP, 0, 0);
        } else {
            SendMessage(hwnd, WM_COMMAND, 
                       MAKEWPARAM(button_id, BN_CLICKED), 
                       (LPARAM)hButton);
        }
    }
}

void UpdateAllButtons(HWND hwnd) {
    for (int i = 0; i < ROWS * COLS; i++) {
        HWND hButton = GetDlgItem(hwnd, BUTTON_ID_BASE + i);
        if (hButton) {
            InvalidateRect(hButton, NULL, TRUE);
            UpdateWindow(hButton);
        }
    }
}

// Функция для рекурсивного открытия клеток
void openCell(HWND hwnd, int row, int col) {
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) return;
    
    int id = row * COLS + col;
    
    // Если клетка уже открыта или помечена флагом - пропускаем
    if (isClicked[id] || user_field[id] == 'F') return;
    
    // Открываем клетку
    isClicked[id] = true;
    user_field[id] = field[id];
    
    // Обновляем отображение кнопки
    HWND hButton = GetDlgItem(hwnd, BUTTON_ID_BASE + id);
    if (hButton) {
        char label[8];
        snprintf(label, sizeof(label), " %c ", view_symbols[field[id] + 1]);
        SetWindowText(hButton, label);
        InvalidateRect(hButton, NULL, TRUE);
    }
    
    // Если клетка пустая (0), рекурсивно открываем всех соседей
    if (field[id] == 0) {
        int points[8][2] = {
            {col-1, row-1}, {col-1, row}, {col-1, row+1},
            {col, row-1}, {col, row+1},
            {col+1, row-1}, {col+1, row}, {col+1, row+1}
        };
        
        for (int i = 0; i < 8; i++) {
            int newCol = points[i][0];
            int newRow = points[i][1];
            openCell(hwnd, newRow, newCol);
        }
    }
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Создаем кнопку рестарта
            hRestartButton = CreateWindow(
                "BUTTON", "Restart",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                13, 12, 80, 25,
                hwnd, (HMENU)1, NULL, NULL
            );
            
            // Создаем поля ввода с подписями
            CreateWindow("STATIC", "Width:", 
                        WS_VISIBLE | WS_CHILD,
                        103, 14, 40, 20, hwnd, NULL, NULL, NULL);
            
            hColsEdit = CreateWindow("EDIT", "15", 
                                    WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                                    143, 12, 40, 20, hwnd, NULL, NULL, NULL);
            
            CreateWindow("STATIC", "Height:", 
                        WS_VISIBLE | WS_CHILD,
                        193, 14, 40, 20, hwnd, NULL, NULL, NULL);
            
            hRowsEdit = CreateWindow("EDIT", "8", 
                                   WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                                   233, 12, 40, 20, hwnd, NULL, NULL, NULL);
            
            CreateWindow("STATIC", "Mines:", 
                        WS_VISIBLE | WS_CHILD,
                        283, 14, 40, 20, hwnd, NULL, NULL, NULL);
            
            hMinesEdit = CreateWindow("EDIT", "20", 
                                     WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                                     323, 12, 40, 20, hwnd, NULL, NULL, NULL);
            
            // Создаем кнопки игрового поля
            for (int i = 0; i < ROWS; i++) {
                for (int j = 0; j < COLS; j++) {
                    char label[8];
                    snprintf(label, sizeof(label), " %c ", user_field[i * COLS + j]);
                    HWND button = CreateWindow(
                        "BUTTON", label,
                        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                        j * BUTTON_WIDTH, i * BUTTON_HEIGHT + CONTROL_PANEL_HEIGHT,
                        BUTTON_WIDTH, BUTTON_HEIGHT,
                        hwnd, (HMENU)((uintptr_t)(BUTTON_ID_BASE + i * COLS + j)), NULL, NULL
                    );
                    
                    if (originalButtonProc == NULL) {
                        originalButtonProc = (WNDPROC)GetWindowLongPtr(button, GWLP_WNDPROC);
                    }
                    SetWindowLongPtr(button, GWLP_WNDPROC, (LONG_PTR)ButtonSubclassProc);
                }
            }
            break;
        }
        
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            
            // Обработка кнопки рестарта
            if (wmId == 1) {
                restartGame();
                break;
            }
            
            // Обработка игровых кнопок
            if (wmId >= BUTTON_ID_BASE && wmId < BUTTON_ID_BASE + ROWS * COLS) {
                int id = wmId - BUTTON_ID_BASE;
                int row = id / COLS;
                int col = id % COLS;
                
                if (gameover == 1 && field[id] != MINE) return 0;
                if (gameover == 2) return 0;
                
                if (!isClicked[id] && user_field[id] != 'F') {
                    // Открываем клетку с помощью рекурсивной функции
                    openCell(hwnd, row, col);

                    // Проверяем, не попали ли на мину
                    if (field[id] == MINE && !gameover) {
                        gameover = 1;
                        printf("You loose\n");
                        // Открываем все мины
                        for (int i = 0; i < ROWS * COLS; i++) {
                            if (field[i] == MINE) {
                                int mineRow = i / COLS;
                                int mineCol = i % COLS;
                                openCell(hwnd, mineRow, mineCol);
                            }
                        }
                        return 0;
                    }
                } else if (isClicked[id] && field[id] != MINE && field[id] != 0) {
                    // Обработка двойного клика на число
                    int points[8][2] = {
                        {col - 1, row - 1}, {col - 1, row}, {col - 1, row + 1},
                        {col, row - 1}, {col, row + 1},
                        {col + 1, row - 1}, {col + 1, row}, {col + 1, row + 1},
                    };
                    int count1 = 0, count2 = 0;
                    for (int i = 0; i < 8; i++) {
                        int x1 = points[i][0];
                        int y1 = points[i][1];
                        if (x1 < 0 || x1 >= COLS || y1 < 0 || y1 >= ROWS) continue;
                        if (field[y1 * COLS + x1] == MINE) count1++;
                        if (user_field[y1 * COLS + x1] == 'F') count2++;
                    }
                    if (count1 == count2) {
                        for (int i = 0; i < 8; i++) {
                            int x1 = points[i][0];
                            int y1 = points[i][1];
                            if (x1 < 0 || x1 >= COLS || y1 < 0 || y1 >= ROWS) continue;
                            if (user_field[y1 * COLS + x1] == '?') {
                                openCell(hwnd, y1, x1);
                                
                                // Проверяем, не открыли ли мину при двойном клике
                                if (field[y1 * COLS + x1] == MINE && !gameover) {
                                    gameover = 1;
                                    printf("You loose\n");
                                    for (int j = 0; j < ROWS * COLS; j++) {
                                        if (field[j] == MINE) {
                                            int mineRow = j / COLS;
                                            int mineCol = j % COLS;
                                            openCell(hwnd, mineRow, mineCol);
                                        }
                                    }
                                    return 0;
                                }
                            }
                        }
                    }
                }
                
                // Проверка победы
                if (!gameover) {
                    int allCleared = 1;
                    for (int i = 0; i < ROWS * COLS; i++) {
                        if ((user_field[i] == '?' || user_field[i] == 'F') && field[i] != MINE) {
                            allCleared = 0;
                            break;
                        }
                    }
                    if (allCleared) {
                        gameover = 2;
                        UpdateAllButtons(hwnd);
                        printf("You won\n");
                    }
                }
            }
            break;
        }
        
        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        }
        
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
            if (dis->CtlType == ODT_BUTTON) {
                int id = dis->CtlID - BUTTON_ID_BASE;
                if (id >= 0 && id < ROWS * COLS) {
                    HDC hdc = dis->hDC;
                    RECT rc = dis->rcItem;
                    
                    COLORREF bkColor = isClicked[id] ? ((field[id] == MINE) ? RGB(255, 0, 0) : RGB(32, 32, 32)) : RGB(55, 55, 55);
                    if (gameover == 2) if (field[id] != MINE) bkColor = RGB(88, 200, 88);
                    HBRUSH hBrush = CreateSolidBrush(bkColor);
                    FillRect(hdc, &rc, hBrush);
                    DeleteObject(hBrush);
                    
                    COLORREF textColor;
                    if (isClicked[id]) {
                        textColor = colors[field[id] + 1];
                        if (field[id] == MINE) textColor = RGB(255, 255, 255);
                    } else {
                        textColor = (user_field[id] == 'F') ? RGB(255, 0, 0) : RGB(255, 255, 255);
                    }
                    if (gameover == 2) if (field[id] != MINE) textColor = RGB(0, 255, 0);
                    
                    SetTextColor(hdc, textColor);
                    SetBkMode(hdc, TRANSPARENT);
                    
                    char text[16];
                    GetWindowText(dis->hwndItem, text, sizeof(text));
                    DrawText(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    
                    HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(77, 77, 77));
                    HPEN hOldPen = (HPEN)SelectObject(hdc, hBorderPen);
                    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                    
                    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
                    
                    SelectObject(hdc, hOldPen);
                    SelectObject(hdc, hOldBrush);
                    DeleteObject(hBorderPen);
                }
            }
            return TRUE;
        }
        
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void generate_field(int *field, int mines_count) {
    int *cords = malloc(ROWS * COLS * sizeof(int));
    for (int i = 0; i < ROWS * COLS; i++) cords[i] = i;
    int index;
    for (int i = 0; i < mines_count; i++) {
        index = rand() % (ROWS * COLS - i);
        field[cords[index]] = MINE;
        for (int j = index; j < ROWS * COLS - i - 1; j++) {
            cords[j] = cords[j + 1];
        }
    }
    free(cords);
    
    for (int i = 0; i < ROWS * COLS; i++) {
        if (field[i] == MINE) {
            printf(" X");
            if (i % COLS == COLS - 1) printf("\n");
            continue;
        }
        int x = i % COLS, y = i / COLS;
        int points[8][2] = {
            {x - 1, y - 1}, {x - 1, y}, {x - 1, y + 1},
            {x, y - 1}, {x, y + 1},
            {x + 1, y - 1}, {x + 1, y}, {x + 1, y + 1},
        };
        int count = 0;
        for (int i = 0; i < 8; i++) {
            int x1 = points[i][0];
            int y1 = points[i][1];
            if (x1 < 0 || x1 >= COLS || y1 < 0 || y1 >= ROWS) continue;
            if (field[y1 * COLS + x1] == MINE) count++;
        }
        field[i] = count;
        printf(" %d", count);
        if (x == COLS - 1) printf("\n");
    }
}

// Функция рестарта игры
void restartGame() {
    // Получаем значения из полей ввода
    char colsText[10], rowsText[10], minesText[10];
    GetWindowText(hColsEdit, colsText, sizeof(colsText));
    GetWindowText(hRowsEdit, rowsText, sizeof(rowsText));
    GetWindowText(hMinesEdit, minesText, sizeof(minesText));
    
    user_cols = atoi(colsText);
    user_rows = atoi(rowsText);
    user_mines = atoi(minesText);
    
    // Проверка валидности значений
    if (user_cols < 5) user_cols = 5;
    if (user_cols > 50) user_cols = 50;
    if (user_rows < 5) user_rows = 5;
    if (user_rows > 30) user_rows = 30;
    if (user_mines < 1) user_mines = 1;
    if (user_mines > user_cols * user_rows * 0.8) user_mines = user_cols * user_rows * 0.8;
    
    // Обновляем глобальные переменные
    COLS = user_cols;
    ROWS = user_rows;
    MINES_COUNT = user_mines;
    
    // Сброс состояния игры
    gameover = 0;
    
    // Освобождаем старую память
    free(isClicked);
    free(field);
    free(user_field);
    
    // Выделяем новую память
    isClicked = malloc(COLS * ROWS * sizeof(bool));
    field = malloc(COLS * ROWS * sizeof(int));
    user_field = malloc(COLS * ROWS * sizeof(char));
    
    // Инициализация
    for (int i = 0; i < COLS * ROWS; i++) {
        isClicked[i] = false;
        user_field[i] = '?';
        field[i] = 0;
    }
    
    // Генерация нового поля
    generate_field(field, MINES_COUNT);
    
    // Пересоздаем окно (простой способ обновить интерфейс)
    HWND hwnd = GetParent(hRestartButton);
    RECT rc;
    GetClientRect(hwnd, &rc);
    
    // Уничтожаем старые кнопки
    for (int i = 0; i < ROWS * COLS; i++) {
        HWND hButton = GetDlgItem(hwnd, BUTTON_ID_BASE + i);
        if (hButton) DestroyWindow(hButton);
    }
    
    // Создаем новые кнопки
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            char label[8];
            snprintf(label, sizeof(label), " %c ", user_field[i * COLS + j]);
            HWND button = CreateWindow(
                "BUTTON", label,
                WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                j * BUTTON_WIDTH, i * BUTTON_HEIGHT + CONTROL_PANEL_HEIGHT, 
                BUTTON_WIDTH, BUTTON_HEIGHT,
                hwnd, (HMENU)((uintptr_t)(BUTTON_ID_BASE + i * COLS + j)), NULL, NULL
            );
            
            if (originalButtonProc == NULL) {
                originalButtonProc = (WNDPROC)GetWindowLongPtr(button, GWLP_WNDPROC);
            }
            SetWindowLongPtr(button, GWLP_WNDPROC, (LONG_PTR)ButtonSubclassProc);
        }
    }
    
    // Изменяем размер окна под новое поле
    SetWindowPos(hwnd, NULL, 0, 0, 
                 COLS * BUTTON_WIDTH + 6, 
                 ROWS * BUTTON_HEIGHT + CONTROL_PANEL_HEIGHT + 29,
                 SWP_NOMOVE | SWP_NOZORDER);
    
    UpdateAllButtons(hwnd);
}

int main(int argc, char **argv) {
    srand(time(NULL));
    MINES_COUNT = (int)(ROWS * COLS / 6);
    user_mines = MINES_COUNT;
    
    colors[1] = GetSysColor(COLOR_BTNTEXT);
    isClicked = malloc(COLS * ROWS * sizeof(bool));
    field = malloc(COLS * ROWS * sizeof(int));
    user_field = malloc(COLS * ROWS * sizeof(char));
    
    for (int i = 0; i < COLS * ROWS; i++) {
        isClicked[i] = false;
        user_field[i] = '?';
        field[i] = 0;
    }
    
    generate_field(field, MINES_COUNT);
    
    DWORD pids[10];
    DWORD count = GetConsoleProcessList(pids, 10);
    if (count == 1) {
        FreeConsole();
    }

    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASS wc = {0};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;   
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = "ButtonMatrixClass";

    RegisterClass(&wc);

    // Создаем окно без возможности изменения размера
    HWND hwnd = CreateWindow(
        "ButtonMatrixClass", WINDOW_TITLE,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, // Убрали WS_THICKFRAME для фиксированного размера
        CW_USEDEFAULT, CW_USEDEFAULT,
        COLS * BUTTON_WIDTH + 6, 
        ROWS * BUTTON_HEIGHT + CONTROL_PANEL_HEIGHT + 29,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    free(isClicked);
    free(field);
    free(user_field);
    return (int)msg.wParam;
}
