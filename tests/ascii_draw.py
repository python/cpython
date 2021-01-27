import tty, termios, sys, fcntl, os
from time import sleep
from ascii_buffer import AsciiBuffer
from types import SimpleNamespace

BLACK = 30
RED = 31
GREEN = 32
YELLOW = 33
BLUE = 34
MAGENTA = 35
CYAN = 36
WHITE = 37

colors = [
    WHITE,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN
]

def print_at(position, value):
    goto(position)
    print(value, end = '')
    sys.stdout.flush()

def goto(position):
    write('[%d;%df' % (position.y, position.x))

def write(value):
    print('\x1B' + value, end = '')
    sys.stdout.flush()

def clear_screen():
    write('\x1B[0m')
    write('\x1B[2J')
    write('\x1Bc')

def clean_up(old_settings):
    clear_screen()
    termios.tcsetattr(sys.stdin.fileno(), termios.TCSADRAIN, old_settings)
    print('\x1B[0m')
    
def get_input():
    data = ''
    while True:
        ch = sys.stdin.read(1)
        if ch == '':
            if len(data) > 0:
                break
            else:
                continue
        data = data + ch
    return data

def make_stdin_non_blocking():
    fl = fcntl.fcntl(sys.stdin.fileno(), fcntl.F_GETFL)
    fcntl.fcntl(sys.stdin.fileno(), fcntl.F_SETFL, fl | os.O_NONBLOCK)

def is_left_arrow(data):
    return data == '\x1B[D'

def is_right_arrow(data):
    return data == '\x1B[C'

def is_up_arrow(data):
    return data == '\x1B[A'

def is_down_arrow(data):
    return data == '\x1B[B'

def display_filename(filename):
    tsize = os.get_terminal_size()
    filename = filename or "[Untitled]"
    
    print_at(Position(1, tsize.lines), '\x1B[0K\x1B[47m\x1B[30m' + filename + '\x1B[0m')
    
def display_status(filename, drag_char, color):
    tsize = os.get_terminal_size()
    filename = filename or "[Untitled]"
    line = '\x1B[0K\x1B[37mFile=' + filename
    if drag_char:
        line += " drag=%s" % drag_char
    else:
        line += " drag=Off"
    line += "\x1B[0m color=\x1B[%dm█" % color
    line += "\x1B[0m"
    print_at(Position(1, tsize.lines), line)
    
def get_canvas_size():
    tsize = os.get_terminal_size()
    return tsize.columns, tsize.lines - 1

def Position(x, y):
    return SimpleNamespace(x = x, y = y)

def draw(cursor, color, buffer, data):
    data = '\x1B[%dm%s' % (color, data)
    print_at(cursor, data)
    buffer.set(cursor.x, cursor.y, data)
    goto(cursor)

def save_file(filename, cursor, buffer):
    tsize = os.get_terminal_size()
    if filename is None:
        filename = prompt_input("Save to file: ")
        
    file = open(filename, 'w')
    file.write(buffer.serialize() + '\x1B[0m\n')
    file.close()
    print_at(Position(1, tsize.lines), '\x1B[0K\x1B[47m\x1B[30mWrote ' + filename + '.\x1B[0m')
    sleep(0.5)
    return filename

def open_file(cursor, buffer):
    tsize = os.get_terminal_size()
    filename = prompt_input("Open file: ")
    file = open(filename, 'r')
    content = file.read()
    file.close()
    buffer.deserialize(content, tsize)
    print_at(Position(1, tsize.lines), '\x1B[0K\x1B[47m\x1B[30mLoaded ' + filename + '.\x1B[0m')
    sleep(0.5)
    return filename

def prompt_input(prompt):
    tsize = os.get_terminal_size()
    print_at(Position(1, tsize.lines), '\x1B[0K\x1B[47m\x1B[30m' + prompt)
    sys.stdout.flush()
    text = ''
    while True:
        data = get_input()
        if data == '\n' or data == '\r':
            break
        elif data == '\x7f':
            text = text[:-1]
            print_at(Position(1, tsize.lines), '\x1B[0K\x1B[47m\x1B[30m' + prompt + text)
            sys.stdout.flush()
        else:
            text += data
            print_at(Position(1, tsize.lines), '\x1B[0K\x1B[47m\x1B[30m' + prompt + text)
            sys.stdout.flush()
    return text

def main():
    log_file = open("log.txt", "w")
    log_file.write("start")
    last_char = None
    color_idx = 0
    color = colors[color_idx]
    buffer = AsciiBuffer()
    filename = None
    drag_char = None
    tsize = os.get_terminal_size()
    cursor = Position(x = tsize.columns // 2, y = tsize.lines // 2)
    old_settings = termios.tcgetattr(sys.stdin.fileno())
    tty.setraw(sys.stdin.fileno())
    make_stdin_non_blocking()
    clear_screen()
    display_status(filename, drag_char, color)
    goto(cursor)
    
    try:
        while True:
            data = get_input()
            log_file.write("Input: (" + repr(data) + ")\n")
            log_file.flush()
            if data == '\x1b': # ESC to quit
                break
            elif data == ' ': # SPACE to toggle drag mode
                if drag_char:
                    drag_char = None
                else:
                    drag_char = last_char
                display_status(filename, drag_char, color)
                goto(cursor)
            elif data == '\r': # ENTER to change colors
                color_idx += 1
                if color_idx >= len(colors):
                    color_idx = 0
                color = colors[color_idx]
                display_status(filename, drag_char, color)
                goto(cursor)
            elif is_up_arrow(data):
                cursor.y -= 1
                if cursor.y < 1:
                    cursor.y = 1
                goto(cursor)
                if drag_char:
                    draw(cursor, color, buffer, drag_char)
            elif is_down_arrow(data):
                cursor.y += 1
                w, h = get_canvas_size()
                if cursor.y > h:
                    cursor.y = h
                goto(cursor)
                if drag_char:
                    draw(cursor, color, buffer, drag_char)
            elif is_left_arrow(data):
                cursor.x -= 1
                if cursor.x < 1:
                    cursor.x = 1
                goto(cursor)
                if drag_char:
                    draw(cursor, color, buffer, drag_char)
            elif is_right_arrow(data):
                cursor.x += 1
                w, h = get_canvas_size()
                if cursor.x > w:
                    cursor.x = w
                goto(cursor)
                if drag_char:
                    draw(cursor, color, buffer, drag_char)
            elif data == '\x13': # Ctrl-S to save
                filename = save_file(filename, cursor, buffer)
                display_status(filename, drag_char, color)
                goto(cursor)
            elif data == '\x0f': # Ctrl-O to open
                filename = open_file(cursor, buffer)
                clear_screen()
                display_status(filename, drag_char, color)
                for i, line in enumerate(buffer.data):
                    for j, chr in enumerate(line):
                        if chr != ' ':
                            goto(Position(j + 1, i + 1))
                            print(chr, end = '')
                            sys.stdout.flush()
                goto(cursor)
            elif data == '\x7f': # DELETE
                draw(cursor, color, buffer, ' ')
            else:                # draw a character
                last_char = data
                draw(cursor, color, buffer, data)
                

    finally:
        clean_up(old_settings)

        
main()
    