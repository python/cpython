"""
dialog for building tkinter accelerator key bindings 
"""
from Tkinter import *
import tkMessageBox
import string, os

class GetAccelDialog(Toplevel):
    def __init__(self,parent,title,action,keySequenceStr):
        Toplevel.__init__(self, parent)
        self.configure(borderwidth=5)
        self.resizable(height=FALSE,width=FALSE)
        self.title(title)
        self.transient(parent)
        self.grab_set()
        self.protocol("WM_DELETE_WINDOW", self.Cancel)
        self.parent = parent
        self.action=action
        self.keySequenceStr=keySequenceStr
        self.keyString=StringVar(self)
        self.keyString.set('')
        self.keyCtrl=StringVar(self)
        self.keyCtrl.set('')
        self.keyAlt=StringVar(self)
        self.keyAlt.set('')
#         self.keyMeta=StringVar(self)
#         self.keyMeta.set('')
        self.keyShift=StringVar(self)
        self.keyShift.set('')
#         self.keyFinal1=StringVar(self)
#         self.keyFinal1.set('')
#         self.keyFinal2=StringVar(self)
#         self.keyFinal2.set('')
#         self.keyFn1=IntVar(self)
#         self.keyFn2=IntVar(self)
        self.CreateWidgets()
        self.LoadFinalKeyList()
        #self.buttonOk.focus_set()
        self.withdraw() #hide while setting geometry
        self.update_idletasks()
        self.geometry("+%d+%d" % 
            ((parent.winfo_rootx()+((parent.winfo_width()/2)
                -(self.winfo_reqwidth()/2)),
              parent.winfo_rooty()+((parent.winfo_height()/2)
                -(self.winfo_reqheight()/2)) )) ) #centre dialog over parent
        self.deiconify() #geometry set, unhide
        self.wait_window()
        
    def CreateWidgets(self):
        frameMain = Frame(self,borderwidth=2,relief=SUNKEN)
        frameMain.pack(side=TOP,expand=TRUE,fill=BOTH)
        frameButtons=Frame(self)
        frameButtons.pack(side=BOTTOM,fill=X)
        self.buttonOk = Button(frameButtons,text='Ok',
                width=8,command=self.Ok)
        self.buttonOk.grid(row=0,column=0,padx=5,pady=5)
        self.buttonCancel = Button(frameButtons,text='Cancel',
                width=8,command=self.Cancel)
        self.buttonCancel.grid(row=0,column=1,padx=5,pady=5)
        self.frameKeySeqBasic = Frame(frameMain)
        self.frameKeySeqAdvanced = Frame(frameMain)
        self.frameControlsBasic = Frame(frameMain)
        self.frameHelpAdvanced = Frame(frameMain)
        self.frameKeySeqAdvanced.grid(row=0,column=0,sticky=NSEW,padx=5,pady=5)
        self.frameKeySeqBasic.grid(row=0,column=0,sticky=NSEW,padx=5,pady=5)
        self.frameKeySeqBasic.lift()
        self.frameHelpAdvanced.grid(row=1,column=0,sticky=NSEW,padx=5)
        self.frameControlsBasic.grid(row=1,column=0,sticky=NSEW,padx=5)
        self.frameControlsBasic.lift()
        self.buttonLevel = Button(frameMain,command=self.ToggleLevel,
                text='Advanced Key Binding Entry >>')
        self.buttonLevel.grid(row=2,column=0,stick=EW,padx=5,pady=5)
        labelTitleBasic = Label(self.frameKeySeqBasic,
                text="New keys for  '"+self.action+"' :")
        labelTitleBasic.pack(anchor=W)
        labelKeysBasic = Label(self.frameKeySeqBasic,justify=LEFT,
                textvariable=self.keyString,relief=GROOVE,borderwidth=2)
        labelKeysBasic.pack(ipadx=5,ipady=5,fill=X)
        checkCtrl=Checkbutton(self.frameControlsBasic,
                command=self.BuildKeyString,
                text='Ctrl',variable=self.keyCtrl,onvalue='Control',offvalue='')
        checkCtrl.grid(row=0,column=0,padx=2,sticky=W)
        checkAlt=Checkbutton(self.frameControlsBasic,
                command=self.BuildKeyString,
                text='Alt',variable=self.keyAlt,onvalue='Alt',offvalue='')
        checkAlt.grid(row=0,column=1,padx=2,sticky=W)
#         checkMeta=Checkbutton(self.frameControlsBasic,
#         command=self.BuildKeyString,
#                 text='Meta',variable=self.keyMeta,onvalue='Meta',offvalue='')
#         checkMeta.grid(row=0,column=2,padx=2,sticky=W)
        checkShift=Checkbutton(self.frameControlsBasic,
                command=self.BuildKeyString,
                text='Shift',variable=self.keyShift,onvalue='Shift',offvalue='')
        checkShift.grid(row=0,column=3,padx=2,sticky=W)
        labelFnAdvice=Label(self.frameControlsBasic,justify=LEFT,
                text="Select the desired modifier\n"+
                     "keys above, and final key\n"+
#                      "keys above, and final key(s)\n"+
                     "from the list on the right.")
        labelFnAdvice.grid(row=1,column=0,columnspan=4,padx=2,sticky=W)
        self.listKeysFinal=Listbox(self.frameControlsBasic,width=15,height=10,
                selectmode=SINGLE)
