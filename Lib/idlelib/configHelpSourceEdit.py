"""
Dialog that allows user to specify or edit the parameters for a user configured
help source.
"""
from Tkinter import *
import tkMessageBox
import os

class GetHelpSourceDialog(Toplevel):
    def __init__(self,parent,title,menuItem='',filePath=''):
        """
        menuItem - string, the menu item to edit, if any
        filePath - string, the help file path to edit, if any
        """
        Toplevel.__init__(self, parent)
        self.configure(borderwidth=5)
        self.resizable(height=FALSE,width=FALSE)
        self.title(title)
        self.transient(parent)
        self.grab_set()
        self.protocol("WM_DELETE_WINDOW", self.Cancel)
        self.parent = parent
        self.result=None
        self.CreateWidgets()
        self.withdraw() #hide while setting geometry
        self.update_idletasks()
        #needs to be done here so that the winfo_reqwidth is valid
        self.geometry("+%d+%d" % 
            ((parent.winfo_rootx()+((parent.winfo_width()/2)
                -(self.winfo_reqwidth()/2)),
              parent.winfo_rooty()+((parent.winfo_height()/2)
                -(self.winfo_reqheight()/2)) )) ) #centre dialog over parent
        self.deiconify() #geometry set, unhide
        self.wait_window()

    def CreateWidgets(self):
        self.menu=StringVar(self)
        self.path=StringVar(self)
        self.fontSize=StringVar(self)
        self.frameMain = Frame(self,borderwidth=2,relief=SUNKEN)
        self.frameMain.pack(side=TOP,expand=TRUE,fill=BOTH)
        labelMenu=Label(self.frameMain,anchor=W,justify=LEFT,
                text='Menu Item:')
        self.entryMenu=Entry(self.frameMain,textvariable=self.menu,width=30)
        self.entryMenu.focus_set()
        labelPath=Label(self.frameMain,anchor=W,justify=LEFT,
                text='Help File Path:')
        self.entryPath=Entry(self.frameMain,textvariable=self.path,width=40)
        self.entryMenu.focus_set()
        labelMenu.pack(anchor=W,padx=5,pady=3)
        self.entryMenu.pack(anchor=W,padx=5,pady=3)
        labelPath.pack(anchor=W,padx=5,pady=3)
        self.entryPath.pack(anchor=W,padx=5,pady=3)
        frameButtons=Frame(self)
        frameButtons.pack(side=BOTTOM,fill=X)
        self.buttonOk = Button(frameButtons,text='Ok',
                width=8,command=self.Ok)
        self.buttonOk.grid(row=0,column=0,padx=5,pady=5)
        self.buttonCancel = Button(frameButtons,text='Cancel',
                width=8,command=self.Cancel)
        self.buttonCancel.grid(row=0,column=1,padx=5,pady=5)

    def MenuOk(self):
        #simple validity check for a sensible 
        #menu item name
        menuOk=1
        menu=self.menu.get()
        menu.strip()
        if not menu: #no menu item specified
            tkMessageBox.showerror(title='Menu Item Error',
                    message='No menu item specified.')
            self.entryMenu.focus_set()
            menuOk=0
        elif len(menu)>30: #menu item name too long
            tkMessageBox.showerror(title='Menu Item Error',
                    message='Menu item too long. It should be no more '+
                    'than 30 characters.')
            self.entryMenu.focus_set()
            menuOk=0
        return menuOk
    
    def PathOk(self):
        #simple validity check for menu file path 
        pathOk=1
        path=self.path.get()
        path.strip()
        if not path: #no path specified
            tkMessageBox.showerror(title='File Path Error',
                    message='No help file path specified.')
            self.entryPath.focus_set()
            pathOk=0
        elif not os.path.exists(path):
            tkMessageBox.showerror(title='File Path Error',
                    message='Help file path does not exist.')
            self.entryPath.focus_set()
            pathOk=0
        return pathOk
            
    def Ok(self, event=None):
        if self.MenuOk():
            if self.PathOk():
                self.result=( self.menu.get().strip(),self.path.get().strip() ) 
                self.destroy()
        
    def Cancel(self, event=None):
        self.result=None
        self.destroy()

if __name__ == '__main__':
    #test the dialog
    root=Tk()
    def run():
        keySeq=''
        dlg=GetHelpSourceDialog(root,'Get Help Source')
        print dlg.result
    Button(root,text='Dialog',command=run).pack()
    root.mainloop()
    
 
