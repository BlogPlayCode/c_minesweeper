#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#define MINE -1
#define BUTTON_SIZE 32
#define CONTROL_PANEL_HEIGHT 40

int COLS = 15, ROWS = 8, MINES_COUNT;
int user_cols = 15, user_rows = 8, user_mines = 20;
char WINDOW_TITLE[] = "Minesweeper";
int gameover = 0;
#define RGB(r, g, b) ((r) | ((g) << 8) | ((b) << 16))
typedef unsigned int COLORREF;
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

GtkWidget *window = NULL;
GtkWidget *restart_button = NULL;
GtkWidget *cols_edit = NULL, *rows_edit = NULL, *mines_edit = NULL;
GtkWidget *main_box = NULL;
GtkWidget *top_box = NULL;
GtkWidget *grid = NULL;
GtkWidget **cell_buttons = NULL;

// Forward declarations
void update_button(int id);
void update_all_buttons();
void openCell(int row, int col);
void generate_field(int *field, int mines_count);
void on_restart(GtkWidget *widget, gpointer data);

gboolean on_cell_right_click(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (event->button != 3) return FALSE;

    int id = GPOINTER_TO_INT(data);
    if (isClicked[id]) return TRUE;

    if (user_field[id] == '?') {
        user_field[id] = 'F';
    } else if (user_field[id] == 'F') {
        user_field[id] = '?';
    }

    update_button(id);
    return TRUE;
}

void on_cell_clicked(GtkWidget *widget, gpointer data) {
    int id = GPOINTER_TO_INT(data);
    int row = id / COLS;
    int col = id % COLS;

    if (gameover == 1 && field[id] != MINE) return;
    if (gameover == 2) return;

    if (!isClicked[id] && user_field[id] != 'F') {
        openCell(row, col);

        if (field[id] == MINE && !gameover) {
            gameover = 1;
            printf("You lose\n");
            for (int i = 0; i < ROWS * COLS; i++) {
                if (field[i] == MINE) {
                    openCell(i / COLS, i % COLS);
                }
            }
            return;
        }
    } else if (isClicked[id] && field[id] != MINE && field[id] != 0) {
        int dx[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
        int dy[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
        int count1 = 0, count2 = 0;
        for (int k = 0; k < 8; k++) {
            int x1 = col + dx[k];
            int y1 = row + dy[k];
            if (x1 < 0 || x1 >= COLS || y1 < 0 || y1 >= ROWS) continue;
            if (field[y1 * COLS + x1] == MINE) count1++;
            if (user_field[y1 * COLS + x1] == 'F') count2++;
        }
        if (count1 == count2) {
            for (int k = 0; k < 8; k++) {
                int x1 = col + dx[k];
                int y1 = row + dy[k];
                if (x1 < 0 || x1 >= COLS || y1 < 0 || y1 >= ROWS) continue;
                if (user_field[y1 * COLS + x1] == '?') {
                    openCell(y1, x1);
                    if (field[y1 * COLS + x1] == MINE && !gameover) {
                        gameover = 1;
                        printf("You lose\n");
                        for (int j = 0; j < ROWS * COLS; j++) {
                            if (field[j] == MINE) {
                                openCell(j / COLS, j % COLS);
                            }
                        }
                        return;
                    }
                }
            }
        }
    }

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
            update_all_buttons();
            printf("You won\n");
        }
    }
}

