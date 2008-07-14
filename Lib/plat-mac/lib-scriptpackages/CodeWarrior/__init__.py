"""
Package generated from /Volumes/Sap/Applications (Mac OS 9)/Metrowerks CodeWarrior 7.0/Metrowerks CodeWarrior/CodeWarrior IDE 4.2.5
"""

from warnings import warnpy3k
warnpy3k("In 3.x, the CodeWarrior package is removed.", stacklevel=2)

import aetools
Error = aetools.Error
import CodeWarrior_suite
import Standard_Suite
import Metrowerks_Shell_Suite
import Required


_code_to_module = {
    'CWIE' : CodeWarrior_suite,
    'CoRe' : Standard_Suite,
    'MMPR' : Metrowerks_Shell_Suite,
    'reqd' : Required,
}



_code_to_fullname = {
    'CWIE' : ('CodeWarrior.CodeWarrior_suite', 'CodeWarrior_suite'),
    'CoRe' : ('CodeWarrior.Standard_Suite', 'Standard_Suite'),
    'MMPR' : ('CodeWarrior.Metrowerks_Shell_Suite', 'Metrowerks_Shell_Suite'),
    'reqd' : ('CodeWarrior.Required', 'Required'),
}

from CodeWarrior_suite import *
from Standard_Suite import *
from Metrowerks_Shell_Suite import *
from Required import *

def getbaseclasses(v):
    if not getattr(v, '_propdict', None):
        v._propdict = {}
        v._elemdict = {}
        for superclassname in getattr(v, '_superclassnames', []):
            superclass = eval(superclassname)
            getbaseclasses(superclass)
            v._propdict.update(getattr(superclass, '_propdict', {}))
            v._elemdict.update(getattr(superclass, '_elemdict', {}))
        v._propdict.update(getattr(v, '_privpropdict', {}))
        v._elemdict.update(getattr(v, '_privelemdict', {}))

import StdSuites

#
# Set property and element dictionaries now that all classes have been defined
#
getbaseclasses(character)
getbaseclasses(selection_2d_object)
getbaseclasses(application)
getbaseclasses(document)
getbaseclasses(text)
getbaseclasses(window)
getbaseclasses(file)
getbaseclasses(line)
getbaseclasses(insertion_point)
getbaseclasses(single_class_browser)
getbaseclasses(project_document)
getbaseclasses(symbol_browser)
getbaseclasses(editor_document)
getbaseclasses(file_compare_document)
getbaseclasses(class_browser)
getbaseclasses(subtarget)
getbaseclasses(message_document)
getbaseclasses(project_inspector)
getbaseclasses(text_document)
getbaseclasses(catalog_document)
getbaseclasses(class_hierarchy)
getbaseclasses(target)
getbaseclasses(build_progress_document)
getbaseclasses(target_file)
getbaseclasses(ToolServer_worksheet)
getbaseclasses(single_class_hierarchy)
getbaseclasses(File_Mapping)
getbaseclasses(browser_catalog)
getbaseclasses(Build_Settings)
getbaseclasses(ProjectFile)
getbaseclasses(VCS_Setup)
getbaseclasses(data_member)
getbaseclasses(Shielded_Folder)
getbaseclasses(Custom_Keywords)
getbaseclasses(Path_Information)
getbaseclasses(Segment)
getbaseclasses(Source_Tree)
getbaseclasses(Access_Paths)
getbaseclasses(Debugger_Windowing)
getbaseclasses(Relative_Path)
getbaseclasses(Environment_Variable)
getbaseclasses(base_class)
getbaseclasses(Debugger_Display)
getbaseclasses(Build_Extras)
getbaseclasses(Error_Information)
getbaseclasses(Editor)
getbaseclasses(Shielded_Folders)
getbaseclasses(Extras)
getbaseclasses(File_Mappings)
getbaseclasses(Function_Information)
getbaseclasses(Debugger_Target)
getbaseclasses(Syntax_Coloring)
getbaseclasses(class_)
getbaseclasses(Global_Source_Trees)
getbaseclasses(Target_Settings)
getbaseclasses(Debugger_Global)
getbaseclasses(member_function)
getbaseclasses(Runtime_Settings)
getbaseclasses(Plugin_Settings)
getbaseclasses(Browser_Coloring)
getbaseclasses(Font)
getbaseclasses(Target_Source_Trees)

#
# Indices of types declared in this module
#
_classdeclarations = {
    'cha ' : character,
    'csel' : selection_2d_object,
    'capp' : application,
    'docu' : document,
    'ctxt' : text,
    'cwin' : window,
    'file' : file,
    'clin' : line,
    'cins' : insertion_point,
    '1BRW' : single_class_browser,
    'PRJD' : project_document,
    'SYMB' : symbol_browser,
    'EDIT' : editor_document,
    'COMP' : file_compare_document,
    'BROW' : class_browser,
    'SBTG' : subtarget,
    'MSSG' : message_document,
    'INSP' : project_inspector,
    'TXTD' : text_document,
    'CTLG' : catalog_document,
    'HIER' : class_hierarchy,
    'TRGT' : target,
    'PRGS' : build_progress_document,
    'SRCF' : target_file,
    'TOOL' : ToolServer_worksheet,
    '1HIR' : single_class_hierarchy,
    'FMap' : File_Mapping,
    'Cata' : browser_catalog,
    'BSTG' : Build_Settings,
    'SrcF' : ProjectFile,
    'VCSs' : VCS_Setup,
    'DtMb' : data_member,
    'SFit' : Shielded_Folder,
    'CUKW' : Custom_Keywords,
    'PInf' : Path_Information,
    'Seg ' : Segment,
    'SrcT' : Source_Tree,
    'PATH' : Access_Paths,
    'DbWN' : Debugger_Windowing,
    'RlPt' : Relative_Path,
    'EnvV' : Environment_Variable,
    'BsCl' : base_class,
    'DbDS' : Debugger_Display,
    'LXTR' : Build_Extras,
    'ErrM' : Error_Information,
    'EDTR' : Editor,
    'SHFL' : Shielded_Folders,
    'GXTR' : Extras,
    'FLMP' : File_Mappings,
    'FDef' : Function_Information,
    'DbTG' : Debugger_Target,
    'SNTX' : Syntax_Coloring,
    'Clas' : class_,
    'GSTs' : Global_Source_Trees,
    'TARG' : Target_Settings,
    'DbGL' : Debugger_Global,
    'MbFn' : member_function,
    'RSTG' : Runtime_Settings,
    'PSTG' : Plugin_Settings,
    'BRKW' : Browser_Coloring,
    'mFNT' : Font,
    'TSTs' : Target_Source_Trees,
}


class CodeWarrior(CodeWarrior_suite_Events,
        Standard_Suite_Events,
        Metrowerks_Shell_Suite_Events,
        Required_Events,
        aetools.TalkTo):
    _signature = 'CWIE'

    _moduleName = 'CodeWarrior'
