import os
import sys
import curses

MAX_DIR = 512
ITEM_WIDTH = 24  # 1項目あたりの幅

def get_directories(target_to_focus=None):
    """ディレクトリ一覧を取得し、初期の選択インデックスを返す"""
    dirs = ["."]
    try:
        # カレントディレクトリ内のフォルダを取得（隠し属性などは簡易的に除外）
        for entry in os.scandir("."):
            if entry.is_dir():
                dirs.append(entry.name)
    except Exception:
        pass
    
    # MAX_DIR制限
    dirs = dirs[:MAX_DIR]
    
    # フォーカス対象のインデックスを探す
    current_selection = 0
    if target_to_focus and target_to_focus in dirs:
        current_selection = dirs.index(target_to_focus)
        
    return dirs, current_selection

def draw_menu(stdscr, dirs, current_selection, scroll_top_row):
    # 画面全体のサイズを取得
    screen_height, screen_width = stdscr.getmaxyx()
    
    # 画面を一度クリア
    stdscr.erase()
    
    # 1. ヘッダー情報の描画（4行固定）
    current_path = os.getcwd()
    stdscr.addstr(0, 0, "=" * min(50, screen_width - 1))
    stdscr.addstr(1, 0, f" 現在のパス: {current_path}"[:screen_width - 1])
    stdscr.addstr(2, 0, " [各フォルダ]: Enterで中へ  [.]: Enterでここを確定"[:screen_width - 1])
    stdscr.addstr(3, 0, "=" * min(50, screen_width - 1))
    
    # ファイルリストの描画開始行
    menu_start_row = 4
    
    # 2. リスト部分の描画計算
    cols = screen_width // ITEM_WIDTH
    if cols < 1:
        cols = 1
        
    visible_rows = screen_height - 1 - menu_start_row  # 最下行の1行手前まで
    if visible_rows < 1:
        visible_rows = 1

    # スクロール位置の自動調整（はみ出したら追従）
    curr_row = current_selection // cols
    if curr_row < scroll_top_row:
        scroll_top_row = curr_row
    elif curr_row >= scroll_top_row + visible_rows:
        scroll_top_row = curr_row - visible_rows + 1

    # 3. ディレクトリの配置
    for i, dname in enumerate(dirs):
        item_row = i // cols
        item_col = i % cols
        
        # スクロール窓の範囲内にあるか判定
        final_row = menu_start_row + (item_row - scroll_top_row)
        if menu_start_row <= final_row < (screen_height - 1):
            x_pos = item_col * ITEM_WIDTH
            
            # 表示名の切り詰め（全角半角混在は簡易的に文字数でカット）
            display_name = dname[:20].ljust(20)
            
            if i == current_selection:
                # 選択中のハイライト表示（反転）
                stdscr.attron(curses.A_REVERSE)
                stdscr.addstr(final_row, x_pos, display_name)
                stdscr.attroff(curses.A_REVERSE)
                # カーソルを選択中の先頭に配置して点滅させる
                stdscr.move(final_row, x_pos)
            else:
                stdscr.addstr(final_row, x_pos, display_name)
                
    stdscr.refresh()
    return scroll_top_row

def main(stdscr):
    # カーソルを表示する
    curses.curs_set(1)
    # キー入力を即座に反映させる
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
            
        # 画面サイズを取得して列数を計算
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
                
        elif key in (10, 13, curses.KEY_ENTER):  # Enterキー
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
                        
        elif key == curses.KEY_BACKSPACE or key == 8:  # Backspace
            last_dir_name = os.path.basename(os.getcwd())
            try:
                os.chdir("..")
                dirs, current_selection = get_directories(last_dir_name)
                scroll_top_row = 0
            except Exception:
                pass
                
        elif key == 27:  # ESCキー
            break
            
        # 画面の再描画
        scroll_top_row = draw_menu(stdscr, dirs, current_selection, scroll_top_row)
        
    # 終了時に確定したパスがあれば標準出力に書き出す
    return target_path

if __name__ == "__main__":
    # cursesの初期化と後処理を自動で包んでくれる wrapper を使用
    selected_path = curses.wrapper(main)
    
    if selected_path:
        print(selected_path)