#                 selectmode=MULTIPLE)
        self.listKeysFinal.bind('<ButtonRelease-1>',self.FinalKeySelected)
        self.listKeysFinal.grid(row=0,column=4,rowspan=4,sticky=NS)
        scrollKeysFinal=Scrollbar(self.frameControlsBasic,orient=VERTICAL,
                command=self.listKeysFinal.yview)
        self.listKeysFinal.config(yscrollcommand=scrollKeysFinal.set)
        scrollKeysFinal.grid(row=0,column=5,rowspan=4,sticky=NS)
        self.buttonAddNew=Button(self.frameControlsBasic,
                text='Accept Key Sequence',width=25,command=None)
        self.buttonAddNew.grid(row=2,column=0,columnspan=4)
        self.buttonClearLast=Button(self.frameControlsBasic,
                text='Clear Last Key Sequence',width=25,
                command=self.ClearLastKeySeq)
        self.buttonClearLast.grid(row=3,column=0,columnspan=4)
        labelTitleAdvanced = Label(self.frameKeySeqAdvanced,justify=LEFT,
                text="Enter new binding(s) for  '"+self.action+"' :\n"+
                "(will not be checked for validity)")
        labelTitleAdvanced.pack(anchor=W)
        self.entryKeysAdvanced=Entry(self.frameKeySeqAdvanced,
                textvariable=self.keyString)
        self.entryKeysAdvanced.pack(fill=X)
        labelHelpAdvanced=Label(self.frameHelpAdvanced,justify=LEFT,
            text="Key bindings are specified using tkinter key id's as\n"+
                 "in these samples: <Control-f>, <Shift-F2>, <F12>,\n"
                 "<Control-space>, <Meta-less>, <Control-Alt-Shift-x>.\n\n"+
                 "'Emacs style' multi-keystroke bindings are specified as\n"+
                 "follows: <Control-x><Control-y> or <Meta-f><Meta-g>.\n\n"+
                 "Multiple separate bindings for one action should be\n"+
                 "separated by a space, eg., <Alt-v> <Meta-v>." )
        labelHelpAdvanced.grid(row=0,column=0,sticky=NSEW)

    def ToggleLevel(self):
        if  self.buttonLevel.cget('text')[:8]=='Advanced':
            self.buttonLevel.config(text='<< Basic Key Binding Entry')
            self.frameKeySeqAdvanced.lift()
            self.frameHelpAdvanced.lift()
            self.entryKeysAdvanced.focus_set()
        else:
            self.buttonLevel.config(text='Advanced Key Binding Entry >>')
            self.frameKeySeqBasic.lift()
            self.frameControlsBasic.lift()    
    
    def FinalKeySelected(self,event):
        self.BuildKeyString()
        
    def BuildKeyString(self):
        keyList=[]
        modifiers=self.GetModifiers(self.keyCtrl.get(),self.keyAlt.get(),
                self.keyShift.get())
        finalKey=self.listKeysFinal.get(ANCHOR)
        if modifiers: modifiers[0]='<'+modifiers[0]
        keyList=keyList+modifiers
        if finalKey: 
            if (not modifiers) and (finalKey in self.functionKeys):
                finalKey='<'+finalKey
            keyList.append(finalKey+'>')
                
        keyStr=string.join(keyList,'-')
        self.keyString.set(keyStr)
        


#         if (finalKey not in self.functionKeys) and (not modifiers):
#             tkMessageBox.showerror(title='Key Binding Error',
#                     message='At least one modifier key should be specified.')
#             return
        
    def GetModifiers(self,ctrl,alt,shift):
        modList=[]
        if ctrl: modList.append(ctrl)
        if alt: modList.append(alt)
        if shift: modList.append(shift)
        return modList

    def ClearLastKeySeq(self):
        pass
    
    def LoadFinalKeyList(self):
        #make a tuple of most of the useful common 'final' keys
        self.functionKeys=('F1','F2','F2','F4','F5','F6','F7','F8','F9',
                'F10','F11','F12')
        keys=(tuple(string.ascii_lowercase+string.digits+
               '~!@#%^&*()_-+={}[]|;:,./?')+('tab','space')+self.functionKeys)
        apply(self.listKeysFinal.insert,
            (END,)+keys)
    
    def Ok(self, event=None):
        if self.KeysOk():
            self.keySequenceStr=self.keyDisplay.get()
            self.destroy()
    
    def Cancel(self, event=None):
        self.keySequenceStr=''
        self.destroy()
    
if __name__ == '__main__':
    #test the dialog
    root=Tk()
    def run():
        #import aboutDialog
        #aboutDialog.AboutDialog(root,'About')
        keySeq=''
        GetAccelDialog(root,'Get Keys','find-again',keySeq)
        print keySeq
    Button(root,text='Dialog',command=run).pack()
    root.mainloop()
