##---------------------------------------------------------------------------##
##
## idle - configuration data handler, based on and replacing IdleConfig.py 
## elguavas
## 
##---------------------------------------------------------------------------##
"""Provides access to configuration information"""

import os
import sys
from ConfigParser import ConfigParser, NoOptionError, NoSectionError

class IdleConfParser(ConfigParser):
    """
    A ConfigParser specialised for idle configuration file handling
    """
    def __init__(self, cfgFile, cfgDefaults=None):
        """
        cfgFile - string, fully specified configuration file name
        """
        self.file=cfgFile
        ConfigParser.__init__(self,defaults=cfgDefaults)
    
    def GetInt(self, section, option):
        """
        Get an option value as an integer
        """
        return self.Get(section, option, type='int')

    def GetBool(self, section, option):
        """
        Get an option value as a boolean
        """
        return self.Get(section, option, type='bool')
        
    def Get(self, section, option, raw=0, vars=None, default=None, 
            type=None):
        """
        Get an option value for given section/option or return default.
        If type is specified, return as type.
        """
        if type=='bool': getVal=self.getbool
        elif type=='int': getVal=self.getint
        else: getVal=self.get
        if self.has_option(section,option):
            return getVal(section, option, raw, vars)
        else:
            return default

    def GetSectionList(self):
        # only provided for consistency
        return self.sections()
    
    def GetOptionList(self,section):
        """
        Get an option list for given section
        """
        if self.has_section:
            return self.options(section)
        else:  #return a default value
            return []

    def GetHighlight(self, theme, element):
        fore = self.Get(theme, element + "-foreground")
        back = self.Get(theme, element + "-background")
        style = self.Ge(theme, element + "-fontStyle", default='')
        return {"fg": fore,
                "bg": back,
                "fStyle": style}

    def Load(self):
        """ 
        Load the configuration file from disk 
        """
        self.read(self.file)
        
class IdleUserConfParser(IdleConfParser):
    """
    IdleConfigParser specialised for user configuration handling
    """
    def Save(self):
        """
        write loaded user configuration file back to disk
        """
        # this is a user config, it can be written to disk
        self.write()

class IdleConf:
    """
    holds config parsers for all idle config files:
    default config files
        (idle install dir)/config-main.def
        (idle install dir)/config-extensions.def
        (idle install dir)/config-highlight.def
        (idle install dir)/config-keys.def
    user config  files
        (user home dir)/.idlerc/idle-main.cfg
        (user home dir)/.idlerc/idle-extensions.cfg
        (user home dir)/.idlerc/idle-highlight.cfg
        (user home dir)/.idlerc/idle-keys.cfg
    """
    def __init__(self):
        self.defaultCfg={}
        self.userCfg={}
        self.cfg={}
        self.CreateConfigHandlers()
        self.LoadCfgFiles()
        #self.LoadCfg()
            
    def CreateConfigHandlers(self):
        """
        set up a dictionary config parsers for default and user 
        configurations respectively
        """
        #build idle install path
        if __name__ != '__main__': # we were imported
            idledir=os.path.dirname(__file__)
        else: # we were exec'ed (for testing only)
            idledir=os.path.abspath(sys.path[0])
        #print idledir
        try: #build user home path
            userdir = os.environ['HOME'] #real home directory
        except KeyError:
            userdir = os.getcwd() #hack for os'es without real homedirs
        userdir=os.path.join(userdir,'.idlerc')
        #print userdir
        if not os.path.exists(userdir):
            os.mkdir(userdir)
        configTypes=('main','extensions','highlight','keys')
        defCfgFiles={}
        usrCfgFiles={}
        for cfgType in configTypes: #build config file names
            defCfgFiles[cfgType]=os.path.join(idledir,'config-'+cfgType+'.def')                    
            usrCfgFiles[cfgType]=os.path.join(userdir,'idle-'+cfgType+'.cfg')                    
        for cfgType in configTypes: #create config parsers
            self.defaultCfg[cfgType]=IdleConfParser(defCfgFiles[cfgType])
            self.userCfg[cfgType]=IdleUserConfParser(usrCfgFiles[cfgType])
    
    def LoadCfgFiles(self):
        """ 
        load all configuration files.
        """
        for key in self.defaultCfg.keys():
            self.defaultCfg[key].Load()                    
            self.userCfg[key].Load() #same keys                    

    def SaveUserCfgFiles(self):
        """
        write all loaded user configuration files back to disk
        """
        for key in self.userCfg.keys():
            self.userCfg[key].Save()    

idleConf=IdleConf()

### module test
if __name__ == '__main__':
    def dumpCfg(cfg):
        print '\n',cfg,'\n'
        for key in cfg.keys():
            sections=cfg[key].sections()
            print key
            print sections
            for section in sections:
                options=cfg[key].options(section)
                print section    
                print options
                for option in options:
                    print option, '=', cfg[key].Get(section,option)
    dumpCfg(idleConf.defaultCfg)
    dumpCfg(idleConf.userCfg)
    print idleConf.userCfg['main'].Get('Theme','name')
    #print idleConf.userCfg['highlight'].GetDefHighlight('Foo','normal')
