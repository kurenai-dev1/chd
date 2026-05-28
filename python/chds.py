import os
import sys
import curses
import unicodedata

MAX_DIR = 512
ITEM_WIDTH = 24  # 1項目あたりの幅

def get_char_width(char):
    """1文字の正確な表示幅を返す (全角=2, 半角=1)"""
    status = unicodedata.east_asian_width(char)
    if status in ('F', 'W', 'A'):
        return 2
    if len(char.encode('utf-8', errors='ignore')) > 1:
        return 2
    return 1

def get_string_width(s):
    """文字列全体の表示幅を計算する"""
    return sum(get_char_width(char) for char in s)

def truncate_string_by_width(s, max_width):
    """指定された表示幅に収まるように文字列を切り詰める"""
    cur_width = 0
    res = []
    for char in s:
        w = get_char_width(char)
        if cur_width + w > max_width:
            break
        res.append(char)
        cur_width += w
    return "".join(res)

def add_str_with_width_fix(stdscr, y, x, text, max_width, attr=curses.A_NORMAL):
    """Windows cursesの文字重なりを物理的に防止する描画関数"""
    stdscr.move(y, x)
    cur_x = x
    
    for char in text:
        w = get_char_width(char)
        if cur_x + w > x + max_width:
            break
            
        try:
            stdscr.addstr(y, cur_x, char, attr)
            cur_x += w
        except curses.error:
            break

def get_directories(target_to_focus=None):
    """★修正ポイント: 親ディレクトリ「..」の登録ロジックを復活"""
    dirs = ["."]
    
    # 現在のパスを取得
    current_path = os.getcwd()
    # ドライブのルート（例: C:\ や D:\）でなければ、".." をリストの2番目に追加
    if os.path.dirname(current_path) != current_path:
        dirs.append("..")
        
    try:
        for entry in os.scandir("."):
            if entry.is_dir():
                dirs.append(entry.name)
    except Exception:
        pass
    
    dirs = dirs[:MAX_DIR]
    
    current_selection = 0
    if target_to_focus and target_to_focus in dirs:
        current_selection = dirs.index(target_to_focus)
        
    return dirs, current_selection

def draw_menu(stdscr, dirs, current_selection, scroll_top_row):
    screen_height, screen_width = stdscr.getmaxyx()
    stdscr.erase()
    
    max_line_width = screen_width - 1
    
    # 1. ヘッダー情報の描画
    stdscr.addstr(0, 0, "=" * min(50, max_line_width))
    
    path_prefix = " 現在のパス: "
    current_path = os.getcwd()
    available_path_width = max_line_width - get_string_width(path_prefix)
    truncated_path = truncate_string_by_width(current_path, available_path_width)
    
    stdscr.move(1, 0)
    stdscr.clrtoeol()
    add_str_with_width_fix(stdscr, 1, 0, f"{path_prefix}{truncated_path}", max_line_width)
    
    stdscr.move(2, 0)
    stdscr.clrtoeol()
    guide_text = " [各フォルダ]: Enterで中へ  [.]: Enterでここを確定"
    add_str_with_width_fix(stdscr, 2, 0, truncate_string_by_width(guide_text, max_line_width), max_line_width)
    
    stdscr.addstr(3, 0, "=" * min(50, max_line_width))
    
    menu_start_row = 4
    
    # 2. リスト部分の計算
    cols = screen_width // ITEM_WIDTH
    if cols < 1:
        cols = 1
        
    visible_rows = screen_height - 1 - menu_start_row
    if visible_rows < 1:
        visible_rows = 1

    curr_row = current_selection // cols
    if curr_row < scroll_top_row:
        scroll_top_row = curr_row
    elif curr_row >= scroll_top_row + visible_rows:
        scroll_top_row = curr_row - visible_rows + 1

    # 3. ディレクトリの配置
    for i, dname in enumerate(dirs):
        item_row = i // cols
        item_col = i % cols
        
        final_row = menu_start_row + (item_row - scroll_top_row)
        if menu_start_row <= final_row < (screen_height - 1):
            x_pos = item_col * ITEM_WIDTH
            
            truncated_dname = truncate_string_by_width(dname, 20)
            padding_len = 20 - get_string_width(truncated_dname)
            display_name = f"{truncated_dname}{' ' * padding_len}"
            
            attr = curses.A_REVERSE if i == current_selection else curses.A_NORMAL
            add_str_with_width_fix(stdscr, final_row, x_pos, display_name, 20, attr)
            
            if i == current_selection:
                stdscr.move(final_row, x_pos)
                
    stdscr.refresh()
    return scroll_top_row

def main(stdscr):
    curses.curs_set(1)
    stdscr.keypad(True)
    curses.cbreak()
    
    target_path = ""
    scroll_top_row = 0
    
    dirs, current_selection = get_directories()
    scroll_top_row = draw_menu(stdscr, dirs, current_selection, scroll_top_row)
    
    while True:
        try:
            key = stdscr.getch()
        except KeyboardInterrupt:
            break
            
        _, screen_width = stdscr.getmaxyx()
        cols = screen_width // ITEM_WIDTH
        if cols < 1:
            cols = 1
            
        if key == curses.KEY_UP:
            if current_selection - cols >= 0:
                current_selection -= cols
                
        elif key == curses.KEY_DOWN:
            if current_selection + cols < len(dirs):
                current_selection += cols
                
        elif key == curses.KEY_LEFT:
            if current_selection > 0:
                current_selection -= 1
                
        elif key == curses.KEY_RIGHT:
            if current_selection < len(dirs) - 1:
                current_selection += 1
                
        elif key in (10, 13, curses.KEY_ENTER):
            if len(dirs) > 0:
                sel_name = dirs[current_selection]
                if sel_name == ".":
                    target_path = os.getcwd()
                    break
                else:
                    last_dir_name = ""
                    if sel_name == "..":
                        last_dir_name = os.path.basename(os.getcwd())
                        
                    try:
                        os.chdir(sel_name)
                        dirs, current_selection = get_directories(last_dir_name if last_dir_name else None)
                        scroll_top_row = 0
                    except Exception:
                        pass
                        
        elif key == curses.KEY_BACKSPACE or key == 8:
            last_dir_name = os.path.basename(os.getcwd())
            try:
                os.chdir("..")
                dirs, current_selection = get_directories(last_dir_name)
                scroll_top_row = 0
            except Exception:
                pass
                
        elif key == 27:
            break
            
        scroll_top_row = draw_menu(stdscr, dirs, current_selection, scroll_top_row)
        
    return target_path

if __name__ == "__main__":
    selected_path = curses.wrapper(main)
    if selected_path:
        print(selected_path)
