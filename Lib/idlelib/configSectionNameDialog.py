"""
Dialog that allows user to specify a new config file section name.
Used to get new highlight theme and keybinding set names.
"""
from tkinter import *
import tkinter.messagebox as tkMessageBox

class GetCfgSectionNameDialog(Toplevel):
    def __init__(self,parent,title,message,usedNames):
        """
        message - string, informational message to display
        usedNames - list, list of names already in use for validity check
        """
        Toplevel.__init__(self, parent)
        self.configure(borderwidth=5)
        self.resizable(height=FALSE,width=FALSE)
        self.title(title)
        self.transient(parent)
        self.grab_set()
        self.protocol("WM_DELETE_WINDOW", self.Cancel)
        self.parent = parent
        self.message=message
        self.usedNames=usedNames
        self.result=''
        self.CreateWidgets()
        self.withdraw() #hide while setting geometry
        self.update_idletasks()
        #needs to be done here so that the winfo_reqwidth is valid
        self.messageInfo.config(width=self.frameMain.winfo_reqwidth())
        self.geometry("+%d+%d" %
            ((parent.winfo_rootx()+((parent.winfo_width()/2)
                -(self.winfo_reqwidth()/2)),
              parent.winfo_rooty()+((parent.winfo_height()/2)
                -(self.winfo_reqheight()/2)) )) ) #centre dialog over parent
        self.deiconify() #geometry set, unhide
        self.wait_window()

    def CreateWidgets(self):
        self.name=StringVar(self)
        self.fontSize=StringVar(self)
        self.frameMain = Frame(self,borderwidth=2,relief=SUNKEN)
        self.frameMain.pack(side=TOP,expand=TRUE,fill=BOTH)
        self.messageInfo=Message(self.frameMain,anchor=W,justify=LEFT,padx=5,pady=5,
                text=self.message)#,aspect=200)
        entryName=Entry(self.frameMain,textvariable=self.name,width=30)
        entryName.focus_set()
        self.messageInfo.pack(padx=5,pady=5)#,expand=TRUE,fill=BOTH)
        entryName.pack(padx=5,pady=5)
        frameButtons=Frame(self)
        frameButtons.pack(side=BOTTOM,fill=X)
        self.buttonOk = Button(frameButtons,text='Ok',
                width=8,command=self.Ok)
        self.buttonOk.grid(row=0,column=0,padx=5,pady=5)
        self.buttonCancel = Button(frameButtons,text='Cancel',
                width=8,command=self.Cancel)
        self.buttonCancel.grid(row=0,column=1,padx=5,pady=5)

    def NameOk(self):
        #simple validity check for a sensible
        #ConfigParser file section name
        nameOk=1
        name=self.name.get()
        name.strip()
        if not name: #no name specified
            tkMessageBox.showerror(title='Name Error',
                    message='No name specified.', parent=self)
            nameOk=0
        elif len(name)>30: #name too long
            tkMessageBox.showerror(title='Name Error',
                    message='Name too long. It should be no more than '+
                    '30 characters.', parent=self)
            nameOk=0
        elif name in self.usedNames:
            tkMessageBox.showerror(title='Name Error',
                    message='This name is already in use.', parent=self)
            nameOk=0
        return nameOk

    def Ok(self, event=None):
        if self.NameOk():
            self.result=self.name.get().strip()
            self.destroy()

    def Cancel(self, event=None):
        self.result=''
        self.destroy()

if __name__ == '__main__':
    #test the dialog
    root=Tk()
    def run():
        keySeq=''
        dlg=GetCfgSectionNameDialog(root,'Get Name',
                'The information here should need to be word wrapped. Test.')
        print(dlg.result)
    Button(root,text='Dialog',command=run).pack()
    root.mainloop()