void update_button(int id) {
    GtkWidget *button = cell_buttons[id];
    GtkStyleContext *context = gtk_widget_get_style_context(button);

    gtk_style_context_remove_class(context, "button-closed");
    gtk_style_context_remove_class(context, "button-opened");
    gtk_style_context_remove_class(context, "button-mine");
    gtk_style_context_remove_class(context, "button-win");

    const char *class_name;
    if (gameover == 2 && field[id] != MINE) {
        class_name = "button-win";
    } else if (isClicked[id]) {
        class_name = (field[id] == MINE) ? "button-mine" : "button-opened";
    } else {
        class_name = "button-closed";
    }
    gtk_style_context_add_class(context, class_name);

    char text[8];
    if (isClicked[id]) {
        snprintf(text, sizeof(text), " %c ", view_symbols[field[id] + 1]);
    } else {
        if (user_field[id] == 'F') {
            snprintf(text, sizeof(text), " %s ", flag);
        } else {
            snprintf(text, sizeof(text), " ? ");
        }
    }

    COLORREF color;
    if (isClicked[id]) {
        color = colors[field[id] + 1];
        if (field[id] == MINE) color = RGB(255, 255, 255);
    } else {
        color = (user_field[id] == 'F') ? RGB(255, 0, 0) : RGB(255, 255, 255);
    }
    if (gameover == 2 && field[id] != MINE) color = RGB(0, 255, 0);

    int r = color & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = (color >> 16) & 0xFF;

    char markup[100];
    snprintf(markup, sizeof(markup), "<span foreground='#%02X%02X%02X'>%s</span>", r, g, b, text);

    GtkWidget *label = gtk_bin_get_child(GTK_BIN(button));
    gtk_label_set_markup(GTK_LABEL(label), markup);
}

void update_all_buttons() {
    for (int i = 0; i < ROWS * COLS; i++) {
        update_button(i);
    }
}

void openCell(int row, int col) {
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) return;

    int id = row * COLS + col;
    if (isClicked[id] || user_field[id] == 'F') return;

    isClicked[id] = true;
    user_field[id] = field[id];

    update_button(id);

    if (field[id] == 0) {
        int dx[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
        int dy[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
        for (int k = 0; k < 8; k++) {
            int new_col = col + dx[k];
            int new_row = row + dy[k];
            openCell(new_row, new_col);
        }
    }
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
        if (field[i] == MINE) continue;
        int x = i % COLS, y = i / COLS;
        int dx[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
        int dy[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
        int count = 0;
        for (int k = 0; k < 8; k++) {
            int x1 = x + dx[k];
            int y1 = y + dy[k];
            if (x1 < 0 || x1 >= COLS || y1 < 0 || y1 >= ROWS) continue;
            if (field[y1 * COLS + x1] == MINE) count++;
        }
        field[i] = count;
    }
}

void create_grid() {
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 0);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 0);
    gtk_widget_set_hexpand(grid, FALSE);
    gtk_widget_set_vexpand(grid, FALSE);

    cell_buttons = malloc(ROWS * COLS * sizeof(GtkWidget *));

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            int id = i * COLS + j;
            GtkWidget *button = gtk_button_new_with_label(" ? ");
            gtk_widget_set_size_request(button, BUTTON_SIZE, BUTTON_SIZE);
            gtk_widget_set_hexpand(button, FALSE);
            gtk_widget_set_vexpand(button, FALSE);
            g_signal_connect(button, "clicked", G_CALLBACK(on_cell_clicked), GINT_TO_POINTER(id));
            g_signal_connect(button, "button-press-event", G_CALLBACK(on_cell_right_click), GINT_TO_POINTER(id));
            gtk_grid_attach(GTK_GRID(grid), button, j, i, 1, 1);
            cell_buttons[id] = button;
            GtkStyleContext *context = gtk_widget_get_style_context(button);
            gtk_style_context_add_class(context, "button-closed");
            update_button(id);
        }
    }
}

