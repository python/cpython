"""
configuration dialog
"""
from Tkinter import *
import tkMessageBox, tkColorChooser, tkFont
import string

from configHandler import idleConf
from dynOptionMenuWidget import DynOptionMenu
from tabpage import TabPageSet
from keybindingDialog import GetKeysDialog
from configSectionNameDialog import GetCfgSectionNameDialog
class ConfigDialog(Toplevel):
    """
    configuration dialog for idle
    """ 
    def __init__(self,parent,title):
        Toplevel.__init__(self, parent)
        self.configure(borderwidth=5)
        self.geometry("+%d+%d" % (parent.winfo_rootx()+20,
                parent.winfo_rooty()+30))
        #Theme Elements. Each theme element key is it's display name.
        #The first value of the tuple is the sample area tag name.
        #The second value is the display name list sort index. 
        #The third value indicates whether the element can have a foreground 
        #or background colour or both. 
        self.themeElements={'Normal Text':('normal','00'),
            'Python Keywords':('keyword','01'),
            'Python Definitions':('definition','02'),
            'Python Comments':('comment','03'),
            'Python Strings':('string','04'),
            'Selected Text':('hilite','05'),
            'Found Text':('hit','06'),
            'Cursor':('cursor','07'),
            'Error Text':('error','08'),
            'Shell Normal Text':('console','09'),
            'Shell Stdout Text':('stdout','10'),
            'Shell Stderr Text':('stderr','11')}
        #changedItems. When any config item is changed in this dialog, an entry
        #should be made in the relevant section (config type) of this 
        #dictionary. The key should be the config file section name and the 
        #value a dictionary, whose key:value pairs are item=value pairs for
        #that config file section.
        self.changedItems={'main':{},'highlight':{},'keys':{},'extensions':{}}
