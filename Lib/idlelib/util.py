def fix_scaling(root):
    import tkinter.font
    for name in tkinter.font.names(root):
        font = tkinter.font.Font(root=root, name=name, exists=True)
        font['size'] = abs(font['size'])
