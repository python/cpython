"""
about box for idle
"""
from Tkinter import *
import tkFont
import string, os
import textView
import idlever
class AboutDialog(Toplevel):
    """
    modal about dialog for idle
    """ 
    def __init__(self,parent,title):
        Toplevel.__init__(self, parent)
        self.configure(borderwidth=5)
        self.geometry("+%d+%d" % (parent.winfo_rootx()+30,
                parent.winfo_rooty()+30))
        self.bg="#707070"
        self.fg="#ffffff"
        
        self.CreateWidgets()
        self.resizable(height=FALSE,width=FALSE)
        self.title(title)
        self.transient(parent)
        self.grab_set()
        self.protocol("WM_DELETE_WINDOW", self.Ok)
        self.parent = parent
        self.buttonOk.focus_set()
        #key bindings for this dialog
        self.bind('<Alt-c>',self.CreditsButtonBinding) #credits button
        self.bind('<Alt-l>',self.LicenseButtonBinding) #license button
        self.bind('<Return>',self.Ok) #dismiss dialog
        self.bind('<Escape>',self.Ok) #dismiss dialog
        self.wait_window()
        
    def CreateWidgets(self):
        frameMain = Frame(self,borderwidth=2,relief=SUNKEN)
        frameButtons = Frame(self)
        frameButtons.pack(side=BOTTOM,fill=X)
        frameMain.pack(side=TOP,expand=TRUE,fill=BOTH)
        self.buttonOk = Button(frameButtons,text='Ok',
                command=self.Ok)#,default=ACTIVE
        self.buttonOk.pack(padx=5,pady=5)
        #self.picture = Image('photo',data=self.pictureData)
        frameBg = Frame(frameMain,bg=self.bg)
        frameBg.pack(expand=TRUE,fill=BOTH)
        labelTitle = Label(frameBg,text='IDLEfork',fg=self.fg,bg=self.bg,
                font=('courier', 24, 'bold'))
        labelTitle.grid(row=0,column=0,sticky=W,padx=10,pady=10)
        #labelPicture = Label(frameBg,text='[picture]')
        #image=self.picture,bg=self.bg)
        #labelPicture.grid(row=0,column=1,sticky=W,rowspan=2,padx=0,pady=3)
        labelVersion = Label(frameBg,text='version  '+idlever.IDLE_VERSION,
                fg=self.fg,bg=self.bg)
        labelVersion.grid(row=1,column=0,sticky=W,padx=10,pady=5)
        labelDesc = Label(frameBg,
                text="A development version of Python's lightweight\n"+
                'Integrated DeveLopment Environment, IDLE.',
                justify=LEFT,fg=self.fg,bg=self.bg)
        labelDesc.grid(row=2,column=0,sticky=W,columnspan=3,padx=10,pady=5)
        labelCopyright = Label(frameBg,
                text="Copyright (c) 2001 Python Software Foundation;\nAll Rights Reserved",
                justify=LEFT,fg=self.fg,bg=self.bg)
        labelCopyright.grid(row=3,column=0,sticky=W,columnspan=3,padx=10,pady=5)
        labelLicense = Label(frameBg,
                text='Released under the Python 2.1.1 PSF Licence',
                justify=LEFT,fg=self.fg,bg=self.bg)
        labelLicense.grid(row=4,column=0,sticky=W,columnspan=3,padx=10,pady=5)
        framePad = Frame(frameBg,height=5,bg=self.bg).grid(row=5,column=0)
        labelEmail = Label(frameBg,text='email:  idle-dev@python.org',
                justify=LEFT,fg=self.fg,bg=self.bg)
        labelEmail.grid(row=6,column=0,columnspan=2,sticky=W,padx=10,pady=0)
        labelWWW = Label(frameBg,text='www:  http://idlefork.sourceforge.net',
                justify=LEFT,fg=self.fg,bg=self.bg)
        labelWWW.grid(row=7,column=0,columnspan=2,sticky=W,padx=10,pady=0)
        frameDivider = Frame(frameBg,borderwidth=1,relief=SUNKEN,
                height=2,bg=self.bg).grid(row=8,column=0,sticky=(E,W),columnspan=3,
                padx=5,pady=5)
        labelPythonVer = Label(frameBg,text='Python version:  '+
                sys.version.split()[0],fg=self.fg,bg=self.bg)
        labelPythonVer.grid(row=9,column=0,sticky=W,padx=10,pady=0)
        #handle weird tk version num in windoze python >= 1.6 (?!?)
        tkVer = `TkVersion`.split('.')
        tkVer[len(tkVer)-1] = str('%.3g' % (float('.'+tkVer[len(tkVer)-1])))[2:]
        if tkVer[len(tkVer)-1] == '': 
            tkVer[len(tkVer)-1] = '0'
        tkVer = string.join(tkVer,'.')
        labelTkVer = Label(frameBg,text='Tk version:  '+
                tkVer,fg=self.fg,bg=self.bg)
        labelTkVer.grid(row=9,column=1,sticky=W,padx=2,pady=0)

        self.buttonLicense = Button(frameBg,text='View License',underline=5,
                width=14,highlightbackground=self.bg,command=self.ShowLicense)#takefocus=FALSE
        self.buttonLicense.grid(row=10,column=0,sticky=W,padx=10,pady=10)
        self.buttonCredits = Button(frameBg,text='View Credits',underline=5,
                width=14,highlightbackground=self.bg,command=self.ShowCredits)#takefocus=FALSE
        self.buttonCredits.grid(row=10,column=1,columnspan=2,sticky=E,padx=10,pady=10)

    def CreditsButtonBinding(self,event):
        self.buttonCredits.invoke()

    def LicenseButtonBinding(self,event):
        self.buttonLicense.invoke()

    def ShowLicense(self):
        self.ViewFile('About - License','LICENSE.txt')
        
    def ShowCredits(self):
        self.ViewFile('About - Credits','CREDITS.txt')

    def ViewFile(self,viewTitle,viewFile):
        fn=os.path.join(os.path.abspath(os.path.dirname(__file__)),viewFile)
        textView.TextViewer(self,viewTitle,fn)

    def Ok(self, event=None):
        self.destroy()
    
if __name__ == '__main__':
    #test the dialog
    root=Tk()
    def run():
        import aboutDialog
        aboutDialog.AboutDialog(root,'About')
    Button(root,text='Dialog',command=run).pack()
    root.mainloop()