#         #defaultItems. This dictionary is loaded with the values from the
#         #default config files. It is used for comparison with self.changedItems
#         #to decide which changed items actually need saving.
#         self.defaultItems=self.GetDefaultItems()
        self.CreateWidgets()
        self.resizable(height=FALSE,width=FALSE)
        self.transient(parent)
        self.grab_set()
        self.protocol("WM_DELETE_WINDOW", self.Cancel)
        self.parent = parent
        self.tabPages.focus_set()
        #key bindings for this dialog
        #self.bind('<Escape>',self.Cancel) #dismiss dialog, no save
        #self.bind('<Alt-a>',self.Apply) #apply changes, save
        #self.bind('<F1>',self.Help) #context help
        self.LoadConfigs()
        self.AttachVarCallbacks() #avoid callbacks during LoadConfigs 
        self.wait_window()
        
    def CreateWidgets(self):
        self.tabPages = TabPageSet(self,
                pageNames=['Fonts/Tabs','Highlighting','Keys','General'])
        self.tabPages.ChangePage()#activates default (first) page
        frameActionButtons = Frame(self)
        #action buttons
        self.buttonHelp = Button(frameActionButtons,text='Help',
                command=self.Help,takefocus=FALSE)
        self.buttonOk = Button(frameActionButtons,text='Ok',
                command=self.Ok,takefocus=FALSE)
        self.buttonApply = Button(frameActionButtons,text='Apply',
                command=self.Apply,takefocus=FALSE)
        self.buttonCancel = Button(frameActionButtons,text='Cancel',
                command=self.Cancel,takefocus=FALSE)
        self.CreatePageFontTab()
        self.CreatePageHighlight()
        self.CreatePageKeys()
        self.CreatePageGeneral()
        self.buttonHelp.pack(side=RIGHT,padx=5,pady=5)
        self.buttonOk.pack(side=LEFT,padx=5,pady=5)
        self.buttonApply.pack(side=LEFT,padx=5,pady=5)
        self.buttonCancel.pack(side=LEFT,padx=5,pady=5)
        frameActionButtons.pack(side=BOTTOM)
        self.tabPages.pack(side=TOP,expand=TRUE,fill=BOTH)
   
    def CreatePageFontTab(self):
        #tkVars
        self.fontSize=StringVar(self)
        self.fontBold=BooleanVar(self)
        self.fontName=StringVar(self)
        self.spaceNum=IntVar(self)
        self.tabCols=IntVar(self)
        self.indentBySpaces=BooleanVar(self) 
        self.editFont=tkFont.Font(self,('courier',12,'normal'))
        ##widget creation
        #body frame
        frame=self.tabPages.pages['Fonts/Tabs']['page']
        #body section frames
        frameFont=Frame(frame,borderwidth=2,relief=GROOVE)
        frameIndent=Frame(frame,borderwidth=2,relief=GROOVE)
        #frameFont
        labelFontTitle=Label(frameFont,text='Set Base Editor Font')
        frameFontName=Frame(frameFont)
        frameFontParam=Frame(frameFont)
        labelFontNameTitle=Label(frameFontName,justify=LEFT,
                text='Font :')
        self.listFontName=Listbox(frameFontName,height=5,takefocus=FALSE,
                exportselection=FALSE)
        self.listFontName.bind('<ButtonRelease-1>',self.OnListFontButtonRelease)
        scrollFont=Scrollbar(frameFontName)
        scrollFont.config(command=self.listFontName.yview)
        self.listFontName.config(yscrollcommand=scrollFont.set)
        labelFontSizeTitle=Label(frameFontParam,text='Size :')
        self.optMenuFontSize=DynOptionMenu(frameFontParam,self.fontSize,None,
            command=self.SetFontSample)
        checkFontBold=Checkbutton(frameFontParam,variable=self.fontBold,
            onvalue=1,offvalue=0,text='Bold',command=self.SetFontSample)
        frameFontSample=Frame(frameFont,relief=SOLID,borderwidth=1)
        self.labelFontSample=Label(frameFontSample,
                text='AaBbCcDdEe\nFfGgHhIiJjK\n1234567890\n#:+=(){}[]',
                justify=LEFT,font=self.editFont)
        #frameIndent
        labelIndentTitle=Label(frameIndent,text='Set Indentation Defaults')
        frameIndentType=Frame(frameIndent)
        frameIndentSize=Frame(frameIndent)
        labelIndentTypeTitle=Label(frameIndentType,
                text='Choose indentation type :')
        radioUseSpaces=Radiobutton(frameIndentType,variable=self.indentBySpaces,
            value=1,text='Tab key inserts spaces')
        radioUseTabs=Radiobutton(frameIndentType,variable=self.indentBySpaces,
            value=0,text='Tab key inserts tabs')
        labelIndentSizeTitle=Label(frameIndentSize,
                text='Choose indentation size :')
        labelSpaceNumTitle=Label(frameIndentSize,justify=LEFT,
                text='when tab key inserts spaces,\nspaces per tab')
        self.scaleSpaceNum=Scale(frameIndentSize,variable=self.spaceNum,
                orient='horizontal',tickinterval=2,from_=2,to=8)
        labeltabColsTitle=Label(frameIndentSize,justify=LEFT,
                text='when tab key inserts tabs,\ncolumns per tab')
        self.scaleTabCols=Scale(frameIndentSize,variable=self.tabCols,
                orient='horizontal',tickinterval=2,from_=2,to=8)
        #widget packing
        #body
        frameFont.pack(side=LEFT,padx=5,pady=10,expand=TRUE,fill=BOTH)
        frameIndent.pack(side=LEFT,padx=5,pady=10,fill=Y)
        #frameFont
        labelFontTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        frameFontName.pack(side=TOP,padx=5,pady=5,fill=X)
        frameFontParam.pack(side=TOP,padx=5,pady=5,fill=X)
        labelFontNameTitle.pack(side=TOP,anchor=W)
        self.listFontName.pack(side=LEFT,expand=TRUE,fill=X)
        scrollFont.pack(side=LEFT,fill=Y)
        labelFontSizeTitle.pack(side=LEFT,anchor=W)
        self.optMenuFontSize.pack(side=LEFT,anchor=W)
        checkFontBold.pack(side=LEFT,anchor=W,padx=20)
        frameFontSample.pack(side=TOP,padx=5,pady=5,expand=TRUE,fill=BOTH)
        self.labelFontSample.pack(expand=TRUE,fill=BOTH)
        #frameIndent
        labelIndentTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        frameIndentType.pack(side=TOP,padx=5,fill=X)
        frameIndentSize.pack(side=TOP,padx=5,pady=5,fill=BOTH)
        labelIndentTypeTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        radioUseSpaces.pack(side=TOP,anchor=W,padx=5)
        radioUseTabs.pack(side=TOP,anchor=W,padx=5)
        labelIndentSizeTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        labelSpaceNumTitle.pack(side=TOP,anchor=W,padx=5)
        self.scaleSpaceNum.pack(side=TOP,padx=5,fill=X)
        labeltabColsTitle.pack(side=TOP,anchor=W,padx=5)
        self.scaleTabCols.pack(side=TOP,padx=5,fill=X)
        return frame

    def CreatePageHighlight(self):
        self.builtinTheme=StringVar(self)
        self.customTheme=StringVar(self)
        self.fgHilite=BooleanVar(self)
        self.colour=StringVar(self)
        self.fontName=StringVar(self)
        self.themeIsBuiltin=BooleanVar(self) 
        self.highlightTarget=StringVar(self)
        ##widget creation
        #body frame
        frame=self.tabPages.pages['Highlighting']['page']
        #body section frames
        frameCustom=Frame(frame,borderwidth=2,relief=GROOVE)
        frameTheme=Frame(frame,borderwidth=2,relief=GROOVE)
        #frameCustom
        self.textHighlightSample=Text(frameCustom,relief=SOLID,borderwidth=1,
            font=('courier',12,''),cursor='hand2',width=10,height=10,
            takefocus=FALSE,highlightthickness=0)
        text=self.textHighlightSample
        text.bind('<Double-Button-1>',lambda e: 'break')
        text.bind('<B1-Motion>',lambda e: 'break')
        textAndTags=(('#you can click in here','comment'),('\n','normal'),
            ('#to choose items','comment'),('\n','normal'),('def','keyword'),
            (' ','normal'),('func','definition'),('(param):','normal'),
            ('\n  ','normal'),('"""string"""','string'),('\n  var0 = ','normal'),
            ("'string'",'string'),('\n  var1 = ','normal'),("'selected'",'hilite'),
            ('\n  var2 = ','normal'),("'found'",'hit'),('\n\n','normal'),
            (' error ','error'),(' ','normal'),('cursor |','cursor'),
            ('\n ','normal'),('shell','console'),(' ','normal'),('stdout','stdout'),
            (' ','normal'),('stderr','stderr'),('\n','normal'))
        for txTa in textAndTags:
            text.insert(END,txTa[0],txTa[1])
        for element in self.themeElements.keys(): 
            text.tag_bind(self.themeElements[element][0],'<ButtonPress-1>',
                lambda event,elem=element: event.widget.winfo_toplevel()
                .highlightTarget.set(elem))
        text.config(state=DISABLED)
        self.frameColourSet=Frame(frameCustom,relief=SOLID,borderwidth=1)
        frameFgBg=Frame(frameCustom)
        labelCustomTitle=Label(frameCustom,text='Set Custom Highlighting')
        buttonSetColour=Button(self.frameColourSet,text='Choose Colour for :',
            command=self.GetColour,highlightthickness=0)
        self.optMenuHighlightTarget=DynOptionMenu(self.frameColourSet,
            self.highlightTarget,None,highlightthickness=0)#,command=self.SetHighlightTargetBinding
        self.radioFg=Radiobutton(frameFgBg,variable=self.fgHilite,
            value=1,text='Foreground',command=self.SetColourSampleBinding)
        self.radioBg=Radiobutton(frameFgBg,variable=self.fgHilite,
            value=0,text='Background',command=self.SetColourSampleBinding)
        self.fgHilite.set(1)
        buttonSaveCustomTheme=Button(frameCustom, 
            text='Save as New Custom Theme')
        #frameTheme
        labelThemeTitle=Label(frameTheme,text='Select a Highlighting Theme')
        labelTypeTitle=Label(frameTheme,text='Select : ')
        self.radioThemeBuiltin=Radiobutton(frameTheme,variable=self.themeIsBuiltin,
            value=1,command=self.SetThemeType,text='a Built-in Theme')
        self.radioThemeCustom=Radiobutton(frameTheme,variable=self.themeIsBuiltin,
            value=0,command=self.SetThemeType,text='a Custom Theme')
        self.optMenuThemeBuiltin=DynOptionMenu(frameTheme,
            self.builtinTheme,None,command=None)
        self.optMenuThemeCustom=DynOptionMenu(frameTheme,
            self.customTheme,None,command=None)
        self.buttonDeleteCustomTheme=Button(frameTheme,text='Delete Custom Theme')
        ##widget packing
        #body
        frameCustom.pack(side=LEFT,padx=5,pady=10,expand=TRUE,fill=BOTH)
        frameTheme.pack(side=LEFT,padx=5,pady=10,fill=Y)
        #frameCustom
        labelCustomTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        self.frameColourSet.pack(side=TOP,padx=5,pady=5,expand=TRUE,fill=X)
        frameFgBg.pack(side=TOP,padx=5,pady=0)
        self.textHighlightSample.pack(side=TOP,padx=5,pady=5,expand=TRUE,
            fill=BOTH)
        buttonSetColour.pack(side=TOP,expand=TRUE,fill=X,padx=8,pady=4)
        self.optMenuHighlightTarget.pack(side=TOP,expand=TRUE,fill=X,padx=8,pady=3)
        self.radioFg.pack(side=LEFT,anchor=E)
        self.radioBg.pack(side=RIGHT,anchor=W)
        buttonSaveCustomTheme.pack(side=BOTTOM,fill=X,padx=5,pady=5)        
        #frameTheme
        labelThemeTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        labelTypeTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        self.radioThemeBuiltin.pack(side=TOP,anchor=W,padx=5)
        self.radioThemeCustom.pack(side=TOP,anchor=W,padx=5,pady=2)
        self.optMenuThemeBuiltin.pack(side=TOP,fill=X,padx=5,pady=5)
        self.optMenuThemeCustom.pack(side=TOP,fill=X,anchor=W,padx=5,pady=5)
        self.buttonDeleteCustomTheme.pack(side=TOP,fill=X,padx=5,pady=5)
        return frame

    def CreatePageKeys(self):
        #tkVars
        self.bindingTarget=StringVar(self)
        self.builtinKeys=StringVar(self)
        self.customKeys=StringVar(self)
        self.keysAreDefault=BooleanVar(self) 
        self.keyBinding=StringVar(self)
        ##widget creation
        #body frame
        frame=self.tabPages.pages['Keys']['page']
        #body section frames
        frameCustom=Frame(frame,borderwidth=2,relief=GROOVE)
        frameKeySets=Frame(frame,borderwidth=2,relief=GROOVE)
        #frameCustom
        frameTarget=Frame(frameCustom)
        labelCustomTitle=Label(frameCustom,text='Set Custom Key Bindings')
        labelTargetTitle=Label(frameTarget,text='Action - Key(s)')
        scrollTargetY=Scrollbar(frameTarget)
        scrollTargetX=Scrollbar(frameTarget,orient=HORIZONTAL)
        self.listBindings=Listbox(frameTarget)
        self.listBindings.bind('<ButtonRelease-1>',self.KeyBindingSelected)
        scrollTargetY.config(command=self.listBindings.yview)
        scrollTargetX.config(command=self.listBindings.xview)
        self.listBindings.config(yscrollcommand=scrollTargetY.set)
        self.listBindings.config(xscrollcommand=scrollTargetX.set)
        self.buttonNewKeys=Button(frameCustom,text='Get New Keys for Selection',
            command=self.GetNewKeys,state=DISABLED)
        buttonSaveCustomKeys=Button(frameCustom,text='Save as New Custom Key Set')
        #frameKeySets
        labelKeysTitle=Label(frameKeySets,text='Select a Key Set')
        labelTypeTitle=Label(frameKeySets,text='Select : ')
        self.radioKeysBuiltin=Radiobutton(frameKeySets,variable=self.keysAreDefault,
            value=1,command=self.SetKeysType,text='a Built-in Key Set')
        self.radioKeysCustom=Radiobutton(frameKeySets,variable=self.keysAreDefault,
            value=0,command=self.SetKeysType,text='a Custom Key Set')
        self.optMenuKeysBuiltin=DynOptionMenu(frameKeySets,
            self.builtinKeys,None,command=None)
        self.optMenuKeysCustom=DynOptionMenu(frameKeySets,
            self.customKeys,None,command=None)
        self.buttonDeleteCustomKeys=Button(frameKeySets,text='Delete Custom Key Set')
        ##widget packing
        #body
        frameCustom.pack(side=LEFT,padx=5,pady=5,expand=TRUE,fill=BOTH)
        frameKeySets.pack(side=LEFT,padx=5,pady=5,fill=Y)
        #frameCustom
        labelCustomTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        buttonSaveCustomKeys.pack(side=BOTTOM,fill=X,padx=5,pady=5)        
        self.buttonNewKeys.pack(side=BOTTOM,fill=X,padx=5,pady=5)        
        frameTarget.pack(side=LEFT,padx=5,pady=5,expand=TRUE,fill=BOTH)
        #frame target
        frameTarget.columnconfigure(0,weight=1)
        frameTarget.rowconfigure(1,weight=1)
        labelTargetTitle.grid(row=0,column=0,columnspan=2,sticky=W)
        self.listBindings.grid(row=1,column=0,sticky=NSEW)
        scrollTargetY.grid(row=1,column=1,sticky=NS)
        scrollTargetX.grid(row=2,column=0,sticky=EW)
        #frameKeySets
        labelKeysTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        labelTypeTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        self.radioKeysBuiltin.pack(side=TOP,anchor=W,padx=5)
        self.radioKeysCustom.pack(side=TOP,anchor=W,padx=5,pady=2)
        self.optMenuKeysBuiltin.pack(side=TOP,fill=X,padx=5,pady=5)
        self.optMenuKeysCustom.pack(side=TOP,fill=X,anchor=W,padx=5,pady=5)
        self.buttonDeleteCustomKeys.pack(side=TOP,fill=X,padx=5,pady=5)
        return frame

    def CreatePageGeneral(self):
        #tkVars        
        self.winWidth=StringVar(self)       
        self.winHeight=StringVar(self)
        self.startupEdit=IntVar(self)       
        self.extEnabled=IntVar(self)       
        #widget creation
        #body
        frame=self.tabPages.pages['General']['page']
        #body section frames        
        frameRun=Frame(frame,borderwidth=2,relief=GROOVE)
        frameWinSize=Frame(frame,borderwidth=2,relief=GROOVE)
        frameExt=Frame(frame,borderwidth=2,relief=GROOVE)
        #frameRun
        labelRunTitle=Label(frameRun,text='Startup Preferences')
        labelRunChoiceTitle=Label(frameRun,text='On startup : ')
        radioStartupEdit=Radiobutton(frameRun,variable=self.startupEdit,
            value=1,command=self.SetKeysType,text="open Edit Window")
        radioStartupShell=Radiobutton(frameRun,variable=self.startupEdit,
            value=0,command=self.SetKeysType,text='open Shell Window')
        #frameWinSize
        labelWinSizeTitle=Label(frameWinSize,text='Initial Window Size'+
                '  (in characters)')
        labelWinWidthTitle=Label(frameWinSize,text='Width')
        entryWinWidth=Entry(frameWinSize,textvariable=self.winWidth,
                width=3)
        labelWinHeightTitle=Label(frameWinSize,text='Height')
        entryWinHeight=Entry(frameWinSize,textvariable=self.winHeight,
                width=3)
        #frameExt
        frameExtList=Frame(frameExt)
        frameExtSet=Frame(frameExt)
        labelExtTitle=Label(frameExt,text='Configure IDLE Extensions')
        labelExtListTitle=Label(frameExtList,text='Extension')
        scrollExtList=Scrollbar(frameExtList)
        self.listExt=Listbox(frameExtList,height=5)
        scrollExtList.config(command=self.listExt.yview)
        self.listExt.config(yscrollcommand=scrollExtList.set)
        self.listExt.bind('<ButtonRelease-1>',self.ExtensionSelected)
        labelExtSetTitle=Label(frameExtSet,text='Settings')
        self.radioEnableExt=Radiobutton(frameExtSet,variable=self.extEnabled,
            value=1,text="enabled",state=DISABLED,
            command=self.ExtensionStateToggled)
        self.radioDisableExt=Radiobutton(frameExtSet,variable=self.extEnabled,
            value=0,text="disabled",state=DISABLED,
            command=self.ExtensionStateToggled)
        self.buttonExtConfig=Button(frameExtSet,text='Configure',state=DISABLED)
        #widget packing
        #body
        frameRun.pack(side=TOP,padx=5,pady=5,fill=X)
        frameWinSize.pack(side=TOP,padx=5,pady=5,fill=X)
        frameExt.pack(side=TOP,padx=5,pady=5,expand=TRUE,fill=BOTH)
        #frameRun
        labelRunTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        labelRunChoiceTitle.pack(side=LEFT,anchor=W,padx=5,pady=5)
        radioStartupEdit.pack(side=LEFT,anchor=W,padx=5,pady=5)
        radioStartupShell.pack(side=LEFT,anchor=W,padx=5,pady=5)     
        #frameWinSize
        labelWinSizeTitle.pack(side=LEFT,anchor=W,padx=5,pady=5)
        entryWinHeight.pack(side=RIGHT,anchor=E,padx=10,pady=5)
        labelWinHeightTitle.pack(side=RIGHT,anchor=E,pady=5)
        entryWinWidth.pack(side=RIGHT,anchor=E,padx=10,pady=5)
        labelWinWidthTitle.pack(side=RIGHT,anchor=E,pady=5)
        #frameExt
        labelExtTitle.pack(side=TOP,anchor=W,padx=5,pady=5)
        frameExtSet.pack(side=RIGHT,padx=5,pady=5,fill=Y)
        frameExtList.pack(side=RIGHT,padx=5,pady=5,expand=TRUE,fill=BOTH)
        labelExtListTitle.pack(side=TOP,anchor=W)
        scrollExtList.pack(side=RIGHT,anchor=W,fill=Y)
        self.listExt.pack(side=LEFT,anchor=E,expand=TRUE,fill=BOTH)
        labelExtSetTitle.pack(side=TOP,anchor=W)
        self.radioEnableExt.pack(side=TOP,anchor=W)
        self.radioDisableExt.pack(side=TOP,anchor=W)
        self.buttonExtConfig.pack(side=TOP,anchor=W,pady=5)
        return frame

    def AttachVarCallbacks(self):
        self.fontSize.trace_variable('w',self.VarChanged_fontSize)
        self.fontName.trace_variable('w',self.VarChanged_fontName)
        self.fontBold.trace_variable('w',self.VarChanged_fontBold)
        self.spaceNum.trace_variable('w',self.VarChanged_spaceNum)
        self.tabCols.trace_variable('w',self.VarChanged_tabCols)
        self.indentBySpaces.trace_variable('w',self.VarChanged_indentBySpaces)
        self.colour.trace_variable('w',self.VarChanged_colour)
        self.keyBinding.trace_variable('w',self.VarChanged_keyBinding)
        self.winWidth.trace_variable('w',self.VarChanged_winWidth)
        self.winHeight.trace_variable('w',self.VarChanged_winHeight)
        self.startupEdit.trace_variable('w',self.VarChanged_startupEdit)
    
    def VarChanged_fontSize(self,*params):
        value=self.fontSize.get()
        self.AddChangedItem('main','EditorWindow','font-size',value)
        
    def VarChanged_fontName(self,*params):
        value=self.fontName.get()
        self.AddChangedItem('main','EditorWindow','font',value)

    def VarChanged_fontBold(self,*params):
        value=self.fontBold.get()
        self.AddChangedItem('main','EditorWindow','font-bold',value)

    def VarChanged_indentBySpaces(self,*params):
        value=self.indentBySpaces.get()
        self.AddChangedItem('main','Indent','use-spaces',value)

    def VarChanged_spaceNum(self,*params):
        value=self.spaceNum.get()
        self.AddChangedItem('main','Indent','num-spaces',value)

    def VarChanged_tabCols(self,*params):
        value=self.tabCols.get()
        self.AddChangedItem('main','Indent','tab-cols',value)

    def VarChanged_colour(self,*params):
        value=self.colour.get()
        theme=self.customTheme.get()
        element=self.themeElements[self.highlightTarget.get()][0]
        self.AddChangedItem('highlight',theme,element,value)
        print params
        
    def VarChanged_keyBinding(self,*params):
        value=self.keyBinding.get()
        keySet=self.customKeys.get()
        event=self.listBindings.get(ANCHOR).split()[0]
        self.AddChangedItem('keys',keySet,event,value)
        print params

    def VarChanged_winWidth(self,*params):
        value=self.winWidth.get()
        self.AddChangedItem('main','EditorWindow','width',value)

    def VarChanged_winHeight(self,*params):
        value=self.winHeight.get()
        self.AddChangedItem('main','EditorWindow','height',value)

    def VarChanged_startupEdit(self,*params):
        value=self.startupEdit.get()
        self.AddChangedItem('main','General','editor-on-startup',value)

    def ExtensionStateToggled(self):
        #callback for the extension enable/disable radio buttons
        value=self.extEnabled.get()
        extension=self.listExt.get(ANCHOR)
        self.AddChangedItem('extensions',extension,'enabled',value)

    def AddChangedItem(self,type,section,item,value):
        value=str(value) #make sure we use a string
        if not self.changedItems[type].has_key(section):
            self.changedItems[type][section]={}    
        self.changedItems[type][section][item]=value
        print type,section,item,value
    
    def GetDefaultItems(self):
        dItems={'main':{},'highlight':{},'keys':{},'extensions':{}}
        for configType in dItems.keys():
            sections=idleConf.GetSectionList('default',configType)
            for section in sections:
                dItems[configType][section]={}
                options=idleConf.defaultCfg[configType].GetOptionList(section)
                for option in options:            
                    dItems[configType][section][option]=(
                            idleConf.defaultCfg[configType].Get(section,option))
        return dItems
            
    def SetThemeType(self):
        if self.themeIsBuiltin.get():
            self.optMenuThemeBuiltin.config(state=NORMAL)
            self.optMenuThemeCustom.config(state=DISABLED)
            self.buttonDeleteCustomTheme.config(state=DISABLED)
        else:
            self.optMenuThemeBuiltin.config(state=DISABLED)
            self.radioThemeCustom.config(state=NORMAL)
            self.optMenuThemeCustom.config(state=NORMAL)
            self.buttonDeleteCustomTheme.config(state=NORMAL)

    def SetKeysType(self):
        if self.keysAreDefault.get():
            self.optMenuKeysBuiltin.config(state=NORMAL)
            self.optMenuKeysCustom.config(state=DISABLED)
            self.buttonDeleteCustomKeys.config(state=DISABLED)
        else:
            self.optMenuKeysBuiltin.config(state=DISABLED)
            self.radioKeysCustom.config(state=NORMAL)
            self.optMenuKeysCustom.config(state=NORMAL)
            self.buttonDeleteCustomKeys.config(state=NORMAL)
    
    def GetNewKeys(self):
        listIndex=self.listBindings.index(ANCHOR)
        binding=self.listBindings.get(listIndex)
        bindName=binding.split()[0] #first part, up to first space
        currentKeySequences=idleConf.GetCurrentKeySet().values()
        newKeys=GetKeysDialog(self,'Get New Keys',bindName,currentKeySequences)
        if newKeys.result: #new keys were specified
            if self.keysAreDefault.get(): #current key set is a built-in
                message=('Your changes will be saved as a new Custom Key Set. '+
                        'Enter a name for your new Custom Key Set below.')
                usedNames=idleConf.GetSectionList('user','keys')
                for newName in self.changedItems['keys'].keys():
                    if newName not in usedNames: usedNames.append(newName)
                newKeySet=GetCfgSectionNameDialog(self,'New Custom Key Set',
                        message,usedNames)
                if not newKeySet.result: #user cancelled custom key set creation
                    self.listBindings.select_set(listIndex)
                    self.listBindings.select_anchor(listIndex)
                    return
                else: #create new custom key set based on previously active key set 
                    self.CreateNewKeySet(newKeySet.result)    
            self.listBindings.delete(listIndex)
            self.listBindings.insert(listIndex,bindName+' - '+newKeys.result)
            self.listBindings.select_set(listIndex)
            self.listBindings.select_anchor(listIndex)
            self.keyBinding.set(newKeys.result)
        else:
            self.listBindings.select_set(listIndex)
            self.listBindings.select_anchor(listIndex)

    def KeyBindingSelected(self,event):
        self.buttonNewKeys.config(state=NORMAL)

    def CreateNewKeySet(self,newKeySetName):
        #creates new custom key set based on the previously active key set,
        #and makes the new key set active
        if self.keysAreDefault.get(): 
            keySetName=self.builtinKeys.get()
        else:  
            keySetName=self.customKeys.get()
        prevKeySet=idleConf.GetKeySet(keySetName)
        #add the new key set to changedItems
        for event in prevKeySet.keys():
            eventName=event[2:-2] #trim off the angle brackets
            self.AddChangedItem('keys',newKeySetName,eventName,
                    prevKeySet[event])
        #change gui over to the new key set
        customKeyList=idleConf.GetSectionList('user','keys')
        for newName in self.changedItems['keys'].keys():
            if newName not in customKeyList: customKeyList.append(newName)
        customKeyList.sort()
        print newKeySetName,customKeyList,self.changedItems['keys'][newKeySetName]
        self.optMenuKeysCustom.SetMenu(customKeyList,newKeySetName)
        self.keysAreDefault.set(0)
        self.SetKeysType()
    
    def GetColour(self):
        target=self.highlightTarget.get()
        rgbTuplet, colourString = tkColorChooser.askcolor(parent=self,
            title='Pick new colour for : '+target,
            initialcolor=self.frameColourSet.cget('bg'))
        if colourString: #user didn't cancel
            if self.themeIsBuiltin.get(): #current theme is a built-in
                message=('Your changes will be saved as a new Custom Theme. '+
                        'Enter a name for your new Custom Theme below.')
                usedNames=idleConf.GetSectionList('user','highlight')
                for newName in self.changedItems['highlight'].keys():
                    if newName not in usedNames: usedNames.append(newName)
                newTheme=GetCfgSectionNameDialog(self,'New Custom Theme',
                        message,usedNames)
                if not newTheme.result: #user cancelled custom theme creation
                    return
                else: #create new custom theme based on previously active theme 
                    self.CreateNewTheme(newTheme.result)    
            self.colour.set(colourString)
            self.frameColourSet.config(bg=colourString)#set sample
            if self.fgHilite.get(): plane='foreground'
            else: plane='background'
            apply(self.textHighlightSample.tag_config,
                (self.themeElements[target][0],),{plane:colourString})
    
    def CreateNewTheme(self,newThemeName):
        #creates new custom theme based on the previously active theme,
        #and makes the new theme active
        if self.themeIsBuiltin.get(): 
            themeType='default'
            themeName=self.builtinTheme.get()
        else:  
            themeType='user'
            themeName=self.customTheme.get()
        newTheme=idleConf.GetThemeDict(themeType,themeName)
        #add the new theme to changedItems
        self.changedItems['highlight'][newThemeName]=newTheme    
        #change gui over to the new theme
        customThemeList=idleConf.GetSectionList('user','highlight')
        for newName in self.changedItems['highlight'].keys():
            if newName not in customThemeList: customThemeList.append(newName)
        customThemeList.sort()
        print newThemeName,customThemeList,newTheme
        self.optMenuThemeCustom.SetMenu(customThemeList,newThemeName)
        self.themeIsBuiltin.set(0)
        self.SetThemeType()
    
    def OnListFontButtonRelease(self,event):
        self.fontName.set(self.listFontName.get(ANCHOR))
        self.SetFontSample()
        
    def SetFontSample(self,event=None):
        fontName=self.fontName.get()
        if self.fontBold.get(): 
            fontWeight=tkFont.BOLD
        else: 
            fontWeight=tkFont.NORMAL
        self.editFont.config(size=self.fontSize.get(),
                weight=fontWeight,family=fontName)

    def SetHighlightTargetBinding(self,*args):
        self.SetHighlightTarget()
        
    def SetHighlightTarget(self):
        if self.highlightTarget.get()=='Cursor': #bg not possible
            self.radioFg.config(state=DISABLED)
            self.radioBg.config(state=DISABLED)
            self.fgHilite.set(1)
        else: #both fg and bg can be set
            self.radioFg.config(state=NORMAL)
            self.radioBg.config(state=NORMAL)
            self.fgHilite.set(1)
        self.SetColourSample()
    
    def SetColourSampleBinding(self,*args):
        self.SetColourSample()
        
    def SetColourSample(self):
        #set the colour smaple area
        tag=self.themeElements[self.highlightTarget.get()][0]
        if self.fgHilite.get(): plane='foreground'
        else: plane='background'
        colour=self.textHighlightSample.tag_cget(tag,plane)
        self.frameColourSet.config(bg=colour)
    
    def PaintThemeSample(self):
        if self.themeIsBuiltin.get(): #a default theme
            theme=self.builtinTheme.get()
        else: #a user theme
            theme=self.customTheme.get()
        for element in self.themeElements.keys():
            colours=idleConf.GetHighlight(theme, self.themeElements[element][0])
            if element=='Cursor': #cursor sample needs special painting
                colours['background']=idleConf.GetHighlight(theme, 
                        'normal', fgBg='bg')
            apply(self.textHighlightSample.tag_config,
                (self.themeElements[element][0],),colours)
    
    def LoadFontCfg(self):
        ##base editor font selection list
        fonts=list(tkFont.families(self))
        fonts.sort()
        for font in fonts:
            self.listFontName.insert(END,font)
        configuredFont=idleConf.GetOption('main','EditorWindow','font',
                default='courier')
        self.fontName.set(configuredFont)
        if configuredFont in fonts:
            currentFontIndex=fonts.index(configuredFont)
            self.listFontName.see(currentFontIndex)
            self.listFontName.select_set(currentFontIndex)
            self.listFontName.select_anchor(currentFontIndex)
        ##font size dropdown
        fontSize=idleConf.GetOption('main','EditorWindow','font-size',
                default='12')
        self.optMenuFontSize.SetMenu(('10','11','12','13','14',
                '16','18','20','22'),fontSize )
        ##fontWeight
        self.fontBold.set(idleConf.GetOption('main','EditorWindow',
                'font-bold',default=0,type='bool'))
        ##font sample 
        self.SetFontSample()
    
    def LoadTabCfg(self):
        ##indent type radibuttons
        spaceIndent=idleConf.GetOption('main','Indent','use-spaces',
                default=1,type='bool')
        self.indentBySpaces.set(spaceIndent)
        ##indent sizes
        spaceNum=idleConf.GetOption('main','Indent','num-spaces',
                default=4,type='int')
        tabCols=idleConf.GetOption('main','Indent','tab-cols',
                default=4,type='int')
        self.spaceNum.set(spaceNum)
        self.tabCols.set(tabCols)
    
    def LoadThemeCfg(self):
        ##current theme type radiobutton
        self.themeIsBuiltin.set(idleConf.GetOption('main','Theme','default',
            type='bool',default=1))
        ##currently set theme
        currentOption=idleConf.CurrentTheme()
        ##load available theme option menus
        if self.themeIsBuiltin.get(): #default theme selected
            itemList=idleConf.GetSectionList('default','highlight')
            itemList.sort()
            self.optMenuThemeBuiltin.SetMenu(itemList,currentOption)
            itemList=idleConf.GetSectionList('user','highlight')
            itemList.sort()
            if not itemList:
                self.radioThemeCustom.config(state=DISABLED)
                self.customTheme.set('- no custom themes -')    
            else:
                self.optMenuThemeCustom.SetMenu(itemList,itemList[0])
        else: #user theme selected
            itemList=idleConf.GetSectionList('user','highlight')
            itemList.sort()
            self.optMenuThemeCustom.SetMenu(itemList,currentOption)
            itemList=idleConf.GetSectionList('default','highlight')
            itemList.sort()
            self.optMenuThemeBuiltin.SetMenu(itemList,itemList[0])
        self.SetThemeType()
        ##load theme element option menu
        themeNames=self.themeElements.keys()
        themeNames.sort(self.__ThemeNameIndexCompare)
        self.optMenuHighlightTarget.SetMenu(themeNames,themeNames[0])   
        self.PaintThemeSample()
        self.SetHighlightTarget()
    
    def __ThemeNameIndexCompare(self,a,b):
        if self.themeElements[a][1]<self.themeElements[b][1]: return -1
        elif self.themeElements[a][1]==self.themeElements[b][1]: return 0
        else: return 1
    
    def LoadKeyCfg(self):
        ##current keys type radiobutton
        self.keysAreDefault.set(idleConf.GetOption('main','Keys','default',
            type='bool',default=1))
        ##currently set keys
        currentOption=idleConf.CurrentKeys()
        ##load available keyset option menus
        if self.keysAreDefault.get(): #default theme selected
            itemList=idleConf.GetSectionList('default','keys')
            itemList.sort()
            self.optMenuKeysBuiltin.SetMenu(itemList,currentOption)
            itemList=idleConf.GetSectionList('user','keys')
            itemList.sort()
            if not itemList:
                self.radioKeysCustom.config(state=DISABLED)    
                self.customKeys.set('- no custom keys -')    
            else:
                self.optMenuKeysCustom.SetMenu(itemList,itemList[0])
        else: #user theme selected
            itemList=idleConf.GetSectionList('user','keys')
            itemList.sort()
            self.optMenuKeysCustom.SetMenu(itemList,currentOption)
            itemList=idleConf.GetSectionList('default','keys')
            itemList.sort()
            self.optMenuKeysBuiltin.SetMenu(itemList,itemList[0])
        self.SetKeysType()   
        ##load keyset element list
        keySet=idleConf.GetCurrentKeySet()
        bindNames=keySet.keys()
        bindNames.sort()
        for bindName in bindNames: 
            key=string.join(keySet[bindName]) #make key(s) into a string
            bindName=bindName[2:-2] #trim off the angle brackets
            self.listBindings.insert(END, bindName+' - '+key)
   
    def LoadGeneralCfg(self):
        #startup state
        self.startupEdit.set(idleConf.GetOption('main','General',
                'editor-on-startup',default=1,type='bool'))
        #initial window size
        self.winWidth.set(idleConf.GetOption('main','EditorWindow','width'))       
        self.winHeight.set(idleConf.GetOption('main','EditorWindow','height'))
        #extensions    
        extns=idleConf.GetExtensions(activeOnly=0)
        apply(self.listExt.insert,(END,)+tuple(extns))
    
    def ExtensionSelected(self,event):
        self.radioEnableExt.config(state=NORMAL)
        self.radioDisableExt.config(state=NORMAL)
        self.buttonExtConfig.config(state=NORMAL)
        extn=self.listExt.get(ANCHOR)
        self.extEnabled.set(idleConf.GetOption('extensions',extn,'enable',
                default=1,type='bool'))
    
    def LoadConfigs(self):
        """
        load configuration from default and user config files and populate
        the widgets on the config dialog pages.
        """
        ### fonts / tabs page
        self.LoadFontCfg()        
        self.LoadTabCfg()        
        ### highlighting page
        self.LoadThemeCfg()
        ### keys page
        self.LoadKeyCfg()
        ### help page
        ### general page
        self.LoadGeneralCfg()
        
    def SetUserValue(self,configType,section,item,value):
        print idleConf.defaultCfg[configType].Get(section,item),value
        if idleConf.defaultCfg[configType].has_option(section,item):
            if idleConf.defaultCfg[configType].Get(section,item)==value:
                #the setting equals a default setting, remove it from user cfg
                return idleConf.userCfg[configType].RemoveOption(section,item)
        #if we got here set the option
        return idleConf.userCfg[configType].SetOption(section,item,value)
            
    def SaveConfigs(self):
        """
        save configuration changes to user config files.
        """
        for configType in self.changedItems.keys():
            cfgTypeHasChanges=0
            for section in self.changedItems[configType].keys():
                for item in self.changedItems[configType][section].keys():
                    value=self.changedItems[configType][section][item]
                    print configType,section,item,value
                    if self.SetUserValue(configType,section,item,value):
                        cfgTypeHasChanges=1
            if cfgTypeHasChanges: 
                idleConf.userCfg[configType].Save()                
    
    def Cancel(self):
        self.destroy()

    def Ok(self):
        self.Apply()
        self.destroy()

    def Apply(self):
        self.SaveConfigs()

    def Help(self):
        pass

if __name__ == '__main__':
    #test the dialog
    root=Tk()
    Button(root,text='Dialog',
            command=lambda:ConfigDialog(root,'Settings')).pack()
    root.mainloop()
