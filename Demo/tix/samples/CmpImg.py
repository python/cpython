# -*-mode: python; fill-column: 75; tab-width: 8; coding: iso-latin-1-unix -*-
#
# $Id$
#
# Tix Demonstration Program
#
# This sample program is structured in such a way so that it can be
# executed from the Tix demo program "tixwidgets.py": it must have a
# procedure called "RunSample". It should also have the "if" statment
# at the end of this file so that it can be run as a standalone
# program.

# This file demonstrates the use of the compound images: it uses compound
# images to display a text string together with a pixmap inside
# buttons
#

import Tix

network_pixmap = """/* XPM */
static char * netw_xpm[] = {
/* width height ncolors chars_per_pixel */
"32 32 7 1",
/* colors */
"       s None  c None",
".      c #000000000000",
"X      c white",
"o      c #c000c000c000",
"O      c #404040",
"+      c blue",
"@      c red",
/* pixels */
"                                ",
"                 .............. ",
"                 .XXXXXXXXXXXX. ",
"                 .XooooooooooO. ",
"                 .Xo.......XoO. ",
"                 .Xo.++++o+XoO. ",
"                 .Xo.++++o+XoO. ",
"                 .Xo.++oo++XoO. ",
"                 .Xo.++++++XoO. ",
"                 .Xo.+o++++XoO. ",
"                 .Xo.++++++XoO. ",
"                 .Xo.XXXXXXXoO. ",
"                 .XooooooooooO. ",
"                 .Xo@ooo....oO. ",
" ..............  .XooooooooooO. ",
" .XXXXXXXXXXXX.  .XooooooooooO. ",
" .XooooooooooO.  .OOOOOOOOOOOO. ",
" .Xo.......XoO.  .............. ",
" .Xo.++++o+XoO.        @        ",
" .Xo.++++o+XoO.        @        ",
" .Xo.++oo++XoO.        @        ",
" .Xo.++++++XoO.        @        ",
" .Xo.+o++++XoO.        @        ",
" .Xo.++++++XoO.      .....      ",
" .Xo.XXXXXXXoO.      .XXX.      ",
" .XooooooooooO.@@@@@@.X O.      ",
" .Xo@ooo....oO.      .OOO.      ",
" .XooooooooooO.      .....      ",
" .XooooooooooO.                 ",
" .OOOOOOOOOOOO.                 ",
" ..............                 ",
"                                "};
"""

hard_disk_pixmap = """/* XPM */
static char * drivea_xpm[] = {
/* width height ncolors chars_per_pixel */
"32 32 5 1",
/* colors */
"       s None  c None",
".      c #000000000000",
"X      c white",
"o      c #c000c000c000",
"O      c #800080008000",
/* pixels */
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"   ..........................   ",
"   .XXXXXXXXXXXXXXXXXXXXXXXo.   ",
"   .XooooooooooooooooooooooO.   ",
"   .Xooooooooooooooooo..oooO.   ",
"   .Xooooooooooooooooo..oooO.   ",
"   .XooooooooooooooooooooooO.   ",
"   .Xoooooooo.......oooooooO.   ",
"   .Xoo...................oO.   ",
"   .Xoooooooo.......oooooooO.   ",
"   .XooooooooooooooooooooooO.   ",
"   .XooooooooooooooooooooooO.   ",
"   .XooooooooooooooooooooooO.   ",
"   .XooooooooooooooooooooooO.   ",
"   .oOOOOOOOOOOOOOOOOOOOOOOO.   ",
"   ..........................   ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                "};
"""

