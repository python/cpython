##---------------------------------------------------------------------------##
##
## idle - configuration dialog 
## elguavas
## 
##---------------------------------------------------------------------------##
"""
configuration dialog
"""
from Tkinter import *
import tkMessageBox

class ConfigDialog(Toplevel):
    """
    configuration dialog for idle
    """ 
    def __init__(self,parent,title,configDict):
        """
        configDict - dictionary of configuration items
        """
        Toplevel.__init__(self, parent)
        self.configure(borderwidth=5)
        self.geometry("+%d+%d" % (parent.winfo_rootx()+20,
                parent.winfo_rooty()+30))
        self.config=configDict
        #elguavas - config placeholders til config stuff completed
        self.bg=self.cget('bg')
        self.fg=None
        #no ugly bold default text font on *nix
        self.textFont=tuple(Label().cget('font').split())[0:2]+('normal',) 

        self.CreateWidgets()
        self.resizable(height=FALSE,width=FALSE)
        self.ChangePage()
        self.transient(parent)
        self.grab_set()
        self.protocol("WM_DELETE_WINDOW", self.Cancel)
        self.parent = parent
        self.framePages.focus_set()
        #key bindings for this dialog
#    self.bind('<Return>',self.Ok) #dismiss dialog
        self.bind('<Escape>',self.CancelBinding) #dismiss dialog, no save
        self.bind('<Alt-s>',self.SaveBinding) #dismiss dialog, save
        self.bind('<Alt-r>',self.RevertBinding) #revert to defaults
        self.bind('<Alt-f>',self.ChangePageBinding)
        self.bind('<Alt-c>',self.ChangePageBinding)
        self.bind('<Alt-k>',self.ChangePageBinding)
        self.bind('<Alt-g>',self.ChangePageBinding)
        self.wait_window()
        
    def Cancel(self):
        self.destroy()

    def Save(self):
        pass

    def Revert(self):
        pass

    def ChangePage(self):
        self.pages[self.pageNum.get()].lift()
        self.title('Settings - '+self.pageButtons[self.pageNum.get()].cget('text'))

    def CancelBinding(self,event):
        self.Cancel()
    
    def SaveBinding(self,event):
        self.Save()
    
    def RevertBinding(self,event):
        self.Revert()
    
    def ChangePageBinding(self,event):
        pageKeys=('f','c','k','g')
        pos=0
        for key in pageKeys:
            if event.char == key:
                self.pageNum.set(pos)
                self.ChangePage()
                return
            pos=pos+1
    
    def CreateWidgets(self):
        self.framePages = Frame(self,borderwidth=2,relief=SUNKEN)
        frameActionButtons = Frame(self)
        framePageButtons = Frame(self.framePages,borderwidth=1,relief=SUNKEN)
        #action buttons
        self.buttonRevert = Button(frameActionButtons,text='Revert',
                command=self.Revert,underline=0,takefocus=FALSE)
        self.buttonSave = Button(frameActionButtons,text='Save',
                command=self.Save,underline=0,takefocus=FALSE)
        self.buttonCancel = Button(frameActionButtons,text='Cancel',
                command=self.Cancel,takefocus=FALSE)
        #page buttons
        self.pageNum=IntVar()
        self.pageNum.set(0)
        buttonPageFonts = Radiobutton(framePageButtons,value=0,text='Fonts')
        buttonPageColours = Radiobutton(framePageButtons,value=1,text='Colours')
        buttonPageKeys = Radiobutton(framePageButtons,value=2,text='Keys')
        buttonPageGeneral = Radiobutton(framePageButtons,value=3,text='General')
        self.pageButtons=(buttonPageFonts,buttonPageColours,
                buttonPageKeys,buttonPageGeneral)
        for button in self.pageButtons:
            button.config(command=self.ChangePage,underline=0,takefocus=FALSE,
            indicatoron=FALSE,highlightthickness=0,variable=self.pageNum,
            selectcolor=self.bg,borderwidth=1)
            button.pack(side=LEFT)
        #pages
        framePageFonts=Frame(self.framePages)
        framePageColours=Frame(self.framePages)
        framePageKeys=Frame(self.framePages)
        framePageGeneral=Frame(self.framePages)
        self.pages=(framePageFonts,framePageColours,framePageKeys,framePageGeneral)
        #pageFonts
        Button(framePageFonts,text='fonts page test').pack(padx=30,pady=30)
        #pageColours
        Button(framePageColours,text='colours page test').pack(padx=60,pady=60)
        #pageKeys
        Button(framePageKeys,text='keys page test').pack(padx=90,pady=90)
        #pageGeneral
        Button(framePageGeneral,text='general page test').pack(padx=110,pady=110)
        
        #grid in framePages so we can overlap pages
        framePageButtons.grid(row=0,column=0,sticky=W)
        for page in self.pages: page.grid(row=1,column=0,sticky=(N,S,E,W))
        
        self.buttonRevert.pack(side=LEFT,padx=5,pady=5)
        self.buttonSave.pack(side=LEFT,padx=5,pady=5)
        self.buttonCancel.pack(side=LEFT,padx=5,pady=5)
        frameActionButtons.pack(side=BOTTOM)
        self.framePages.pack(side=TOP,expand=TRUE,fill=BOTH)
        
if __name__ == '__main__':
    #test the dialog
    root=Tk()
    Button(root,text='Dialog',
            command=lambda:ConfigDialog(root,'Settings',None)).pack()
    root.mainloop()
