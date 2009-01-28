"""Sample taken from: http://www.tkdocs.com/tutorial/morewidgets.html and
converted to Python, mainly to demonstrate xscrollcommand option.

grid [tk::listbox .l -yscrollcommand ".s set" -height 5] -column 0 -row 0 -sticky nwes
grid [ttk::scrollbar .s -command ".l yview" -orient vertical] -column 1 -row 0 -sticky ns
grid [ttk::label .stat -text "Status message here" -anchor w] -column 0 -row 1 -sticky we
grid [ttk::sizegrip .sz] -column 1 -row 1 -sticky se
grid columnconfigure . 0 -weight 1; grid rowconfigure . 0 -weight 1
for {set i 0} {$i<100} {incr i} {
   .l insert end "Line $i of 100"
   }
"""
import Tkinter
import ttk

root = Tkinter.Tk()

l = Tkinter.Listbox(height=5)
l.grid(column=0, row=0, sticky='nwes')

s = ttk.Scrollbar(command=l.yview, orient='vertical')
l['yscrollcommand'] = s.set
s.grid(column=1, row=0, sticky="ns")

stat = ttk.Label(text="Status message here", anchor='w')
stat.grid(column=0, row=1, sticky='we')

sz = ttk.Sizegrip()
sz.grid(column=1, row=1, sticky='se')

root.grid_columnconfigure(0, weight=1)
root.grid_rowconfigure(0, weight=1)

for i in range(100):
    l.insert('end', "Line %d of 100" % i)

root.mainloop()