network_bitmap = """
#define netw_width 32
#define netw_height 32
static unsigned char netw_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x7f, 0x00, 0x00, 0x02, 0x40,
   0x00, 0x00, 0xfa, 0x5f, 0x00, 0x00, 0x0a, 0x50, 0x00, 0x00, 0x0a, 0x52,
   0x00, 0x00, 0x0a, 0x52, 0x00, 0x00, 0x8a, 0x51, 0x00, 0x00, 0x0a, 0x50,
   0x00, 0x00, 0x4a, 0x50, 0x00, 0x00, 0x0a, 0x50, 0x00, 0x00, 0x0a, 0x50,
   0x00, 0x00, 0xfa, 0x5f, 0x00, 0x00, 0x02, 0x40, 0xfe, 0x7f, 0x52, 0x55,
   0x02, 0x40, 0xaa, 0x6a, 0xfa, 0x5f, 0xfe, 0x7f, 0x0a, 0x50, 0xfe, 0x7f,
   0x0a, 0x52, 0x80, 0x00, 0x0a, 0x52, 0x80, 0x00, 0x8a, 0x51, 0x80, 0x00,
   0x0a, 0x50, 0x80, 0x00, 0x4a, 0x50, 0x80, 0x00, 0x0a, 0x50, 0xe0, 0x03,
   0x0a, 0x50, 0x20, 0x02, 0xfa, 0xdf, 0x3f, 0x03, 0x02, 0x40, 0xa0, 0x02,
   0x52, 0x55, 0xe0, 0x03, 0xaa, 0x6a, 0x00, 0x00, 0xfe, 0x7f, 0x00, 0x00,
   0xfe, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
"""

hard_disk_bitmap = """
#define drivea_width 32
#define drivea_height 32
static unsigned char drivea_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xf8, 0xff, 0xff, 0x1f, 0x08, 0x00, 0x00, 0x18, 0xa8, 0xaa, 0xaa, 0x1a,
   0x48, 0x55, 0xd5, 0x1d, 0xa8, 0xaa, 0xaa, 0x1b, 0x48, 0x55, 0x55, 0x1d,
   0xa8, 0xfa, 0xaf, 0x1a, 0xc8, 0xff, 0xff, 0x1d, 0xa8, 0xfa, 0xaf, 0x1a,
   0x48, 0x55, 0x55, 0x1d, 0xa8, 0xaa, 0xaa, 0x1a, 0x48, 0x55, 0x55, 0x1d,
   0xa8, 0xaa, 0xaa, 0x1a, 0xf8, 0xff, 0xff, 0x1f, 0xf8, 0xff, 0xff, 0x1f,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
"""

def RunSample(w):
    w.img0 = Tix.Image('pixmap', data=network_pixmap)
    if not w.img0:
        w.img0 = Tix.Image('bitmap', data=network_bitmap)
    w.img1 = Tix.Image('pixmap', data=hard_disk_pixmap)
    if not w.img0:
        w.img1 = Tix.Image('bitmap', data=hard_disk_bitmap)

    hdd = Tix.Button(w, padx=4, pady=1, width=120)
    net = Tix.Button(w, padx=4, pady=1, width=120)

    # Create the first image: we create a line, then put a string,
    # a space and an image into this line, from left to right.
    # The result: we have a one-line image that consists of three
    # individual items
    #
    # The tk.calls should be methods in Tix ...
    w.hdd_img = Tix.Image('compound', window=hdd)
    w.hdd_img.tk.call(str(w.hdd_img), 'add', 'line')
    w.hdd_img.tk.call(str(w.hdd_img), 'add', 'text', '-text', 'Hard Disk',
                    '-underline', '0')
    w.hdd_img.tk.call(str(w.hdd_img), 'add', 'space', '-width', '7')
    w.hdd_img.tk.call(str(w.hdd_img), 'add', 'image', '-image', w.img1)

    # Put this image into the first button
    #
    hdd['image'] = w.hdd_img

    # Next button
    w.net_img = Tix.Image('compound', window=net)
    w.net_img.tk.call(str(w.net_img), 'add', 'line')
    w.net_img.tk.call(str(w.net_img), 'add', 'text', '-text', 'Network',
                    '-underline', '0')
    w.net_img.tk.call(str(w.net_img), 'add', 'space', '-width', '7')
    w.net_img.tk.call(str(w.net_img), 'add', 'image', '-image', w.img0)

    # Put this image into the first button
    #
    net['image'] = w.net_img

    close = Tix.Button(w, pady=1, text='Close',
                       command=lambda w=w: w.destroy())

    hdd.pack(side=Tix.LEFT, padx=10, pady=10, fill=Tix.Y, expand=1)
    net.pack(side=Tix.LEFT, padx=10, pady=10, fill=Tix.Y, expand=1)
    close.pack(side=Tix.LEFT, padx=10, pady=10, fill=Tix.Y, expand=1)

if __name__ == '__main__':
    root = Tix.Tk()
    RunSample(root)
    root.mainloop()
