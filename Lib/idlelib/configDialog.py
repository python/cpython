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
        self.bind('<F1>',self.HelpBinding) #context help
        self.bind('<Alt-f>',self.ChangePageBinding)
        self.bind('<Alt-h>',self.ChangePageBinding)
        self.bind('<Alt-k>',self.ChangePageBinding)
        self.bind('<Alt-g>',self.ChangePageBinding)
        self.wait_window()
        
    def Cancel(self):
        self.destroy()

    def Save(self):
        pass

    def Help(self):
        pass

    def CancelBinding(self,event):
        self.Cancel()
    
    def SaveBinding(self,event):
        self.Save()
    
    def HelpBinding(self,event):
        self.Help()
    
    def ChangePage(self):
        self.pages[self.pageNum.get()].lift()
        self.title('Settings -'+self.pageButtons[self.pageNum.get()].cget('text'))

    def ChangePageBinding(self,event):
        pageKeys=('f','h','k','g')
        pos=0
        for key in pageKeys:
            if event.char == key:
                self.pageNum.set(pos)
                self.ChangePage()
                return
            pos=pos+1
    
    def SetThemeType(self):
        if self.themeType.get()==0:
            self.optMenuThemeBuiltin.config(state=NORMAL)
            self.optMenuThemeCustom.config(state=DISABLED)
            self.buttonDeleteCustom.config(state=DISABLED)
        elif self.themeType.get()==1:
            self.optMenuThemeBuiltin.config(state=DISABLED)
            self.optMenuThemeCustom.config(state=NORMAL)
            self.buttonDeleteCustom.config(state=NORMAL)
    
    def CreateWidgets(self):
        self.framePages = Frame(self)
        frameActionButtons = Frame(self)
        framePageButtons = Frame(self.framePages,borderwidth=1,relief=SUNKEN)
        #action buttons
        self.buttonHelp = Button(frameActionButtons,text='Help',
                command=self.Help,takefocus=FALSE)
        self.buttonSave = Button(frameActionButtons,text='Save, Apply and Exit',
                command=self.Save,underline=0,takefocus=FALSE)
        self.buttonCancel = Button(frameActionButtons,text='Cancel',
                command=self.Cancel,takefocus=FALSE)
        #page buttons
        self.pageNum=IntVar()
        self.pageNum.set(0)
        buttonPageFonts = Radiobutton(framePageButtons,value=0,
                text='Font/Tabs',padx=5,pady=5)
        buttonPageHighlight = Radiobutton(framePageButtons,value=1,
                text='Highlighting',padx=5,pady=5)
        buttonPageKeys = Radiobutton(framePageButtons,value=2,
                text='Keys',padx=5,pady=5)
        buttonPageGeneral = Radiobutton(framePageButtons,value=3,
                text='General',padx=5,pady=5)
        self.pageButtons=(buttonPageFonts,buttonPageHighlight,
                buttonPageKeys,buttonPageGeneral)
        for button in self.pageButtons:
            button.config(command=self.ChangePage,underline=0,takefocus=FALSE,
            indicatoron=FALSE,highlightthickness=0,variable=self.pageNum,
            selectcolor=self.bg,borderwidth=1)
            button.pack(side=LEFT)
        #pages
        self.pages=(self.CreatePageFonts(),
                    self.CreatePageHighlight(),
                    self.CreatePageKeys(),
                    self.CreatePageGeneral())

        #grid in framePages so we can overlap pages
        framePageButtons.grid(row=0,column=0,sticky=W)
        for page in self.pages: page.grid(row=1,column=0,sticky=(N,S,E,W))
        
        self.buttonHelp.pack(side=RIGHT,padx=20,pady=5)
        self.buttonSave.pack(side=LEFT,padx=5,pady=5)
        self.buttonCancel.pack(side=LEFT,padx=5,pady=5)
        frameActionButtons.pack(side=BOTTOM)
        self.framePages.pack(side=TOP,expand=TRUE,fill=BOTH)
        
    def CreatePageFonts(self):
        frame=Frame(self.framePages,borderwidth=2,relief=SUNKEN)
        Button(frame,text='fonts page test').pack(padx=30,pady=30)
        return frame

    def CreatePageHighlight(self):
        #tkVars
        self.target=StringVar()
        self.builtinTheme=StringVar()
        self.customTheme=StringVar()
        self.colour=StringVar()
        self.fontName=StringVar()
        self.fontBold=StringVar()
        self.fontItalic=StringVar()
        self.fontSize=IntVar()
        self.themeType=IntVar() 
        ##widget creation
        #body frame
        frame=Frame(self.framePages,borderwidth=2,relief=SUNKEN)
        #body section frames
        frameCustom=Frame(frame,borderwidth=2,relief=GROOVE)
        frameTheme=Frame(frame,borderwidth=2,relief=GROOVE)

        #frameCustom
        frameTarget=Frame(frameCustom)
        frameSample=Frame(frameCustom,relief=SOLID,borderwidth=1)
        frameSet=Frame(frameCustom)
        frameColourSet=Frame(frameSet,relief=SOLID,borderwidth=1)
        frameFontSet=Frame(frameSet)
        
        labelCustomTitle=Label(frameCustom,text='Set Custom Highlighting')
        labelTargetTitle=Label(frameTarget,text='for : ')
        optMenuTarget=OptionMenu(frameTarget,
            self.target,'test target interface item','test target interface item 2')
        self.target.set('test target interface item')
        buttonSetColour=Button(frameColourSet,text='Set Colour')
        labelFontTitle=Label(frameFontSet,text='Set Font Style')
        checkFontBold=Checkbutton(frameFontSet,variable=self.fontBold,
            onvalue='Bold',offvalue='',text='Bold')
        checkFontItalic=Checkbutton(frameFontSet,variable=self.fontItalic,
            onvalue='Italic',offvalue='',text='Italic')
        labelTestSample=Label(frameSample,justify=LEFT,
            text='def Ahem(foo,bar):\n    test=foo\n    text=bar\n    return')        
        buttonSaveCustom=Button(frameCustom, 
            text='Save as a Custom Theme')
        
        #frameTheme
        #frameDivider=Frame(frameTheme,relief=SUNKEN,borderwidth=1,
        #    width=2,height=10)
        labelThemeTitle=Label(frameTheme,text='Select a Highlighting Theme')
        labelTypeTitle=Label(frameTheme,text='Select : ')
        radioThemeBuiltin=Radiobutton(frameTheme,variable=self.themeType,
            value=0,command=self.SetThemeType,text='a Built-in Theme')
        radioThemeCustom=Radiobutton(frameTheme,variable=self.themeType,
            value=1,command=self.SetThemeType,text='a Custom Theme')
        self.optMenuThemeBuiltin=OptionMenu(frameTheme,
            self.builtinTheme,'test builtin junk','test builtin junk 2')
        self.builtinTheme.set('test builtin junk')
        self.optMenuThemeCustom=OptionMenu(frameTheme,
            self.customTheme,'test custom junk','test custom junk 2')
        self.customTheme.set('test custom junk')
        self.themeType.set(0)
        self.buttonDeleteCustom=Button(frameTheme,text='Delete Custom Theme')
        self.SetThemeType()
        
        ##widget packing
        #body
        frameCustom.pack(side=LEFT,padx=5,pady=10,fill=Y)
        frameTheme.pack(side=RIGHT,padx=5,pady=10,fill=Y)
        #frameCustom
        labelCustomTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        frameTarget.pack(side=TOP,padx=5,pady=5,expand=TRUE,fill=X)
        frameSample.pack(side=TOP,padx=5,pady=5,expand=TRUE,fill=BOTH)
        frameSet.pack(side=TOP,fill=X)
        frameColourSet.pack(side=LEFT,padx=5,pady=5,fill=BOTH)
        frameFontSet.pack(side=RIGHT,padx=5,pady=5,anchor=W)
        labelTargetTitle.pack(side=LEFT,anchor=E)
        optMenuTarget.pack(side=RIGHT,anchor=W,fill=X,expand=TRUE)
        buttonSetColour.pack(expand=TRUE,fill=BOTH,padx=10,pady=10)
        labelFontTitle.pack(side=TOP,anchor=W)
        checkFontBold.pack(side=LEFT,anchor=W,pady=2)
        checkFontItalic.pack(side=RIGHT,anchor=W)
        labelTestSample.pack()
        buttonSaveCustom.pack(side=BOTTOM,fill=X,padx=5,pady=5)        
                
        #frameTheme
        #frameDivider.pack(side=LEFT,fill=Y,padx=5,pady=5)
        labelThemeTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        labelTypeTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        radioThemeBuiltin.pack(side=TOP,anchor=W,padx=5)
        radioThemeCustom.pack(side=TOP,anchor=W,padx=5,pady=2)
        self.optMenuThemeBuiltin.pack(side=TOP,fill=X,padx=5,pady=5)
        self.optMenuThemeCustom.pack(side=TOP,fill=X,anchor=W,padx=5,pady=5)
        self.buttonDeleteCustom.pack(side=TOP,fill=X,padx=5,pady=5)
        
        return frame

    def CreatePageKeys(self):
        frame=Frame(self.framePages,borderwidth=2,relief=SUNKEN)
        Button(frame,text='keys page test').pack(padx=90,pady=90)
        return frame

    def CreatePageGeneral(self):
        frame=Frame(self.framePages,borderwidth=2,relief=SUNKEN)
        Button(frame,text='general page test').pack(padx=110,pady=110)
        return frame

if __name__ == '__main__':
    #test the dialog
    root=Tk()
    Button(root,text='Dialog',
            command=lambda:ConfigDialog(root,'Settings',None)).pack()
    root.mainloop()
