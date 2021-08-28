import tkinter as tk

# create window
window = tk.Tk()
window.geometry('500x500')
window['bg'] = '#cce'
window.resizable(height=False, width=False)
window.wm_attributes('-alpha', 0.95)
# label
label = tk.Label(text='Change theme', font='Consolas 20')


# funcs
def white():
    window['bg'] = '#000'
    btn_black['bg'] = 'white'
    btn_black['fg'] = 'black'
    btn_white['bg'] = 'black'
    btn_white['fg'] = 'white'
    btn_white["relief"] = "solid"


def black():
    window['bg'] = 'white'
    btn_black['bg'] = 'white'
    btn_black['fg'] = 'black'
    btn_white['bg'] = 'black'
    btn_white['fg'] = 'white'


# buttons
label.pack()
btn_black = tk.Button(text='Black Theme', command=white)
btn_black["bg"] = "grey"
btn_black["border"] = "0"
btn_black['font'] = 'Arial 13'
btn_black['fg'] = '#eee'
btn_black.place(x=20, y=80, width=200, height=60)

btn_white = tk.Button(text='White Theme', command=black)
btn_white["bg"] = "grey"
btn_white["border"] = "0"
btn_white['font'] = 'Arial 13'
btn_white['fg'] = '#eee'
btn_white.place(x=280, y=80, width=200, height=60)

window.mainloop()