void on_restart(GtkWidget *widget, gpointer data) {
    const char *cols_text = gtk_entry_get_text(GTK_ENTRY(cols_edit));
    const char *rows_text = gtk_entry_get_text(GTK_ENTRY(rows_edit));
    const char *mines_text = gtk_entry_get_text(GTK_ENTRY(mines_edit));

    user_cols = atoi(cols_text);
    user_rows = atoi(rows_text);
    user_mines = atoi(mines_text);

    if (user_cols < 5) user_cols = 5;
    if (user_cols > 50) user_cols = 50;
    if (user_rows < 5) user_rows = 5;
    if (user_rows > 30) user_rows = 30;
    if (user_mines < 1) user_mines = 1;
    if (user_mines > user_cols * user_rows * 0.8) user_mines = (int)(user_cols * user_rows * 0.8);

    COLS = user_cols;
    ROWS = user_rows;
    MINES_COUNT = user_mines;

    gameover = 0;

    free(isClicked);
    free(field);
    free(user_field);
    free(cell_buttons);

    isClicked = calloc(ROWS * COLS, sizeof(bool));
    field = calloc(ROWS * COLS, sizeof(int));
    user_field = malloc(ROWS * COLS * sizeof(char));
    for (int i = 0; i < ROWS * COLS; i++) user_field[i] = '?';

    generate_field(field, MINES_COUNT);

    gtk_container_remove(GTK_CONTAINER(main_box), grid);

    create_grid();
    gtk_box_pack_start(GTK_BOX(main_box), grid, FALSE, FALSE, 0);
    gtk_widget_show_all(grid);

    // Force window to resize to exact dimensions
    gtk_window_resize(GTK_WINDOW(window), COLS * BUTTON_SIZE + 20, ROWS * BUTTON_SIZE + CONTROL_PANEL_HEIGHT + 40);
}

int main(int argc, char **argv) {
    srand(time(NULL));
    MINES_COUNT = (int)(ROWS * COLS / 6);
    user_mines = MINES_COUNT;

    colors[1] = RGB(32, 32, 32);

    isClicked = calloc(ROWS * COLS, sizeof(bool));
    field = calloc(ROWS * COLS, sizeof(int));
    user_field = malloc(ROWS * COLS * sizeof(char));
    for (int i = 0; i < ROWS * COLS; i++) user_field[i] = '?';

    generate_field(field, MINES_COUNT);

    gtk_init(&argc, &argv);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        ".button-closed { background: #373737; border: 1px solid #4D4D4D; padding: 0px; }"
        ".button-opened { background: #202020; border: 1px solid #4D4D4D; padding: 0px; }"
        ".button-mine { background: #FF0000; border: 1px solid #4D4D4D; padding: 0px; }"
        ".button-win { background: #58C858; border: 1px solid #4D4D4D; padding: 0px; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), WINDOW_TITLE);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_resize(GTK_WINDOW(window), COLS * BUTTON_SIZE + 20, ROWS * BUTTON_SIZE + CONTROL_PANEL_HEIGHT + 40);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(main_box, FALSE);
    gtk_widget_set_vexpand(main_box, FALSE);
    gtk_container_add(GTK_CONTAINER(window), main_box);

    top_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(main_box), top_box, FALSE, FALSE, 5);

    restart_button = gtk_button_new_with_label("Restart");
    g_signal_connect(restart_button, "clicked", G_CALLBACK(on_restart), NULL);
    gtk_box_pack_start(GTK_BOX(top_box), restart_button, FALSE, FALSE, 0);

    GtkWidget *width_label = gtk_label_new("Width:");
    gtk_box_pack_start(GTK_BOX(top_box), width_label, FALSE, FALSE, 0);
    cols_edit = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(cols_edit), "15");
    gtk_entry_set_width_chars(GTK_ENTRY(cols_edit), 4);
    gtk_box_pack_start(GTK_BOX(top_box), cols_edit, FALSE, FALSE, 0);

    GtkWidget *height_label = gtk_label_new("Height:");
    gtk_box_pack_start(GTK_BOX(top_box), height_label, FALSE, FALSE, 0);
    rows_edit = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(rows_edit), "8");
    gtk_entry_set_width_chars(GTK_ENTRY(rows_edit), 4);
    gtk_box_pack_start(GTK_BOX(top_box), rows_edit, FALSE, FALSE, 0);

    GtkWidget *mines_label = gtk_label_new("Mines:");
    gtk_box_pack_start(GTK_BOX(top_box), mines_label, FALSE, FALSE, 0);
    mines_edit = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(mines_edit), "20");
    gtk_entry_set_width_chars(GTK_ENTRY(mines_edit), 4);
    gtk_box_pack_start(GTK_BOX(top_box), mines_edit, FALSE, FALSE, 0);

    create_grid();
    gtk_box_pack_start(GTK_BOX(main_box), grid, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
    gtk_main();

    free(isClicked);
    free(field);
    free(user_field);
    free(cell_buttons);

    return 0;
}

