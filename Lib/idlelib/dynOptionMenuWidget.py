##---------------------------------------------------------------------------##
##
## idle - tkinter OptionMenu widget modified to allow dynamic
##        reconfiguration of menu. 
## elguavas
## 
##---------------------------------------------------------------------------##
"""
OptionMenu widget modified to allow dynamic menu reconfiguration
"""
from Tkinter import OptionMenu
from Tkinter import _setit

class DynOptionMenu(OptionMenu):
    """
    OptionMenu widget that allows dynamic menu reconfiguration
    """
    def __init__(self, master, variable, value, *values, **kwargs):
        OptionMenu.__init__(self, master, variable, value, *values, **kwargs)
        #self.menu=self['menu']
        self.variable=variable
        self.command=kwargs.get('command')
    
    def SetMenu(self,valueList,value):
        """
        clear and reload the menu with a new set of options.
        valueList - list of new options
        value - initial value to set the optionmenu's menubutton to 
        """
        self['menu'].delete(0,'end')
        for item in valueList:
            self['menu'].add_command(label=item,
                    command=_setit(self.variable,item,self.command))
        self.variable.set(value)
