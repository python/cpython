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

import IdleConf

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
        self.LoadConfig()
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
        self.bind('<Escape>',self.CancelBinding) #dismiss dialog, no save
        self.bind('<Alt-s>',self.SaveBinding) #dismiss dialog, save
        self.bind('<F1>',self.HelpBinding) #context help
        self.bind('<Alt-f>',self.ChangePageBinding)
        self.bind('<Alt-h>',self.ChangePageBinding)
        self.bind('<Alt-k>',self.ChangePageBinding)
        self.bind('<Alt-g>',self.ChangePageBinding)
        self.wait_window()
        
    def LoadConfig(self):
        #self.configParser=IdleConf.idleconf
        #self.loadedConfig={}        
        #self.workingConfig={}
        #for key in .keys():        
        #print self.configParser.getsection('Colors').options()
        self.workingTestColours={
                'Foo-Bg': '#ffffff',
                'Foo-Fg': '#000000',
                'Bar-Bg': '#777777'}
        
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
        self.title('Settings - '+self.pageButtons[self.pageNum.get()].cget('text'))

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
            self.buttonDeleteCustomTheme.config(state=DISABLED)
        elif self.themeType.get()==1:
            self.optMenuThemeBuiltin.config(state=DISABLED)
            self.optMenuThemeCustom.config(state=NORMAL)
            self.buttonDeleteCustomTheme.config(state=NORMAL)

    def SetKeysType(self):
        if self.keysType.get()==0:
            self.optMenuKeysBuiltin.config(state=NORMAL)
            self.optMenuKeysCustom.config(state=DISABLED)
            self.buttonDeleteCustomKeys.config(state=DISABLED)
        elif self.keysType.get()==1:
            self.optMenuKeysBuiltin.config(state=DISABLED)
            self.optMenuKeysCustom.config(state=NORMAL)
            self.buttonDeleteCustomKeys.config(state=NORMAL)
    
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
        buttonPageFontTab = Radiobutton(framePageButtons,value=0,
                text='Font/Tabs',padx=5,pady=5)
        buttonPageHighlight = Radiobutton(framePageButtons,value=1,
                text='Highlighting',padx=5,pady=5)
        buttonPageKeys = Radiobutton(framePageButtons,value=2,
                text='Keys',padx=5,pady=5)
        buttonPageGeneral = Radiobutton(framePageButtons,value=3,
                text='General',padx=5,pady=5)
        self.pageButtons=(buttonPageFontTab,buttonPageHighlight,
                buttonPageKeys,buttonPageGeneral)
        for button in self.pageButtons:
            button.config(command=self.ChangePage,underline=0,takefocus=FALSE,
            indicatoron=FALSE,highlightthickness=0,variable=self.pageNum,
            selectcolor=self.bg,borderwidth=1)
            button.pack(side=LEFT)
        #pages
        self.pages=(self.CreatePageFontTab(),
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
        
    def CreatePageFontTab(self):
        frame=Frame(self.framePages,borderwidth=2,relief=SUNKEN)
        Button(frame,text='font/tabs page test').pack(padx=90,pady=90)
        return frame

    def CreatePageHighlight(self):
        #tkVars
        self.highlightTarget=StringVar()
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
        frameSample=Frame(frameCustom,relief=SOLID,borderwidth=1,
                bg=self.workingTestColours['Foo-Bg'])
        frameSet=Frame(frameCustom)
        frameColourSet=Frame(frameSet,relief=SOLID,borderwidth=1,
                bg=self.workingTestColours['Foo-Bg'])
        frameFontSet=Frame(frameSet)
        labelCustomTitle=Label(frameCustom,text='Set Custom Highlighting')
        labelTargetTitle=Label(frameTarget,text='for : ')
        optMenuTarget=OptionMenu(frameTarget,
            self.highlightTarget,'test target interface item','test target interface item 2')
        self.highlightTarget.set('test target interface item')
        buttonSetColour=Button(frameColourSet,text='Set Colour')
        labelFontTitle=Label(frameFontSet,text='Set Font Style')
        checkFontBold=Checkbutton(frameFontSet,variable=self.fontBold,
            onvalue='Bold',offvalue='',text='Bold')
        checkFontItalic=Checkbutton(frameFontSet,variable=self.fontItalic,
            onvalue='Italic',offvalue='',text='Italic')
        labelTestSample=Label(frameSample,justify=LEFT,
            text='def Ahem(foo,bar):\n    test=foo\n    text=bar\n    return',
            bg=self.workingTestColours['Foo-Bg'])        
        buttonSaveCustomTheme=Button(frameCustom, 
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
        self.buttonDeleteCustomTheme=Button(frameTheme,text='Delete Custom Theme')
        self.SetThemeType()
        ##widget packing
        #body
        frameCustom.pack(side=LEFT,padx=5,pady=10,expand=TRUE,fill=BOTH)
        frameTheme.pack(side=LEFT,padx=5,pady=10,fill=Y)
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
        buttonSaveCustomTheme.pack(side=BOTTOM,fill=X,padx=5,pady=5)        
        #frameTheme
        #frameDivider.pack(side=LEFT,fill=Y,padx=5,pady=5)
        labelThemeTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        labelTypeTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        radioThemeBuiltin.pack(side=TOP,anchor=W,padx=5)
        radioThemeCustom.pack(side=TOP,anchor=W,padx=5,pady=2)
        self.optMenuThemeBuiltin.pack(side=TOP,fill=X,padx=5,pady=5)
        self.optMenuThemeCustom.pack(side=TOP,fill=X,anchor=W,padx=5,pady=5)
        self.buttonDeleteCustomTheme.pack(side=TOP,fill=X,padx=5,pady=5)
        return frame

    def CreatePageKeys(self):
        #tkVars
        self.bindingTarget=StringVar()
        self.builtinKeys=StringVar()
        self.customKeys=StringVar()
        self.keyChars=StringVar()
        self.keyCtrl=StringVar()
        self.keyAlt=StringVar()
        self.keyShift=StringVar()
        self.keysType=IntVar() 
        ##widget creation
        #body frame
        frame=Frame(self.framePages,borderwidth=2,relief=SUNKEN)
        #body section frames
        frameCustom=Frame(frame,borderwidth=2,relief=GROOVE)
        frameKeySets=Frame(frame,borderwidth=2,relief=GROOVE)
        #frameCustom
        frameTarget=Frame(frameCustom)
        frameSet=Frame(frameCustom)
        labelCustomTitle=Label(frameCustom,text='Set Custom Key Bindings')
        labelTargetTitle=Label(frameTarget,text='Action')
        scrollTarget=Scrollbar(frameTarget)
        listTarget=Listbox(frameTarget)
        labelKeyBindTitle=Label(frameSet,text='Binding')
        labelModifierTitle=Label(frameSet,text='Modifier:')
        checkCtrl=Checkbutton(frameSet,text='Ctrl')
        checkAlt=Checkbutton(frameSet,text='Alt')
        checkShift=Checkbutton(frameSet,text='Shift')
        labelKeyEntryTitle=Label(frameSet,text='Key:')        
        entryKey=Entry(frameSet,width=4)
        buttonSaveCustomKeys=Button(frameCustom,text='Save as a Custom Key Set')
        #frameKeySets
        labelKeysTitle=Label(frameKeySets,text='Select a Key Binding Set')
        labelTypeTitle=Label(frameKeySets,text='Select : ')
        radioKeysBuiltin=Radiobutton(frameKeySets,variable=self.keysType,
            value=0,command=self.SetKeysType,text='a Built-in Key Set')
        radioKeysCustom=Radiobutton(frameKeySets,variable=self.keysType,
            value=1,command=self.SetKeysType,text='a Custom Key Set')
        self.optMenuKeysBuiltin=OptionMenu(frameKeySets,
            self.builtinKeys,'test builtin junk','test builtin junk 2')
        self.builtinKeys.set('test builtin junk')
        self.optMenuKeysCustom=OptionMenu(frameKeySets,
            self.customKeys,'test custom junk','test custom junk 2')
        self.customKeys.set('test custom junk')
        self.keysType.set(0)
        self.buttonDeleteCustomKeys=Button(frameKeySets,text='Delete Custom Key Set')
        self.SetKeysType()
        ##widget packing
        #body
        frameCustom.pack(side=LEFT,padx=5,pady=5,expand=TRUE,fill=BOTH)
        frameKeySets.pack(side=LEFT,padx=5,pady=5,fill=Y)
        #frameCustom
        labelCustomTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        buttonSaveCustomKeys.pack(side=BOTTOM,fill=X,padx=5,pady=5)        
        frameTarget.pack(side=LEFT,padx=5,pady=5,fill=Y)
        frameSet.pack(side=LEFT,padx=5,pady=5,fill=Y)
        labelTargetTitle.pack(side=TOP,anchor=W)
        scrollTarget.pack(side=RIGHT,anchor=W,fill=Y)
        listTarget.pack(side=TOP,anchor=W,expand=TRUE,fill=BOTH)
        labelKeyBindTitle.pack(side=TOP,anchor=W)
        labelModifierTitle.pack(side=TOP,anchor=W,pady=5)
        checkCtrl.pack(side=TOP,anchor=W)
        checkAlt.pack(side=TOP,anchor=W,pady=2)
        checkShift.pack(side=TOP,anchor=W)
        labelKeyEntryTitle.pack(side=TOP,anchor=W,pady=5)
        entryKey.pack(side=TOP,anchor=W)
        #frameKeySets
        labelKeysTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        labelTypeTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        radioKeysBuiltin.pack(side=TOP,anchor=W,padx=5)
        radioKeysCustom.pack(side=TOP,anchor=W,padx=5,pady=2)
        self.optMenuKeysBuiltin.pack(side=TOP,fill=X,padx=5,pady=5)
        self.optMenuKeysCustom.pack(side=TOP,fill=X,anchor=W,padx=5,pady=5)
        self.buttonDeleteCustomKeys.pack(side=TOP,fill=X,padx=5,pady=5)
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
