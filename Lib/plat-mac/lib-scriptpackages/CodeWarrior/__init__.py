"""
Package generated from /Volumes/Sap/Applications (Mac OS 9)/Metrowerks CodeWarrior 7.0/Metrowerks CodeWarrior/CodeWarrior IDE 4.2.5
Resource aete resid 0 AppleEvent Suites
"""
import aetools
Error = aetools.Error
import Required
import Standard_Suite
import CodeWarrior_suite
import Metrowerks_Shell_Suite


_code_to_module = {
	'reqd' : Required,
	'CoRe' : Standard_Suite,
	'CWIE' : CodeWarrior_suite,
	'MMPR' : Metrowerks_Shell_Suite,
}



_code_to_fullname = {
	'reqd' : ('CodeWarrior.Required', 'Required'),
	'CoRe' : ('CodeWarrior.Standard_Suite', 'Standard_Suite'),
	'CWIE' : ('CodeWarrior.CodeWarrior_suite', 'CodeWarrior_suite'),
	'MMPR' : ('CodeWarrior.Metrowerks_Shell_Suite', 'Metrowerks_Shell_Suite'),
}

from Required import *
from Standard_Suite import *
from CodeWarrior_suite import *
from Metrowerks_Shell_Suite import *
def getbaseclasses(v):
	if hasattr(v, '_superclassnames') and not hasattr(v, '_propdict'):
		v._propdict = {}
		v._elemdict = {}
		for superclass in v._superclassnames:
			v._propdict.update(getattr(eval(superclass), '_privpropdict', {}))
			v._elemdict.update(getattr(eval(superclass), '_privelemdict', {}))
		v._propdict.update(v._privpropdict)
		v._elemdict.update(v._privelemdict)

import StdSuites

#
# Set property and element dictionaries now that all classes have been defined
#
getbaseclasses(character)
getbaseclasses(text)
getbaseclasses(window)
getbaseclasses(file)
getbaseclasses(line)
getbaseclasses(selection_2d_object)
getbaseclasses(application)
getbaseclasses(insertion_point)
getbaseclasses(document)
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
getbaseclasses(Browser_Coloring)
getbaseclasses(Error_Information)
getbaseclasses(VCS_Setup)
getbaseclasses(Editor)
getbaseclasses(Shielded_Folders)
getbaseclasses(Shielded_Folder)
getbaseclasses(Custom_Keywords)
getbaseclasses(Path_Information)
getbaseclasses(File_Mappings)
getbaseclasses(Segment)
getbaseclasses(Debugger_Target)
getbaseclasses(Function_Information)
getbaseclasses(Access_Paths)
getbaseclasses(Extras)
getbaseclasses(Debugger_Windowing)
getbaseclasses(Global_Source_Trees)
getbaseclasses(Syntax_Coloring)
getbaseclasses(base_class)
getbaseclasses(Relative_Path)
getbaseclasses(Target_Settings)
getbaseclasses(Environment_Variable)
getbaseclasses(Source_Tree)
getbaseclasses(Debugger_Global)
getbaseclasses(member_function)
getbaseclasses(Runtime_Settings)
getbaseclasses(Plugin_Settings)
getbaseclasses(data_member)
getbaseclasses(Build_Extras)
getbaseclasses(Font)
getbaseclasses(Target_Source_Trees)
getbaseclasses(Debugger_Display)
getbaseclasses(class_)

#
# Indices of types declared in this module
#
_classdeclarations = {
	'cha ' : character,
	'ctxt' : text,
	'cwin' : window,
	'file' : file,
	'clin' : line,
	'csel' : selection_2d_object,
	'capp' : application,
	'cins' : insertion_point,
	'docu' : document,
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
	'BRKW' : Browser_Coloring,
	'ErrM' : Error_Information,
	'VCSs' : VCS_Setup,
	'EDTR' : Editor,
	'SHFL' : Shielded_Folders,
	'SFit' : Shielded_Folder,
	'CUKW' : Custom_Keywords,
	'PInf' : Path_Information,
	'FLMP' : File_Mappings,
	'Seg ' : Segment,
	'DbTG' : Debugger_Target,
	'FDef' : Function_Information,
	'PATH' : Access_Paths,
	'GXTR' : Extras,
	'DbWN' : Debugger_Windowing,
	'GSTs' : Global_Source_Trees,
	'SNTX' : Syntax_Coloring,
	'BsCl' : base_class,
	'RlPt' : Relative_Path,
	'TARG' : Target_Settings,
	'EnvV' : Environment_Variable,
	'SrcT' : Source_Tree,
	'DbGL' : Debugger_Global,
	'MbFn' : member_function,
	'RSTG' : Runtime_Settings,
	'PSTG' : Plugin_Settings,
	'DtMb' : data_member,
	'LXTR' : Build_Extras,
	'mFNT' : Font,
	'TSTs' : Target_Source_Trees,
	'DbDS' : Debugger_Display,
	'Clas' : class_,
}


class CodeWarrior(Required_Events,
		Standard_Suite_Events,
		CodeWarrior_suite_Events,
		Metrowerks_Shell_Suite_Events,
		aetools.TalkTo):
	_signature = 'CWIE'

	_moduleName = 'CodeWarrior'

