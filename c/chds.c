#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_DIR 512
#define ITEM_WIDTH 24  // 1項目あたりの幅（20文字 + 余白4文字）

typedef struct {
    char name[MAX_PATH];
    WCHAR display_name_w[MAX_PATH]; 
} DirectoryEntry;

DirectoryEntry dirs[MAX_DIR];
int dir_count = 0;
int current_selection = 0;
int previous_selection = 0;
int menu_start_row = 0;

// スクロール管理用変数
int scroll_top_row = 0; // 現在画面に表示されているリストの最上部行（0ベース）

void print_to_console_w(const WCHAR* wstr, int max_width, int is_selected) {
    HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
    UINT cp = GetConsoleOutputCP();

    int len = WideCharToMultiByte(cp, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* mbuf = (char*)malloc(len);
    if (mbuf == NULL) return;

    WideCharToMultiByte(cp, 0, wstr, -1, mbuf, len, NULL, NULL);

    char fmt[32];
    sprintf(fmt, "%%-%ds", max_width);

    if (is_selected) {
        SetConsoleTextAttribute(hConsole, BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
        fprintf(stderr, fmt, mbuf);
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        fprintf(stderr, "    ");
    } else {
        fprintf(stderr, fmt, mbuf);
        fprintf(stderr, "    ");
    }

    free(mbuf);
}

// ★修正ポイント: 画面クリアと同時に、コンソールのウィンドウ表示位置を強制的に最上部に固定する
void ClearWholeScreen() {
    HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
    COORD coordScreen = { 0, 0 };
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;

    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
    
    // 1. バッファ全体をスペースでクリア
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacterW(hConsole, L' ', dwConSize, coordScreen, &cCharsWritten);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
    
    // 2. カーソルを左上に戻す
    SetConsoleCursorPosition(hConsole, coordScreen);

    // 3. ★ここが最重要：コンソールの表示ウィンドウの原点を強制的に (0, 0) にスクロールさせる
    // これにより、ヘッダーが画面外（上部）に置いてきぼりになる現象を100%防ぎます
    SMALL_RECT windowRect;
    int windowWidth = csbi.srWindow.Right - csbi.srWindow.Left;
    int windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top;
    
    windowRect.Left = 0;
    windowRect.Top = 0;
    windowRect.Right = windowWidth;
    windowRect.Bottom = windowHeight;
    
    SetConsoleWindowInfo(hConsole, TRUE, &windowRect);
}

void truncate_name_w(const WCHAR* src, WCHAR* dest, int max_width) {
    UINT cp = GetConsoleOutputCP();
    int cur_width = 0;
    int i = 0;
    
    while (src[i] != L'\0') {
        int char_width = WideCharToMultiByte(cp, 0, &src[i], 1, NULL, 0, NULL, NULL);
        if (cur_width + char_width > max_width) break;

        dest[i] = src[i];
        cur_width += char_width;
        i++;
    }
    dest[i] = L'\0';
}

void register_directories(const char* target_to_focus) {
    dir_count = 0;
    current_selection = 0; 
    previous_selection = 0;
    scroll_top_row = 0;
    
    strncpy(dirs[dir_count].name, ".", MAX_PATH);
    wcscpy(dirs[dir_count].display_name_w, L".");
    dir_count++;

    WIN32_FIND_DATAW find_data_w;
    HANDLE hFind = FindFirstFileW(L"*", &find_data_w);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if ((find_data_w.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            wcscmp(find_data_w.cFileName, L".") != 0) {
            
            if (dir_count < MAX_DIR) {
                WideCharToMultiByte(GetConsoleOutputCP(), 0, find_data_w.cFileName, -1, dirs[dir_count].name, MAX_PATH, NULL, NULL);
                truncate_name_w(find_data_w.cFileName, dirs[dir_count].display_name_w, 20);

                if (target_to_focus != NULL && strcmp(dirs[dir_count].name, target_to_focus) == 0) {
                    current_selection = dir_count;
                    previous_selection = dir_count;
                }

                dir_count++;
            }
        }
    } while (FindNextFileW(hFind, &find_data_w));

    FindClose(hFind);
}

void draw_single_item(int index, int is_selected) {
    HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    // 表示ウィンドウ自体の高さを基準にする
    int visible_window_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    int console_width = csbi.dwSize.X;
    int cols = console_width / ITEM_WIDTH;
    if (cols < 1) cols = 1;

    int item_row = index / cols;
    int item_col = index % cols;

    int final_row = menu_start_row + (item_row - scroll_top_row);

    // ウィンドウの最下行には絶対に描画しない（スクロール暴走防止マージン）
    if (final_row < menu_start_row || final_row >= visible_window_height - 1) {
        return;
    }

    COORD target_pos;
    target_pos.X = item_col * ITEM_WIDTH;
    target_pos.Y = final_row;
    SetConsoleCursorPosition(hConsole, target_pos);

    print_to_console_w(dirs[index].display_name_w, 20, is_selected);

    if (is_selected) {
        SetConsoleCursorPosition(hConsole, target_pos);
    }
}

void clear_list_area() {
    HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;

    int visible_window_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    DWORD written;
    COORD start_coord = {0, menu_start_row};
    int total_chars = csbi.dwSize.X * (visible_window_height - 1 - menu_start_row);
    if (total_chars < 0) total_chars = 0;

    FillConsoleOutputCharacterW(hConsole, L' ', total_chars, start_coord, &written);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, total_chars, start_coord, &written);
}

void draw_list_only() {
    clear_list_area();
    for (int i = 0; i < dir_count; i++) {
        draw_single_item(i, (i == current_selection));
    }
}

void draw_full_menu() {
    ClearWholeScreen();
    
    HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    
    WCHAR header[512];
    WCHAR current_path_w[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, current_path_w);
    swprintf(header, 512, 
        L"==================================================\n"
        L" 現在のパス: %s\n"
        L" [各フォルダ]: Enterで中へ  [.]: Enterでここを確定\n"
        L"==================================================\n", current_path_w);
    
    DWORD written;
    WriteConsoleW(hConsole, header, wcslen(header), &written, NULL);

    GetConsoleScreenBufferInfo(hConsole, &csbi);
    menu_start_row = csbi.dwCursorPosition.Y;

    int cols = csbi.dwSize.X / ITEM_WIDTH;
    if (cols < 1) cols = 1;
    
    int visible_window_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    int visible_rows = visible_window_height - 1 - menu_start_row;
    if (visible_rows < 1) visible_rows = 1;

    int target_row = current_selection / cols;
    if (target_row >= visible_rows) {
        scroll_top_row = target_row - visible_rows + 1;
    }

    draw_list_only();
}

void update_cursor_position() {
    HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    int cols = csbi.dwSize.X / ITEM_WIDTH;
    if (cols < 1) cols = 1;

    int curr_row = current_selection / cols;
    
    int visible_window_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    int visible_rows = visible_window_height - 1 - menu_start_row;
    if (visible_rows < 1) visible_rows = 1;

    int need_refresh = 0;
    if (curr_row < scroll_top_row) {
        scroll_top_row = curr_row;
        need_refresh = 1;
    } else if (curr_row >= scroll_top_row + visible_rows) {
        scroll_top_row = curr_row - visible_rows + 1;
        need_refresh = 1;
    }

    if (need_refresh) {
        draw_list_only();
    } else {
        if (previous_selection != current_selection) {
            draw_single_item(previous_selection, 0);
        }
        draw_single_item(current_selection, 1);
    }
    previous_selection = current_selection;
}

int main() {
    UINT current_cp = GetConsoleOutputCP();
    SetConsoleOutputCP(current_cp);

    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD prev_mode;
    GetConsoleMode(hInput, &prev_mode);
    SetConsoleMode(hInput, prev_mode & ~ENABLE_QUICK_EDIT_MODE);

    register_directories(NULL);
    draw_full_menu();

    INPUT_RECORD ir;
    DWORD read;
    char target_path[MAX_PATH] = "";
    int break_loop = 0;

    while (!break_loop) {
        ReadConsoleInput(hInput, &ir, 1, &read);

        if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown) {
            WORD wKey = ir.Event.KeyEvent.wVirtualKeyCode;

            HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(hConsole, &csbi);
            int cols = csbi.dwSize.X / ITEM_WIDTH;
            if (cols < 1) cols = 1;

            if (wKey == VK_UP) {
                if (current_selection - cols >= 0) {
                    current_selection -= cols;
                    update_cursor_position();
                }
            } 
            else if (wKey == VK_DOWN) {
                if (current_selection + cols < dir_count) {
                    current_selection += cols;
                    update_cursor_position();
                }
            }
            else if (wKey == VK_LEFT) {
                if (current_selection > 0) {
                    current_selection--;
                    update_cursor_position();
                }
            }
            else if (wKey == VK_RIGHT) {
                if (current_selection < dir_count - 1) {
                    current_selection++;
                    update_cursor_position();
                }
            }
            else if (wKey == VK_RETURN) {
                if (dir_count > 0) {
                    if (strcmp(dirs[current_selection].name, ".") == 0) {
                        GetCurrentDirectory(MAX_PATH, target_path);
                        break_loop = 1;
                    } 
                    else {
                        char last_dir_name[MAX_PATH] = "";
                        
                        if (strcmp(dirs[current_selection].name, "..") == 0) {
                            char full_path_before[MAX_PATH];
                            GetCurrentDirectory(MAX_PATH, full_path_before);
                            char* last_slash = strrchr(full_path_before, '\\');
                            if (last_slash != NULL) {
                                strcpy(last_dir_name, last_slash + 1);
                            }
                        }

                        if (SetCurrentDirectory(dirs[current_selection].name)) {
                            register_directories(strlen(last_dir_name) > 0 ? last_dir_name : NULL);
                            draw_full_menu();
                        }
                    }
                }
            } 
            else if (wKey == VK_BACK) {
                char last_dir_name[MAX_PATH] = "";
                char full_path_before[MAX_PATH];
                GetCurrentDirectory(MAX_PATH, full_path_before);
                char* last_slash = strrchr(full_path_before, '\\');
                if (last_slash != NULL) {
                    strcpy(last_dir_name, last_slash + 1);
                }

                if (SetCurrentDirectory("..")) {
                    register_directories(last_dir_name);
                    draw_full_menu();
                }
            } 
            else if (wKey == VK_ESCAPE) {
                break_loop = 1;
            }
        }
    }

    SetConsoleMode(hInput, prev_mode);
    HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    ClearWholeScreen();

    if (strlen(target_path) > 0) {
        printf("%s", target_path);
    }

    return 0;
}
