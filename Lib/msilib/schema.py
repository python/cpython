from . import Table

_Validation = Table('_Validation')
_Validation.add_field(1,'Table',11552)
_Validation.add_field(2,'Column',11552)
_Validation.add_field(3,'Nullable',3332)
_Validation.add_field(4,'MinValue',4356)
_Validation.add_field(5,'MaxValue',4356)
_Validation.add_field(6,'KeyTable',7679)
_Validation.add_field(7,'KeyColumn',5378)
_Validation.add_field(8,'Category',7456)
_Validation.add_field(9,'Set',7679)
_Validation.add_field(10,'Description',7679)

ActionText = Table('ActionText')
ActionText.add_field(1,'Action',11592)
ActionText.add_field(2,'Description',7936)
ActionText.add_field(3,'Template',7936)

AdminExecuteSequence = Table('AdminExecuteSequence')
AdminExecuteSequence.add_field(1,'Action',11592)
AdminExecuteSequence.add_field(2,'Condition',7679)
AdminExecuteSequence.add_field(3,'Sequence',5378)

Condition = Table('Condition')
Condition.add_field(1,'Feature_',11558)
Condition.add_field(2,'Level',9474)
Condition.add_field(3,'Condition',7679)

AdminUISequence = Table('AdminUISequence')
AdminUISequence.add_field(1,'Action',11592)
AdminUISequence.add_field(2,'Condition',7679)
AdminUISequence.add_field(3,'Sequence',5378)

AdvtExecuteSequence = Table('AdvtExecuteSequence')
AdvtExecuteSequence.add_field(1,'Action',11592)
AdvtExecuteSequence.add_field(2,'Condition',7679)
AdvtExecuteSequence.add_field(3,'Sequence',5378)

AdvtUISequence = Table('AdvtUISequence')
AdvtUISequence.add_field(1,'Action',11592)
AdvtUISequence.add_field(2,'Condition',7679)
AdvtUISequence.add_field(3,'Sequence',5378)

AppId = Table('AppId')
AppId.add_field(1,'AppId',11558)
AppId.add_field(2,'RemoteServerName',7679)
AppId.add_field(3,'LocalService',7679)
AppId.add_field(4,'ServiceParameters',7679)
AppId.add_field(5,'DllSurrogate',7679)
AppId.add_field(6,'ActivateAtStorage',5378)
AppId.add_field(7,'RunAsInteractiveUser',5378)

AppSearch = Table('AppSearch')
AppSearch.add_field(1,'Property',11592)
AppSearch.add_field(2,'Signature_',11592)

Property = Table('Property')
Property.add_field(1,'Property',11592)
Property.add_field(2,'Value',3840)

BBControl = Table('BBControl')
BBControl.add_field(1,'Billboard_',11570)
BBControl.add_field(2,'BBControl',11570)
BBControl.add_field(3,'Type',3378)
BBControl.add_field(4,'X',1282)
BBControl.add_field(5,'Y',1282)
BBControl.add_field(6,'Width',1282)
BBControl.add_field(7,'Height',1282)
BBControl.add_field(8,'Attributes',4356)
BBControl.add_field(9,'Text',7986)

Billboard = Table('Billboard')
Billboard.add_field(1,'Billboard',11570)
Billboard.add_field(2,'Feature_',3366)
Billboard.add_field(3,'Action',7474)
Billboard.add_field(4,'Ordering',5378)

Feature = Table('Feature')
Feature.add_field(1,'Feature',11558)
Feature.add_field(2,'Feature_Parent',7462)
Feature.add_field(3,'Title',8000)
Feature.add_field(4,'Description',8191)
Feature.add_field(5,'Display',5378)
Feature.add_field(6,'Level',1282)
Feature.add_field(7,'Directory_',7496)
Feature.add_field(8,'Attributes',1282)

Binary = Table('Binary')
Binary.add_field(1,'Name',11592)
Binary.add_field(2,'Data',2304)

BindImage = Table('BindImage')
BindImage.add_field(1,'File_',11592)
BindImage.add_field(2,'Path',7679)

File = Table('File')
File.add_field(1,'File',11592)
File.add_field(2,'Component_',3400)
File.add_field(3,'FileName',4095)
File.add_field(4,'FileSize',260)
File.add_field(5,'Version',7496)
File.add_field(6,'Language',7444)
File.add_field(7,'Attributes',5378)
File.add_field(8,'Sequence',1282)

CCPSearch = Table('CCPSearch')
CCPSearch.add_field(1,'Signature_',11592)

CheckBox = Table('CheckBox')
CheckBox.add_field(1,'Property',11592)
CheckBox.add_field(2,'Value',7488)

Class = Table('Class')
Class.add_field(1,'CLSID',11558)
Class.add_field(2,'Context',11552)
Class.add_field(3,'Component_',11592)
Class.add_field(4,'ProgId_Default',7679)
Class.add_field(5,'Description',8191)
Class.add_field(6,'AppId_',7462)
Class.add_field(7,'FileTypeMask',7679)
Class.add_field(8,'Icon_',7496)
Class.add_field(9,'IconIndex',5378)
Class.add_field(10,'DefInprocHandler',7456)
Class.add_field(11,'Argument',7679)
Class.add_field(12,'Feature_',3366)
Class.add_field(13,'Attributes',5378)

Component = Table('Component')
Component.add_field(1,'Component',11592)
Component.add_field(2,'ComponentId',7462)
Component.add_field(3,'Directory_',3400)
Component.add_field(4,'Attributes',1282)
Component.add_field(5,'Condition',7679)
Component.add_field(6,'KeyPath',7496)

Icon = Table('Icon')
Icon.add_field(1,'Name',11592)
Icon.add_field(2,'Data',2304)

ProgId = Table('ProgId')
ProgId.add_field(1,'ProgId',11775)
ProgId.add_field(2,'ProgId_Parent',7679)
ProgId.add_field(3,'Class_',7462)
ProgId.add_field(4,'Description',8191)
ProgId.add_field(5,'Icon_',7496)
ProgId.add_field(6,'IconIndex',5378)

ComboBox = Table('ComboBox')
ComboBox.add_field(1,'Property',11592)
ComboBox.add_field(2,'Order',9474)
ComboBox.add_field(3,'Value',3392)
ComboBox.add_field(4,'Text',8000)

CompLocator = Table('CompLocator')
CompLocator.add_field(1,'Signature_',11592)
CompLocator.add_field(2,'ComponentId',3366)
CompLocator.add_field(3,'Type',5378)

Complus = Table('Complus')
Complus.add_field(1,'Component_',11592)
Complus.add_field(2,'ExpType',13570)

Directory = Table('Directory')
Directory.add_field(1,'Directory',11592)
Directory.add_field(2,'Directory_Parent',7496)
Directory.add_field(3,'DefaultDir',4095)

Control = Table('Control')
Control.add_field(1,'Dialog_',11592)
Control.add_field(2,'Control',11570)
Control.add_field(3,'Type',3348)
Control.add_field(4,'X',1282)
Control.add_field(5,'Y',1282)
Control.add_field(6,'Width',1282)
Control.add_field(7,'Height',1282)
Control.add_field(8,'Attributes',4356)
Control.add_field(9,'Property',7474)
Control.add_field(10,'Text',7936)
Control.add_field(11,'Control_Next',7474)
Control.add_field(12,'Help',7986)

Dialog = Table('Dialog')
Dialog.add_field(1,'Dialog',11592)
Dialog.add_field(2,'HCentering',1282)
Dialog.add_field(3,'VCentering',1282)
Dialog.add_field(4,'Width',1282)
Dialog.add_field(5,'Height',1282)
Dialog.add_field(6,'Attributes',4356)
Dialog.add_field(7,'Title',8064)
Dialog.add_field(8,'Control_First',3378)
Dialog.add_field(9,'Control_Default',7474)
Dialog.add_field(10,'Control_Cancel',7474)

ControlCondition = Table('ControlCondition')
ControlCondition.add_field(1,'Dialog_',11592)
ControlCondition.add_field(2,'Control_',11570)
ControlCondition.add_field(3,'Action',11570)
ControlCondition.add_field(4,'Condition',11775)

ControlEvent = Table('ControlEvent')
ControlEvent.add_field(1,'Dialog_',11592)
ControlEvent.add_field(2,'Control_',11570)
ControlEvent.add_field(3,'Event',11570)
ControlEvent.add_field(4,'Argument',11775)
ControlEvent.add_field(5,'Condition',15871)
ControlEvent.add_field(6,'Ordering',5378)

CreateFolder = Table('CreateFolder')
CreateFolder.add_field(1,'Directory_',11592)
CreateFolder.add_field(2,'Component_',11592)

CustomAction = Table('CustomAction')
CustomAction.add_field(1,'Action',11592)
CustomAction.add_field(2,'Type',1282)
CustomAction.add_field(3,'Source',7496)
CustomAction.add_field(4,'Target',7679)

DrLocator = Table('DrLocator')
DrLocator.add_field(1,'Signature_',11592)
DrLocator.add_field(2,'Parent',15688)
DrLocator.add_field(3,'Path',15871)
DrLocator.add_field(4,'Depth',5378)

DuplicateFile = Table('DuplicateFile')
DuplicateFile.add_field(1,'FileKey',11592)
DuplicateFile.add_field(2,'Component_',3400)
DuplicateFile.add_field(3,'File_',3400)
DuplicateFile.add_field(4,'DestName',8191)
DuplicateFile.add_field(5,'DestFolder',7496)

Environment = Table('Environment')
Environment.add_field(1,'Environment',11592)
Environment.add_field(2,'Name',4095)
Environment.add_field(3,'Value',8191)
Environment.add_field(4,'Component_',3400)

Error = Table('Error')
Error.add_field(1,'Error',9474)
Error.add_field(2,'Message',7936)

EventMapping = Table('EventMapping')
EventMapping.add_field(1,'Dialog_',11592)
EventMapping.add_field(2,'Control_',11570)
EventMapping.add_field(3,'Event',11570)
EventMapping.add_field(4,'Attribute',3378)

Extension = Table('Extension')
Extension.add_field(1,'Extension',11775)
Extension.add_field(2,'Component_',11592)
Extension.add_field(3,'ProgId_',7679)
Extension.add_field(4,'MIME_',7488)
Extension.add_field(5,'Feature_',3366)

MIME = Table('MIME')
MIME.add_field(1,'ContentType',11584)
MIME.add_field(2,'Extension_',3583)
MIME.add_field(3,'CLSID',7462)

FeatureComponents = Table('FeatureComponents')
FeatureComponents.add_field(1,'Feature_',11558)
FeatureComponents.add_field(2,'Component_',11592)

FileSFPCatalog = Table('FileSFPCatalog')
FileSFPCatalog.add_field(1,'File_',11592)
FileSFPCatalog.add_field(2,'SFPCatalog_',11775)

SFPCatalog = Table('SFPCatalog')
SFPCatalog.add_field(1,'SFPCatalog',11775)
SFPCatalog.add_field(2,'Catalog',2304)
SFPCatalog.add_field(3,'Dependency',7424)

Font = Table('Font')
Font.add_field(1,'File_',11592)
Font.add_field(2,'FontTitle',7552)

IniFile = Table('IniFile')
IniFile.add_field(1,'IniFile',11592)
IniFile.add_field(2,'FileName',4095)
IniFile.add_field(3,'DirProperty',7496)
IniFile.add_field(4,'Section',3936)
IniFile.add_field(5,'Key',3968)
IniFile.add_field(6,'Value',4095)
IniFile.add_field(7,'Action',1282)
IniFile.add_field(8,'Component_',3400)

IniLocator = Table('IniLocator')
IniLocator.add_field(1,'Signature_',11592)
IniLocator.add_field(2,'FileName',3583)
IniLocator.add_field(3,'Section',3424)
IniLocator.add_field(4,'Key',3456)
IniLocator.add_field(5,'Field',5378)
IniLocator.add_field(6,'Type',5378)

InstallExecuteSequence = Table('InstallExecuteSequence')
InstallExecuteSequence.add_field(1,'Action',11592)
InstallExecuteSequence.add_field(2,'Condition',7679)
InstallExecuteSequence.add_field(3,'Sequence',5378)

InstallUISequence = Table('InstallUISequence')
InstallUISequence.add_field(1,'Action',11592)
InstallUISequence.add_field(2,'Condition',7679)
InstallUISequence.add_field(3,'Sequence',5378)

IsolatedComponent = Table('IsolatedComponent')
IsolatedComponent.add_field(1,'Component_Shared',11592)
IsolatedComponent.add_field(2,'Component_Application',11592)

LaunchCondition = Table('LaunchCondition')
LaunchCondition.add_field(1,'Condition',11775)
LaunchCondition.add_field(2,'Description',4095)

ListBox = Table('ListBox')
ListBox.add_field(1,'Property',11592)
ListBox.add_field(2,'Order',9474)
ListBox.add_field(3,'Value',3392)
ListBox.add_field(4,'Text',8000)

ListView = Table('ListView')
ListView.add_field(1,'Property',11592)
ListView.add_field(2,'Order',9474)
ListView.add_field(3,'Value',3392)
ListView.add_field(4,'Text',8000)
ListView.add_field(5,'Binary_',7496)

LockPermissions = Table('LockPermissions')
LockPermissions.add_field(1,'LockObject',11592)
LockPermissions.add_field(2,'Table',11552)
LockPermissions.add_field(3,'Domain',15871)
LockPermissions.add_field(4,'User',11775)
LockPermissions.add_field(5,'Permission',4356)

Media = Table('Media')
Media.add_field(1,'DiskId',9474)
Media.add_field(2,'LastSequence',1282)
Media.add_field(3,'DiskPrompt',8000)
Media.add_field(4,'Cabinet',7679)
Media.add_field(5,'VolumeLabel',7456)
Media.add_field(6,'Source',7496)

MoveFile = Table('MoveFile')
MoveFile.add_field(1,'FileKey',11592)
MoveFile.add_field(2,'Component_',3400)
MoveFile.add_field(3,'SourceName',8191)
MoveFile.add_field(4,'DestName',8191)
MoveFile.add_field(5,'SourceFolder',7496)
MoveFile.add_field(6,'DestFolder',3400)
MoveFile.add_field(7,'Options',1282)

MsiAssembly = Table('MsiAssembly')
MsiAssembly.add_field(1,'Component_',11592)
MsiAssembly.add_field(2,'Feature_',3366)
MsiAssembly.add_field(3,'File_Manifest',7496)
MsiAssembly.add_field(4,'File_Application',7496)
MsiAssembly.add_field(5,'Attributes',5378)

MsiAssemblyName = Table('MsiAssemblyName')
MsiAssemblyName.add_field(1,'Component_',11592)
MsiAssemblyName.add_field(2,'Name',11775)
MsiAssemblyName.add_field(3,'Value',3583)

MsiDigitalCertificate = Table('MsiDigitalCertificate')
MsiDigitalCertificate.add_field(1,'DigitalCertificate',11592)
MsiDigitalCertificate.add_field(2,'CertData',2304)

MsiDigitalSignature = Table('MsiDigitalSignature')
MsiDigitalSignature.add_field(1,'Table',11552)
MsiDigitalSignature.add_field(2,'SignObject',11592)
MsiDigitalSignature.add_field(3,'DigitalCertificate_',3400)
MsiDigitalSignature.add_field(4,'Hash',6400)

MsiFileHash = Table('MsiFileHash')
MsiFileHash.add_field(1,'File_',11592)
MsiFileHash.add_field(2,'Options',1282)
MsiFileHash.add_field(3,'HashPart1',260)
MsiFileHash.add_field(4,'HashPart2',260)
MsiFileHash.add_field(5,'HashPart3',260)
MsiFileHash.add_field(6,'HashPart4',260)

MsiPatchHeaders = Table('MsiPatchHeaders')
MsiPatchHeaders.add_field(1,'StreamRef',11558)
MsiPatchHeaders.add_field(2,'Header',2304)

ODBCAttribute = Table('ODBCAttribute')
ODBCAttribute.add_field(1,'Driver_',11592)
ODBCAttribute.add_field(2,'Attribute',11560)
ODBCAttribute.add_field(3,'Value',8191)

ODBCDriver = Table('ODBCDriver')
ODBCDriver.add_field(1,'Driver',11592)
ODBCDriver.add_field(2,'Component_',3400)
ODBCDriver.add_field(3,'Description',3583)
ODBCDriver.add_field(4,'File_',3400)
ODBCDriver.add_field(5,'File_Setup',7496)

ODBCDataSource = Table('ODBCDataSource')
ODBCDataSource.add_field(1,'DataSource',11592)
ODBCDataSource.add_field(2,'Component_',3400)
ODBCDataSource.add_field(3,'Description',3583)
ODBCDataSource.add_field(4,'DriverDescription',3583)
ODBCDataSource.add_field(5,'Registration',1282)

ODBCSourceAttribute = Table('ODBCSourceAttribute')
ODBCSourceAttribute.add_field(1,'DataSource_',11592)
ODBCSourceAttribute.add_field(2,'Attribute',11552)
ODBCSourceAttribute.add_field(3,'Value',8191)

ODBCTranslator = Table('ODBCTranslator')
ODBCTranslator.add_field(1,'Translator',11592)
ODBCTranslator.add_field(2,'Component_',3400)
ODBCTranslator.add_field(3,'Description',3583)
ODBCTranslator.add_field(4,'File_',3400)
ODBCTranslator.add_field(5,'File_Setup',7496)

Patch = Table('Patch')
Patch.add_field(1,'File_',11592)
Patch.add_field(2,'Sequence',9474)
Patch.add_field(3,'PatchSize',260)
Patch.add_field(4,'Attributes',1282)
Patch.add_field(5,'Header',6400)
Patch.add_field(6,'StreamRef_',7462)

PatchPackage = Table('PatchPackage')
PatchPackage.add_field(1,'PatchId',11558)
PatchPackage.add_field(2,'Media_',1282)

PublishComponent = Table('PublishComponent')
PublishComponent.add_field(1,'ComponentId',11558)
PublishComponent.add_field(2,'Qualifier',11775)
PublishComponent.add_field(3,'Component_',11592)
PublishComponent.add_field(4,'AppData',8191)
PublishComponent.add_field(5,'Feature_',3366)

RadioButton = Table('RadioButton')
RadioButton.add_field(1,'Property',11592)
RadioButton.add_field(2,'Order',9474)
RadioButton.add_field(3,'Value',3392)
RadioButton.add_field(4,'X',1282)
RadioButton.add_field(5,'Y',1282)
RadioButton.add_field(6,'Width',1282)
RadioButton.add_field(7,'Height',1282)
RadioButton.add_field(8,'Text',8000)
RadioButton.add_field(9,'Help',7986)

Registry = Table('Registry')
Registry.add_field(1,'Registry',11592)
Registry.add_field(2,'Root',1282)
Registry.add_field(3,'Key',4095)
Registry.add_field(4,'Name',8191)
Registry.add_field(5,'Value',7936)
Registry.add_field(6,'Component_',3400)

RegLocator = Table('RegLocator')
RegLocator.add_field(1,'Signature_',11592)
RegLocator.add_field(2,'Root',1282)
RegLocator.add_field(3,'Key',3583)
RegLocator.add_field(4,'Name',7679)
RegLocator.add_field(5,'Type',5378)

RemoveFile = Table('RemoveFile')
RemoveFile.add_field(1,'FileKey',11592)
RemoveFile.add_field(2,'Component_',3400)
RemoveFile.add_field(3,'FileName',8191)
RemoveFile.add_field(4,'DirProperty',3400)
RemoveFile.add_field(5,'InstallMode',1282)

RemoveIniFile = Table('RemoveIniFile')
RemoveIniFile.add_field(1,'RemoveIniFile',11592)
RemoveIniFile.add_field(2,'FileName',4095)
RemoveIniFile.add_field(3,'DirProperty',7496)
RemoveIniFile.add_field(4,'Section',3936)
RemoveIniFile.add_field(5,'Key',3968)
RemoveIniFile.add_field(6,'Value',8191)
RemoveIniFile.add_field(7,'Action',1282)
RemoveIniFile.add_field(8,'Component_',3400)

RemoveRegistry = Table('RemoveRegistry')
RemoveRegistry.add_field(1,'RemoveRegistry',11592)
RemoveRegistry.add_field(2,'Root',1282)
RemoveRegistry.add_field(3,'Key',4095)
RemoveRegistry.add_field(4,'Name',8191)
RemoveRegistry.add_field(5,'Component_',3400)

ReserveCost = Table('ReserveCost')
ReserveCost.add_field(1,'ReserveKey',11592)
ReserveCost.add_field(2,'Component_',3400)
ReserveCost.add_field(3,'ReserveFolder',7496)
ReserveCost.add_field(4,'ReserveLocal',260)
ReserveCost.add_field(5,'ReserveSource',260)

SelfReg = Table('SelfReg')
SelfReg.add_field(1,'File_',11592)
SelfReg.add_field(2,'Cost',5378)

ServiceControl = Table('ServiceControl')
ServiceControl.add_field(1,'ServiceControl',11592)
ServiceControl.add_field(2,'Name',4095)
ServiceControl.add_field(3,'Event',1282)
ServiceControl.add_field(4,'Arguments',8191)
ServiceControl.add_field(5,'Wait',5378)
ServiceControl.add_field(6,'Component_',3400)

ServiceInstall = Table('ServiceInstall')
ServiceInstall.add_field(1,'ServiceInstall',11592)
ServiceInstall.add_field(2,'Name',3583)
ServiceInstall.add_field(3,'DisplayName',8191)
ServiceInstall.add_field(4,'ServiceType',260)
ServiceInstall.add_field(5,'StartType',260)
ServiceInstall.add_field(6,'ErrorControl',260)
ServiceInstall.add_field(7,'LoadOrderGroup',7679)
ServiceInstall.add_field(8,'Dependencies',7679)
ServiceInstall.add_field(9,'StartName',7679)
ServiceInstall.add_field(10,'Password',7679)
ServiceInstall.add_field(11,'Arguments',7679)
ServiceInstall.add_field(12,'Component_',3400)
ServiceInstall.add_field(13,'Description',8191)

Shortcut = Table('Shortcut')
Shortcut.add_field(1,'Shortcut',11592)
Shortcut.add_field(2,'Directory_',3400)
Shortcut.add_field(3,'Name',3968)
Shortcut.add_field(4,'Component_',3400)
Shortcut.add_field(5,'Target',3400)
Shortcut.add_field(6,'Arguments',7679)
Shortcut.add_field(7,'Description',8191)
Shortcut.add_field(8,'Hotkey',5378)
Shortcut.add_field(9,'Icon_',7496)
Shortcut.add_field(10,'IconIndex',5378)
Shortcut.add_field(11,'ShowCmd',5378)
Shortcut.add_field(12,'WkDir',7496)

Signature = Table('Signature')
Signature.add_field(1,'Signature',11592)
Signature.add_field(2,'FileName',3583)
Signature.add_field(3,'MinVersion',7444)
Signature.add_field(4,'MaxVersion',7444)
Signature.add_field(5,'MinSize',4356)
Signature.add_field(6,'MaxSize',4356)
Signature.add_field(7,'MinDate',4356)
Signature.add_field(8,'MaxDate',4356)
Signature.add_field(9,'Languages',7679)

TextStyle = Table('TextStyle')
TextStyle.add_field(1,'TextStyle',11592)
TextStyle.add_field(2,'FaceName',3360)
TextStyle.add_field(3,'Size',1282)
TextStyle.add_field(4,'Color',4356)
TextStyle.add_field(5,'StyleBits',5378)

TypeLib = Table('TypeLib')
TypeLib.add_field(1,'LibID',11558)
TypeLib.add_field(2,'Language',9474)
TypeLib.add_field(3,'Component_',11592)
TypeLib.add_field(4,'Version',4356)
TypeLib.add_field(5,'Description',8064)
TypeLib.add_field(6,'Directory_',7496)
TypeLib.add_field(7,'Feature_',3366)
TypeLib.add_field(8,'Cost',4356)

UIText = Table('UIText')
UIText.add_field(1,'Key',11592)
UIText.add_field(2,'Text',8191)

Upgrade = Table('Upgrade')
Upgrade.add_field(1,'UpgradeCode',11558)
Upgrade.add_field(2,'VersionMin',15636)
Upgrade.add_field(3,'VersionMax',15636)
Upgrade.add_field(4,'Language',15871)
Upgrade.add_field(5,'Attributes',8452)
Upgrade.add_field(6,'Remove',7679)
Upgrade.add_field(7,'ActionProperty',3400)

Verb = Table('Verb')
Verb.add_field(1,'Extension_',11775)
Verb.add_field(2,'Verb',11552)
Verb.add_field(3,'Sequence',5378)
Verb.add_field(4,'Command',8191)
Verb.add_field(5,'Argument',8191)

tables=[_Validation, ActionText, AdminExecuteSequence, Condition, AdminUISequence, AdvtExecuteSequence, AdvtUISequence, AppId, AppSearch, Property, BBControl, Billboard, Feature, Binary, BindImage, File, CCPSearch, CheckBox, Class, Component, Icon, ProgId, ComboBox, CompLocator, Complus, Directory, Control, Dialog, ControlCondition, ControlEvent, CreateFolder, CustomAction, DrLocator, DuplicateFile, Environment, Error, EventMapping, Extension, MIME, FeatureComponents, FileSFPCatalog, SFPCatalog, Font, IniFile, IniLocator, InstallExecuteSequence, InstallUISequence, IsolatedComponent, LaunchCondition, ListBox, ListView, LockPermissions, Media, MoveFile, MsiAssembly, MsiAssemblyName, MsiDigitalCertificate, MsiDigitalSignature, MsiFileHash, MsiPatchHeaders, ODBCAttribute, ODBCDriver, ODBCDataSource, ODBCSourceAttribute, ODBCTranslator, Patch, PatchPackage, PublishComponent, RadioButton, Registry, RegLocator, RemoveFile, RemoveIniFile, RemoveRegistry, ReserveCost, SelfReg, ServiceControl, ServiceInstall, Shortcut, Signature, TextStyle, TypeLib, UIText, Upgrade, Verb]

_Validation_records = [
('_Validation','Table','N',None, None, None, None, 'Identifier',None, 'Name of table',),
('_Validation','Column','N',None, None, None, None, 'Identifier',None, 'Name of column',),
('_Validation','Description','Y',None, None, None, None, 'Text',None, 'Description of column',),
('_Validation','Set','Y',None, None, None, None, 'Text',None, 'Set of values that are permitted',),
('_Validation','Category','Y',None, None, None, None, None, 'Text;Formatted;Template;Condition;Guid;Path;Version;Language;Identifier;Binary;UpperCase;LowerCase;Filename;Paths;AnyPath;WildCardFilename;RegPath;KeyFormatted;CustomSource;Property;Cabinet;Shortcut;URL','String category',),
('_Validation','KeyColumn','Y',1,32,None, None, None, None, 'Column to which foreign key connects',),
('_Validation','KeyTable','Y',None, None, None, None, 'Identifier',None, 'For foreign key, Name of table to which data must link',),
('_Validation','MaxValue','Y',-2147483647,2147483647,None, None, None, None, 'Maximum value allowed',),
('_Validation','MinValue','Y',-2147483647,2147483647,None, None, None, None, 'Minimum value allowed',),
('_Validation','Nullable','N',None, None, None, None, None, 'Y;N;@','Whether the column is nullable',),
('ActionText','Description','Y',None, None, None, None, 'Text',None, 'Localized description displayed in progress dialog and log when action is executing.',),
('ActionText','Action','N',None, None, None, None, 'Identifier',None, 'Name of action to be described.',),
('ActionText','Template','Y',None, None, None, None, 'Template',None, 'Optional localized format template used to format action data records for display during action execution.',),
('AdminExecuteSequence','Action','N',None, None, None, None, 'Identifier',None, 'Name of action to invoke, either in the engine or the handler DLL.',),
('AdminExecuteSequence','Condition','Y',None, None, None, None, 'Condition',None, 'Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.',),
('AdminExecuteSequence','Sequence','Y',-4,32767,None, None, None, None, 'Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.',),
('Condition','Condition','Y',None, None, None, None, 'Condition',None, 'Expression evaluated to determine if Level in the Feature table is to change.',),
('Condition','Feature_','N',None, None, 'Feature',1,'Identifier',None, 'Reference to a Feature entry in Feature table.',),
('Condition','Level','N',0,32767,None, None, None, None, 'New selection Level to set in Feature table if Condition evaluates to TRUE.',),
('AdminUISequence','Action','N',None, None, None, None, 'Identifier',None, 'Name of action to invoke, either in the engine or the handler DLL.',),
('AdminUISequence','Condition','Y',None, None, None, None, 'Condition',None, 'Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.',),
('AdminUISequence','Sequence','Y',-4,32767,None, None, None, None, 'Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.',),
('AdvtExecuteSequence','Action','N',None, None, None, None, 'Identifier',None, 'Name of action to invoke, either in the engine or the handler DLL.',),
('AdvtExecuteSequence','Condition','Y',None, None, None, None, 'Condition',None, 'Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.',),
('AdvtExecuteSequence','Sequence','Y',-4,32767,None, None, None, None, 'Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.',),
('AdvtUISequence','Action','N',None, None, None, None, 'Identifier',None, 'Name of action to invoke, either in the engine or the handler DLL.',),
('AdvtUISequence','Condition','Y',None, None, None, None, 'Condition',None, 'Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.',),
('AdvtUISequence','Sequence','Y',-4,32767,None, None, None, None, 'Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.',),
('AppId','AppId','N',None, None, None, None, 'Guid',None, None, ),
('AppId','ActivateAtStorage','Y',0,1,None, None, None, None, None, ),
('AppId','DllSurrogate','Y',None, None, None, None, 'Text',None, None, ),
('AppId','LocalService','Y',None, None, None, None, 'Text',None, None, ),
('AppId','RemoteServerName','Y',None, None, None, None, 'Formatted',None, None, ),
('AppId','RunAsInteractiveUser','Y',0,1,None, None, None, None, None, ),
('AppId','ServiceParameters','Y',None, None, None, None, 'Text',None, None, ),
('AppSearch','Property','N',None, None, None, None, 'Identifier',None, 'The property associated with a Signature',),
('AppSearch','Signature_','N',None, None, 'Signature;RegLocator;IniLocator;DrLocator;CompLocator',1,'Identifier',None, 'The Signature_ represents a unique file signature and is also the foreign key in the Signature,  RegLocator, IniLocator, CompLocator and the DrLocator tables.',),
('Property','Property','N',None, None, None, None, 'Identifier',None, 'Name of property, uppercase if settable by launcher or loader.',),
('Property','Value','N',None, None, None, None, 'Text',None, 'String value for property.  Never null or empty.',),
('BBControl','Type','N',None, None, None, None, 'Identifier',None, 'The type of the control.',),
('BBControl','Y','N',0,32767,None, None, None, None, 'Vertical coordinate of the upper left corner of the bounding rectangle of the control.',),
('BBControl','Text','Y',None, None, None, None, 'Text',None, 'A string used to set the initial text contained within a control (if appropriate).',),
('BBControl','BBControl','N',None, None, None, None, 'Identifier',None, 'Name of the control. This name must be unique within a billboard, but can repeat on different billboard.',),
('BBControl','Attributes','Y',0,2147483647,None, None, None, None, 'A 32-bit word that specifies the attribute flags to be applied to this control.',),
('BBControl','Billboard_','N',None, None, 'Billboard',1,'Identifier',None, 'External key to the Billboard table, name of the billboard.',),
('BBControl','Height','N',0,32767,None, None, None, None, 'Height of the bounding rectangle of the control.',),
('BBControl','Width','N',0,32767,None, None, None, None, 'Width of the bounding rectangle of the control.',),
('BBControl','X','N',0,32767,None, None, None, None, 'Horizontal coordinate of the upper left corner of the bounding rectangle of the control.',),
('Billboard','Action','Y',None, None, None, None, 'Identifier',None, 'The name of an action. The billboard is displayed during the progress messages received from this action.',),
('Billboard','Billboard','N',None, None, None, None, 'Identifier',None, 'Name of the billboard.',),
('Billboard','Feature_','N',None, None, 'Feature',1,'Identifier',None, 'An external key to the Feature Table. The billboard is shown only if this feature is being installed.',),
('Billboard','Ordering','Y',0,32767,None, None, None, None, 'A positive integer. If there is more than one billboard corresponding to an action they will be shown in the order defined by this column.',),
('Feature','Description','Y',None, None, None, None, 'Text',None, 'Longer descriptive text describing a visible feature item.',),
('Feature','Attributes','N',None, None, None, None, None, '0;1;2;4;5;6;8;9;10;16;17;18;20;21;22;24;25;26;32;33;34;36;37;38;48;49;50;52;53;54','Feature attributes',),
('Feature','Feature','N',None, None, None, None, 'Identifier',None, 'Primary key used to identify a particular feature record.',),
('Feature','Directory_','Y',None, None, 'Directory',1,'UpperCase',None, 'The name of the Directory that can be configured by the UI. A non-null value will enable the browse button.',),
('Feature','Level','N',0,32767,None, None, None, None, 'The install level at which record will be initially selected. An install level of 0 will disable an item and prevent its display.',),
('Feature','Title','Y',None, None, None, None, 'Text',None, 'Short text identifying a visible feature item.',),
('Feature','Display','Y',0,32767,None, None, None, None, 'Numeric sort order, used to force a specific display ordering.',),
('Feature','Feature_Parent','Y',None, None, 'Feature',1,'Identifier',None, 'Optional key of a parent record in the same table. If the parent is not selected, then the record will not be installed. Null indicates a root item.',),
('Binary','Name','N',None, None, None, None, 'Identifier',None, 'Unique key identifying the binary data.',),
('Binary','Data','N',None, None, None, None, 'Binary',None, 'The unformatted binary data.',),
('BindImage','File_','N',None, None, 'File',1,'Identifier',None, 'The index into the File table. This must be an executable file.',),
('BindImage','Path','Y',None, None, None, None, 'Paths',None, 'A list of ;  delimited paths that represent the paths to be searched for the import DLLS. The list is usually a list of properties each enclosed within square brackets [] .',),
('File','Sequence','N',1,32767,None, None, None, None, 'Sequence with respect to the media images; order must track cabinet order.',),
('File','Attributes','Y',0,32767,None, None, None, None, 'Integer containing bit flags representing file attributes (with the decimal value of each bit position in parentheses)',),
('File','File','N',None, None, None, None, 'Identifier',None, 'Primary key, non-localized token, must match identifier in cabinet.  For uncompressed files, this field is ignored.',),
('File','Component_','N',None, None, 'Component',1,'Identifier',None, 'Foreign key referencing Component that controls the file.',),
('File','FileName','N',None, None, None, None, 'Filename',None, 'File name used for installation, may be localized.  This may contain a "short name|long name" pair.',),
('File','FileSize','N',0,2147483647,None, None, None, None, 'Size of file in bytes (integer).',),
('File','Language','Y',None, None, None, None, 'Language',None, 'List of decimal language Ids, comma-separated if more than one.',),
('File','Version','Y',None, None, 'File',1,'Version',None, 'Version string for versioned files;  Blank for unversioned files.',),
('CCPSearch','Signature_','N',None, None, 'Signature;RegLocator;IniLocator;DrLocator;CompLocator',1,'Identifier',None, 'The Signature_ represents a unique file signature and is also the foreign key in the Signature,  RegLocator, IniLocator, CompLocator and the DrLocator tables.',),
('CheckBox','Property','N',None, None, None, None, 'Identifier',None, 'A named property to be tied to the item.',),
('CheckBox','Value','Y',None, None, None, None, 'Formatted',None, 'The value string associated with the item.',),
('Class','Description','Y',None, None, None, None, 'Text',None, 'Localized description for the Class.',),
('Class','Attributes','Y',None, 32767,None, None, None, None, 'Class registration attributes.',),
('Class','Feature_','N',None, None, 'Feature',1,'Identifier',None, 'Required foreign key into the Feature Table, specifying the feature to validate or install in order for the CLSID factory to be operational.',),
('Class','AppId_','Y',None, None, 'AppId',1,'Guid',None, 'Optional AppID containing DCOM information for associated application (string GUID).',),
('Class','Argument','Y',None, None, None, None, 'Formatted',None, 'optional argument for LocalServers.',),
('Class','CLSID','N',None, None, None, None, 'Guid',None, 'The CLSID of an OLE factory.',),
('Class','Component_','N',None, None, 'Component',1,'Identifier',None, 'Required foreign key into the Component Table, specifying the component for which to return a path when called through LocateComponent.',),
('Class','Context','N',None, None, None, None, 'Identifier',None, 'The numeric server context for this server. CLSCTX_xxxx',),
('Class','DefInprocHandler','Y',None, None, None, None, 'Filename','1;2;3','Optional default inproc handler.  Only optionally provided if Context=CLSCTX_LOCAL_SERVER.  Typically "ole32.dll" or "mapi32.dll"',),
('Class','FileTypeMask','Y',None, None, None, None, 'Text',None, 'Optional string containing information for the HKCRthis CLSID key. If multiple patterns exist, they must be delimited by a semicolon, and numeric subkeys will be generated: 0,1,2...',),
('Class','Icon_','Y',None, None, 'Icon',1,'Identifier',None, 'Optional foreign key into the Icon Table, specifying the icon file associated with this CLSID. Will be written under the DefaultIcon key.',),
('Class','IconIndex','Y',-32767,32767,None, None, None, None, 'Optional icon index.',),
('Class','ProgId_Default','Y',None, None, 'ProgId',1,'Text',None, 'Optional ProgId associated with this CLSID.',),
('Component','Condition','Y',None, None, None, None, 'Condition',None, "A conditional statement that will disable this component if the specified condition evaluates to the 'True' state. If a component is disabled, it will not be installed, regardless of the 'Action' state associated with the component.",),
('Component','Attributes','N',None, None, None, None, None, None, 'Remote execution option, one of irsEnum',),
('Component','Component','N',None, None, None, None, 'Identifier',None, 'Primary key used to identify a particular component record.',),
('Component','ComponentId','Y',None, None, None, None, 'Guid',None, 'A string GUID unique to this component, version, and language.',),
('Component','Directory_','N',None, None, 'Directory',1,'Identifier',None, 'Required key of a Directory table record. This is actually a property name whose value contains the actual path, set either by the AppSearch action or with the default setting obtained from the Directory table.',),
('Component','KeyPath','Y',None, None, 'File;Registry;ODBCDataSource',1,'Identifier',None, 'Either the primary key into the File table, Registry table, or ODBCDataSource table. This extract path is stored when the component is installed, and is used to detect the presence of the component and to return the path to it.',),
('Icon','Name','N',None, None, None, None, 'Identifier',None, 'Primary key. Name of the icon file.',),
('Icon','Data','N',None, None, None, None, 'Binary',None, 'Binary stream. The binary icon data in PE (.DLL or .EXE) or icon (.ICO) format.',),
('ProgId','Description','Y',None, None, None, None, 'Text',None, 'Localized description for the Program identifier.',),
('ProgId','Icon_','Y',None, None, 'Icon',1,'Identifier',None, 'Optional foreign key into the Icon Table, specifying the icon file associated with this ProgId. Will be written under the DefaultIcon key.',),
('ProgId','IconIndex','Y',-32767,32767,None, None, None, None, 'Optional icon index.',),
('ProgId','ProgId','N',None, None, None, None, 'Text',None, 'The Program Identifier. Primary key.',),
('ProgId','Class_','Y',None, None, 'Class',1,'Guid',None, 'The CLSID of an OLE factory corresponding to the ProgId.',),
('ProgId','ProgId_Parent','Y',None, None, 'ProgId',1,'Text',None, 'The Parent Program Identifier. If specified, the ProgId column becomes a version independent prog id.',),
('ComboBox','Text','Y',None, None, None, None, 'Formatted',None, 'The visible text to be assigned to the item. Optional. If this entry or the entire column is missing, the text is the same as the value.',),
('ComboBox','Property','N',None, None, None, None, 'Identifier',None, 'A named property to be tied to this item. All the items tied to the same property become part of the same combobox.',),
('ComboBox','Value','N',None, None, None, None, 'Formatted',None, 'The value string associated with this item. Selecting the line will set the associated property to this value.',),
('ComboBox','Order','N',1,32767,None, None, None, None, 'A positive integer used to determine the ordering of the items within one list.\tThe integers do not have to be consecutive.',),
('CompLocator','Type','Y',0,1,None, None, None, None, 'A boolean value that determines if the registry value is a filename or a directory location.',),
('CompLocator','Signature_','N',None, None, None, None, 'Identifier',None, 'The table key. The Signature_ represents a unique file signature and is also the foreign key in the Signature table.',),
('CompLocator','ComponentId','N',None, None, None, None, 'Guid',None, 'A string GUID unique to this component, version, and language.',),
('Complus','Component_','N',None, None, 'Component',1,'Identifier',None, 'Foreign key referencing Component that controls the ComPlus component.',),
('Complus','ExpType','Y',0,32767,None, None, None, None, 'ComPlus component attributes.',),
('Directory','Directory','N',None, None, None, None, 'Identifier',None, 'Unique identifier for directory entry, primary key. If a property by this name is defined, it contains the full path to the directory.',),
('Directory','DefaultDir','N',None, None, None, None, 'DefaultDir',None, "The default sub-path under parent's path.",),
('Directory','Directory_Parent','Y',None, None, 'Directory',1,'Identifier',None, 'Reference to the entry in this table specifying the default parent directory. A record parented to itself or with a Null parent represents a root of the install tree.',),
('Control','Type','N',None, None, None, None, 'Identifier',None, 'The type of the control.',),
('Control','Y','N',0,32767,None, None, None, None, 'Vertical coordinate of the upper left corner of the bounding rectangle of the control.',),
('Control','Text','Y',None, None, None, None, 'Formatted',None, 'A string used to set the initial text contained within a control (if appropriate).',),
('Control','Property','Y',None, None, None, None, 'Identifier',None, 'The name of a defined property to be linked to this control. ',),
('Control','Attributes','Y',0,2147483647,None, None, None, None, 'A 32-bit word that specifies the attribute flags to be applied to this control.',),
('Control','Height','N',0,32767,None, None, None, None, 'Height of the bounding rectangle of the control.',),
('Control','Width','N',0,32767,None, None, None, None, 'Width of the bounding rectangle of the control.',),
('Control','X','N',0,32767,None, None, None, None, 'Horizontal coordinate of the upper left corner of the bounding rectangle of the control.',),
('Control','Control','N',None, None, None, None, 'Identifier',None, 'Name of the control. This name must be unique within a dialog, but can repeat on different dialogs. ',),
('Control','Control_Next','Y',None, None, 'Control',2,'Identifier',None, 'The name of an other control on the same dialog. This link defines the tab order of the controls. The links have to form one or more cycles!',),
('Control','Dialog_','N',None, None, 'Dialog',1,'Identifier',None, 'External key to the Dialog table, name of the dialog.',),
('Control','Help','Y',None, None, None, None, 'Text',None, 'The help strings used with the button. The text is optional. ',),
('Dialog','Attributes','Y',0,2147483647,None, None, None, None, 'A 32-bit word that specifies the attribute flags to be applied to this dialog.',),
('Dialog','Height','N',0,32767,None, None, None, None, 'Height of the bounding rectangle of the dialog.',),
('Dialog','Width','N',0,32767,None, None, None, None, 'Width of the bounding rectangle of the dialog.',),
('Dialog','Dialog','N',None, None, None, None, 'Identifier',None, 'Name of the dialog.',),
('Dialog','Control_Cancel','Y',None, None, 'Control',2,'Identifier',None, 'Defines the cancel control. Hitting escape or clicking on the close icon on the dialog is equivalent to pushing this button.',),
('Dialog','Control_Default','Y',None, None, 'Control',2,'Identifier',None, 'Defines the default control. Hitting return is equivalent to pushing this button.',),
('Dialog','Control_First','N',None, None, 'Control',2,'Identifier',None, 'Defines the control that has the focus when the dialog is created.',),
('Dialog','HCentering','N',0,100,None, None, None, None, 'Horizontal position of the dialog on a 0-100 scale. 0 means left end, 100 means right end of the screen, 50 center.',),
('Dialog','Title','Y',None, None, None, None, 'Formatted',None, "A text string specifying the title to be displayed in the title bar of the dialog's window.",),
('Dialog','VCentering','N',0,100,None, None, None, None, 'Vertical position of the dialog on a 0-100 scale. 0 means top end, 100 means bottom end of the screen, 50 center.',),
('ControlCondition','Action','N',None, None, None, None, None, 'Default;Disable;Enable;Hide;Show','The desired action to be taken on the specified control.',),
('ControlCondition','Condition','N',None, None, None, None, 'Condition',None, 'A standard conditional statement that specifies under which conditions the action should be triggered.',),
('ControlCondition','Dialog_','N',None, None, 'Dialog',1,'Identifier',None, 'A foreign key to the Dialog table, name of the dialog.',),
('ControlCondition','Control_','N',None, None, 'Control',2,'Identifier',None, 'A foreign key to the Control table, name of the control.',),
('ControlEvent','Condition','Y',None, None, None, None, 'Condition',None, 'A standard conditional statement that specifies under which conditions an event should be triggered.',),
('ControlEvent','Ordering','Y',0,2147483647,None, None, None, None, 'An integer used to order several events tied to the same control. Can be left blank.',),
('ControlEvent','Argument','N',None, None, None, None, 'Formatted',None, 'A value to be used as a modifier when triggering a particular event.',),
('ControlEvent','Dialog_','N',None, None, 'Dialog',1,'Identifier',None, 'A foreign key to the Dialog table, name of the dialog.',),
('ControlEvent','Control_','N',None, None, 'Control',2,'Identifier',None, 'A foreign key to the Control table, name of the control',),
('ControlEvent','Event','N',None, None, None, None, 'Formatted',None, 'An identifier that specifies the type of the event that should take place when the user interacts with control specified by the first two entries.',),
('CreateFolder','Component_','N',None, None, 'Component',1,'Identifier',None, 'Foreign key into the Component table.',),
('CreateFolder','Directory_','N',None, None, 'Directory',1,'Identifier',None, 'Primary key, could be foreign key into the Directory table.',),
('CustomAction','Type','N',1,16383,None, None, None, None, 'The numeric custom action type, consisting of source location, code type, entry, option flags.',),
('CustomAction','Action','N',None, None, None, None, 'Identifier',None, 'Primary key, name of action, normally appears in sequence table unless private use.',),
('CustomAction','Source','Y',None, None, None, None, 'CustomSource',None, 'The table reference of the source of the code.',),
('CustomAction','Target','Y',None, None, None, None, 'Formatted',None, 'Execution parameter, depends on the type of custom action',),
('DrLocator','Signature_','N',None, None, None, None, 'Identifier',None, 'The Signature_ represents a unique file signature and is also the foreign key in the Signature table.',),
('DrLocator','Path','Y',None, None, None, None, 'AnyPath',None, 'The path on the user system. This is either a subpath below the value of the Parent or a full path. The path may contain properties enclosed within [ ] that will be expanded.',),
('DrLocator','Depth','Y',0,32767,None, None, None, None, 'The depth below the path to which the Signature_ is recursively searched. If absent, the depth is assumed to be 0.',),
('DrLocator','Parent','Y',None, None, None, None, 'Identifier',None, 'The parent file signature. It is also a foreign key in the Signature table. If null and the Path column does not expand to a full path, then all the fixed drives of the user system are searched using the Path.',),
('DuplicateFile','File_','N',None, None, 'File',1,'Identifier',None, 'Foreign key referencing the source file to be duplicated.',),
('DuplicateFile','Component_','N',None, None, 'Component',1,'Identifier',None, 'Foreign key referencing Component that controls the duplicate file.',),
('DuplicateFile','DestFolder','Y',None, None, None, None, 'Identifier',None, 'Name of a property whose value is assumed to resolve to the full pathname to a destination folder.',),
('DuplicateFile','DestName','Y',None, None, None, None, 'Filename',None, 'Filename to be given to the duplicate file.',),
('DuplicateFile','FileKey','N',None, None, None, None, 'Identifier',None, 'Primary key used to identify a particular file entry',),
('Environment','Name','N',None, None, None, None, 'Text',None, 'The name of the environmental value.',),
('Environment','Value','Y',None, None, None, None, 'Formatted',None, 'The value to set in the environmental settings.',),
('Environment','Component_','N',None, None, 'Component',1,'Identifier',None, 'Foreign key into the Component table referencing component that controls the installing of the environmental value.',),
('Environment','Environment','N',None, None, None, None, 'Identifier',None, 'Unique identifier for the environmental variable setting',),
('Error','Error','N',0,32767,None, None, None, None, 'Integer error number, obtained from header file IError(...) macros.',),
('Error','Message','Y',None, None, None, None, 'Template',None, 'Error formatting template, obtained from user ed. or localizers.',),
('EventMapping','Dialog_','N',None, None, 'Dialog',1,'Identifier',None, 'A foreign key to the Dialog table, name of the Dialog.',),
('EventMapping','Control_','N',None, None, 'Control',2,'Identifier',None, 'A foreign key to the Control table, name of the control.',),
('EventMapping','Event','N',None, None, None, None, 'Identifier',None, 'An identifier that specifies the type of the event that the control subscribes to.',),
('EventMapping','Attribute','N',None, None, None, None, 'Identifier',None, 'The name of the control attribute, that is set when this event is received.',),
('Extension','Feature_','N',None, None, 'Feature',1,'Identifier',None, 'Required foreign key into the Feature Table, specifying the feature to validate or install in order for the CLSID factory to be operational.',),
('Extension','Component_','N',None, None, 'Component',1,'Identifier',None, 'Required foreign key into the Component Table, specifying the component for which to return a path when called through LocateComponent.',),
('Extension','Extension','N',None, None, None, None, 'Text',None, 'The extension associated with the table row.',),
('Extension','MIME_','Y',None, None, 'MIME',1,'Text',None, 'Optional Context identifier, typically "type/format" associated with the extension',),
('Extension','ProgId_','Y',None, None, 'ProgId',1,'Text',None, 'Optional ProgId associated with this extension.',),
('MIME','CLSID','Y',None, None, None, None, 'Guid',None, 'Optional associated CLSID.',),
('MIME','ContentType','N',None, None, None, None, 'Text',None, 'Primary key. Context identifier, typically "type/format".',),
('MIME','Extension_','N',None, None, 'Extension',1,'Text',None, 'Optional associated extension (without dot)',),
('FeatureComponents','Feature_','N',None, None, 'Feature',1,'Identifier',None, 'Foreign key into Feature table.',),
('FeatureComponents','Component_','N',None, None, 'Component',1,'Identifier',None, 'Foreign key into Component table.',),
('FileSFPCatalog','File_','N',None, None, 'File',1,'Identifier',None, 'File associated with the catalog',),
('FileSFPCatalog','SFPCatalog_','N',None, None, 'SFPCatalog',1,'Filename',None, 'Catalog associated with the file',),
('SFPCatalog','SFPCatalog','N',None, None, None, None, 'Filename',None, 'File name for the catalog.',),
('SFPCatalog','Catalog','N',None, None, None, None, 'Binary',None, 'SFP Catalog',),
('SFPCatalog','Dependency','Y',None, None, None, None, 'Formatted',None, 'Parent catalog - only used by SFP',),
('Font','File_','N',None, None, 'File',1,'Identifier',None, 'Primary key, foreign key into File table referencing font file.',),
('Font','FontTitle','Y',None, None, None, None, 'Text',None, 'Font name.',),
('IniFile','Action','N',None, None, None, None, None, '0;1;3','The type of modification to be made, one of iifEnum',),
('IniFile','Value','N',None, None, None, None, 'Formatted',None, 'The value to be written.',),
('IniFile','Component_','N',None, None, 'Component',1,'Identifier',None, 'Foreign key into the Component table referencing component that controls the installing of the .INI value.',),
('IniFile','FileName','N',None, None, None, None, 'Filename',None, 'The .INI file name in which to write the information',),
('IniFile','IniFile','N',None, None, None, None, 'Identifier',None, 'Primary key, non-localized token.',),
('IniFile','DirProperty','Y',None, None, None, None, 'Identifier',None, 'Foreign key into the Directory table denoting the directory where the .INI file is.',),
('IniFile','Key','N',None, None, None, None, 'Formatted',None, 'The .INI file key below Section.',),
('IniFile','Section','N',None, None, None, None, 'Formatted',None, 'The .INI file Section.',),
('IniLocator','Type','Y',0,2,None, None, None, None, 'An integer value that determines if the .INI value read is a filename or a directory location or to be used as is w/o interpretation.',),
('IniLocator','Signature_','N',None, None, None, None, 'Identifier',None, 'The table key. The Signature_ represents a unique file signature and is also the foreign key in the Signature table.',),
('IniLocator','FileName','N',None, None, None, None, 'Filename',None, 'The .INI file name.',),
('IniLocator','Key','N',None, None, None, None, 'Text',None, 'Key value (followed by an equals sign in INI file).',),
('IniLocator','Section','N',None, None, None, None, 'Text',None, 'Section name within in file (within square brackets in INI file).',),
('IniLocator','Field','Y',0,32767,None, None, None, None, 'The field in the .INI line. If Field is null or 0 the entire line is read.',),
('InstallExecuteSequence','Action','N',None, None, None, None, 'Identifier',None, 'Name of action to invoke, either in the engine or the handler DLL.',),
('InstallExecuteSequence','Condition','Y',None, None, None, None, 'Condition',None, 'Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.',),
('InstallExecuteSequence','Sequence','Y',-4,32767,None, None, None, None, 'Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.',),
('InstallUISequence','Action','N',None, None, None, None, 'Identifier',None, 'Name of action to invoke, either in the engine or the handler DLL.',),
('InstallUISequence','Condition','Y',None, None, None, None, 'Condition',None, 'Optional expression which skips the action if evaluates to expFalse.If the expression syntax is invalid, the engine will terminate, returning iesBadActionData.',),
('InstallUISequence','Sequence','Y',-4,32767,None, None, None, None, 'Number that determines the sort order in which the actions are to be executed.  Leave blank to suppress action.',),
('IsolatedComponent','Component_Application','N',None, None, 'Component',1,'Identifier',None, 'Key to Component table item for application',),
('IsolatedComponent','Component_Shared','N',None, None, 'Component',1,'Identifier',None, 'Key to Component table item to be isolated',),
('LaunchCondition','Description','N',None, None, None, None, 'Formatted',None, 'Localizable text to display when condition fails and install must abort.',),
('LaunchCondition','Condition','N',None, None, None, None, 'Condition',None, 'Expression which must evaluate to TRUE in order for install to commence.',),
('ListBox','Text','Y',None, None, None, None, 'Text',None, 'The visible text to be assigned to the item. Optional. If this entry or the entire column is missing, the text is the same as the value.',),
('ListBox','Property','N',None, None, None, None, 'Identifier',None, 'A named property to be tied to this item. All the items tied to the same property become part of the same listbox.',),
('ListBox','Value','N',None, None, None, None, 'Formatted',None, 'The value string associated with this item. Selecting the line will set the associated property to this value.',),
('ListBox','Order','N',1,32767,None, None, None, None, 'A positive integer used to determine the ordering of the items within one list..The integers do not have to be consecutive.',),
('ListView','Text','Y',None, None, None, None, 'Text',None, 'The visible text to be assigned to the item. Optional. If this entry or the entire column is missing, the text is the same as the value.',),
('ListView','Property','N',None, None, None, None, 'Identifier',None, 'A named property to be tied to this item. All the items tied to the same property become part of the same listview.',),
('ListView','Value','N',None, None, None, None, 'Identifier',None, 'The value string associated with this item. Selecting the line will set the associated property to this value.',),
('ListView','Order','N',1,32767,None, None, None, None, 'A positive integer used to determine the ordering of the items within one list..The integers do not have to be consecutive.',),
('ListView','Binary_','Y',None, None, 'Binary',1,'Identifier',None, 'The name of the icon to be displayed with the icon. The binary information is looked up from the Binary Table.',),
('LockPermissions','Table','N',None, None, None, None, 'Identifier','Directory;File;Registry','Reference to another table name',),
('LockPermissions','Domain','Y',None, None, None, None, 'Formatted',None, 'Domain name for user whose permissions are being set. (usually a property)',),
('LockPermissions','LockObject','N',None, None, None, None, 'Identifier',None, 'Foreign key into Registry or File table',),
('LockPermissions','Permission','Y',-2147483647,2147483647,None, None, None, None, 'Permission Access mask.  Full Control = 268435456 (GENERIC_ALL = 0x10000000)',),
('LockPermissions','User','N',None, None, None, None, 'Formatted',None, 'User for permissions to be set.  (usually a property)',),
('Media','Source','Y',None, None, None, None, 'Property',None, 'The property defining the location of the cabinet file.',),
('Media','Cabinet','Y',None, None, None, None, 'Cabinet',None, 'If some or all of the files stored on the media are compressed in a cabinet, the name of that cabinet.',),
('Media','DiskId','N',1,32767,None, None, None, None, 'Primary key, integer to determine sort order for table.',),
('Media','DiskPrompt','Y',None, None, None, None, 'Text',None, 'Disk name: the visible text actually printed on the disk.  This will be used to prompt the user when this disk needs to be inserted.',),
('Media','LastSequence','N',0,32767,None, None, None, None, 'File sequence number for the last file for this media.',),
('Media','VolumeLabel','Y',None, None, None, None, 'Text',None, 'The label attributed to the volume.',),
('ModuleComponents','Component','N',None, None, 'Component',1,'Identifier',None, 'Component contained in the module.',),
('ModuleComponents','Language','N',None, None, 'ModuleSignature',2,None, None, 'Default language ID for module (may be changed by transform).',),
('ModuleComponents','ModuleID','N',None, None, 'ModuleSignature',1,'Identifier',None, 'Module containing the component.',),
('ModuleSignature','Language','N',None, None, None, None, None, None, 'Default decimal language of module.',),
('ModuleSignature','Version','N',None, None, None, None, 'Version',None, 'Version of the module.',),
('ModuleSignature','ModuleID','N',None, None, None, None, 'Identifier',None, 'Module identifier (String.GUID).',),
('ModuleDependency','ModuleID','N',None, None, 'ModuleSignature',1,'Identifier',None, 'Module requiring the dependency.',),
('ModuleDependency','ModuleLanguage','N',None, None, 'ModuleSignature',2,None, None, 'Language of module requiring the dependency.',),
('ModuleDependency','RequiredID','N',None, None, None, None, None, None, 'String.GUID of required module.',),
('ModuleDependency','RequiredLanguage','N',None, None, None, None, None, None, 'LanguageID of the required module.',),
('ModuleDependency','RequiredVersion','Y',None, None, None, None, 'Version',None, 'Version of the required version.',),
('ModuleExclusion','ModuleID','N',None, None, 'ModuleSignature',1,'Identifier',None, 'String.GUID of module with exclusion requirement.',),
('ModuleExclusion','ModuleLanguage','N',None, None, 'ModuleSignature',2,None, None, 'LanguageID of module with exclusion requirement.',),
('ModuleExclusion','ExcludedID','N',None, None, None, None, None, None, 'String.GUID of excluded module.',),
('ModuleExclusion','ExcludedLanguage','N',None, None, None, None, None, None, 'Language of excluded module.',),
('ModuleExclusion','ExcludedMaxVersion','Y',None, None, None, None, 'Version',None, 'Maximum version of excluded module.',),
('ModuleExclusion','ExcludedMinVersion','Y',None, None, None, None, 'Version',None, 'Minimum version of excluded module.',),
('MoveFile','Component_','N',None, None, 'Component',1,'Identifier',None, 'If this component is not "selected" for installation or removal, no action will be taken on the associated MoveFile entry',),
('MoveFile','DestFolder','N',None, None, None, None, 'Identifier',None, 'Name of a property whose value is assumed to resolve to the full path to the destination directory',),
('MoveFile','DestName','Y',None, None, None, None, 'Filename',None, 'Name to be given to the original file after it is moved or copied.  If blank, the destination file will be given the same name as the source file',),
('MoveFile','FileKey','N',None, None, None, None, 'Identifier',None, 'Primary key that uniquely identifies a particular MoveFile record',),
('MoveFile','Options','N',0,1,None, None, None, None, 'Integer value specifying the MoveFile operating mode, one of imfoEnum',),
('MoveFile','SourceFolder','Y',None, None, None, None, 'Identifier',None, 'Name of a property whose value is assumed to resolve to the full path to the source directory',),
('MoveFile','SourceName','Y',None, None, None, None, 'Text',None, "Name of the source file(s) to be moved or copied.  Can contain the '*' or '?' wildcards.",),
('MsiAssembly','Attributes','Y',None, None, None, None, None, None, 'Assembly attributes',),
('MsiAssembly','Feature_','N',None, None, 'Feature',1,'Identifier',None, 'Foreign key into Feature table.',),
('MsiAssembly','Component_','N',None, None, 'Component',1,'Identifier',None, 'Foreign key into Component table.',),
('MsiAssembly','File_Application','Y',None, None, 'File',1,'Identifier',None, 'Foreign key into File table, denoting the application context for private assemblies. Null for global assemblies.',),
('MsiAssembly','File_Manifest','Y',None, None, 'File',1,'Identifier',None, 'Foreign key into the File table denoting the manifest file for the assembly.',),
('MsiAssemblyName','Name','N',None, None, None, None, 'Text',None, 'The name part of the name-value pairs for the assembly name.',),
('MsiAssemblyName','Value','N',None, None, None, None, 'Text',None, 'The value part of the name-value pairs for the assembly name.',),
('MsiAssemblyName','Component_','N',None, None, 'Component',1,'Identifier',None, 'Foreign key into Component table.',),
('MsiDigitalCertificate','CertData','N',None, None, None, None, 'Binary',None, 'A certificate context blob for a signer certificate',),
('MsiDigitalCertificate','DigitalCertificate','N',None, None, None, None, 'Identifier',None, 'A unique identifier for the row',),
('MsiDigitalSignature','Table','N',None, None, None, None, None, 'Media','Reference to another table name (only Media table is supported)',),
('MsiDigitalSignature','DigitalCertificate_','N',None, None, 'MsiDigitalCertificate',1,'Identifier',None, 'Foreign key to MsiDigitalCertificate table identifying the signer certificate',),
('MsiDigitalSignature','Hash','Y',None, None, None, None, 'Binary',None, 'The encoded hash blob from the digital signature',),
('MsiDigitalSignature','SignObject','N',None, None, None, None, 'Text',None, 'Foreign key to Media table',),
('MsiFileHash','File_','N',None, None, 'File',1,'Identifier',None, 'Primary key, foreign key into File table referencing file with this hash',),
('MsiFileHash','Options','N',0,32767,None, None, None, None, 'Various options and attributes for this hash.',),
('MsiFileHash','HashPart1','N',None, None, None, None, None, None, 'Size of file in bytes (integer).',),
('MsiFileHash','HashPart2','N',None, None, None, None, None, None, 'Size of file in bytes (integer).',),
('MsiFileHash','HashPart3','N',None, None, None, None, None, None, 'Size of file in bytes (integer).',),
('MsiFileHash','HashPart4','N',None, None, None, None, None, None, 'Size of file in bytes (integer).',),
('MsiPatchHeaders','StreamRef','N',None, None, None, None, 'Identifier',None, 'Primary key. A unique identifier for the row.',),
('MsiPatchHeaders','Header','N',None, None, None, None, 'Binary',None, 'Binary stream. The patch header, used for patch validation.',),
('ODBCAttribute','Value','Y',None, None, None, None, 'Text',None, 'Value for ODBC driver attribute',),
('ODBCAttribute','Attribute','N',None, None, None, None, 'Text',None, 'Name of ODBC driver attribute',),
('ODBCAttribute','Driver_','N',None, None, 'ODBCDriver',1,'Identifier',None, 'Reference to ODBC driver in ODBCDriver table',),
('ODBCDriver','Description','N',None, None, None, None, 'Text',None, 'Text used as registered name for driver, non-localized',),
('ODBCDriver','File_','N',None, None, 'File',1,'Identifier',None, 'Reference to key driver file',),
('ODBCDriver','Component_','N',None, None, 'Component',1,'Identifier',None, 'Reference to associated component',),
('ODBCDriver','Driver','N',None, None, None, None, 'Identifier',None, 'Primary key, non-localized.internal token for driver',),
('ODBCDriver','File_Setup','Y',None, None, 'File',1,'Identifier',None, 'Optional reference to key driver setup DLL',),
('ODBCDataSource','Description','N',None, None, None, None, 'Text',None, 'Text used as registered name for data source',),
('ODBCDataSource','Component_','N',None, None, 'Component',1,'Identifier',None, 'Reference to associated component',),
('ODBCDataSource','DataSource','N',None, None, None, None, 'Identifier',None, 'Primary key, non-localized.internal token for data source',),
('ODBCDataSource','DriverDescription','N',None, None, None, None, 'Text',None, 'Reference to driver description, may be existing driver',),
('ODBCDataSource','Registration','N',0,1,None, None, None, None, 'Registration option: 0=machine, 1=user, others t.b.d.',),
('ODBCSourceAttribute','Value','Y',None, None, None, None, 'Text',None, 'Value for ODBC data source attribute',),
('ODBCSourceAttribute','Attribute','N',None, None, None, None, 'Text',None, 'Name of ODBC data source attribute',),
('ODBCSourceAttribute','DataSource_','N',None, None, 'ODBCDataSource',1,'Identifier',None, 'Reference to ODBC data source in ODBCDataSource table',),
('ODBCTranslator','Description','N',None, None, None, None, 'Text',None, 'Text used as registered name for translator',),
('ODBCTranslator','File_','N',None, None, 'File',1,'Identifier',None, 'Reference to key translator file',),
('ODBCTranslator','Component_','N',None, None, 'Component',1,'Identifier',None, 'Reference to associated component',),
('ODBCTranslator','File_Setup','Y',None, None, 'File',1,'Identifier',None, 'Optional reference to key translator setup DLL',),
('ODBCTranslator','Translator','N',None, None, None, None, 'Identifier',None, 'Primary key, non-localized.internal token for translator',),
('Patch','Sequence','N',0,32767,None, None, None, None, 'Primary key, sequence with respect to the media images; order must track cabinet order.',),
('Patch','Attributes','N',0,32767,None, None, None, None, 'Integer containing bit flags representing patch attributes',),
('Patch','File_','N',None, None, None, None, 'Identifier',None, 'Primary key, non-localized token, foreign key to File table, must match identifier in cabinet.',),
('Patch','Header','Y',None, None, None, None, 'Binary',None, 'Binary stream. The patch header, used for patch validation.',),
('Patch','PatchSize','N',0,2147483647,None, None, None, None, 'Size of patch in bytes (integer).',),
('Patch','StreamRef_','Y',None, None, None, None, 'Identifier',None, 'Identifier. Foreign key to the StreamRef column of the MsiPatchHeaders table.',),
('PatchPackage','Media_','N',0,32767,None, None, None, None, 'Foreign key to DiskId column of Media table. Indicates the disk containing the patch package.',),
('PatchPackage','PatchId','N',None, None, None, None, 'Guid',None, 'A unique string GUID representing this patch.',),
('PublishComponent','Feature_','N',None, None, 'Feature',1,'Identifier',None, 'Foreign key into the Feature table.',),
('PublishComponent','Component_','N',None, None, 'Component',1,'Identifier',None, 'Foreign key into the Component table.',),
('PublishComponent','ComponentId','N',None, None, None, None, 'Guid',None, 'A string GUID that represents the component id that will be requested by the alien product.',),
('PublishComponent','AppData','Y',None, None, None, None, 'Text',None, 'This is localisable Application specific data that can be associated with a Qualified Component.',),
('PublishComponent','Qualifier','N',None, None, None, None, 'Text',None, 'This is defined only when the ComponentId column is an Qualified Component Id. This is the Qualifier for ProvideComponentIndirect.',),
('RadioButton','Y','N',0,32767,None, None, None, None, 'The vertical coordinate of the upper left corner of the bounding rectangle of the radio button.',),
('RadioButton','Text','Y',None, None, None, None, 'Text',None, 'The visible title to be assigned to the radio button.',),
('RadioButton','Property','N',None, None, None, None, 'Identifier',None, 'A named property to be tied to this radio button. All the buttons tied to the same property become part of the same group.',),
('RadioButton','Height','N',0,32767,None, None, None, None, 'The height of the button.',),
('RadioButton','Width','N',0,32767,None, None, None, None, 'The width of the button.',),
('RadioButton','X','N',0,32767,None, None, None, None, 'The horizontal coordinate of the upper left corner of the bounding rectangle of the radio button.',),
('RadioButton','Value','N',None, None, None, None, 'Formatted',None, 'The value string associated with this button. Selecting the button will set the associated property to this value.',),
('RadioButton','Order','N',1,32767,None, None, None, None, 'A positive integer used to determine the ordering of the items within one list..The integers do not have to be consecutive.',),
('RadioButton','Help','Y',None, None, None, None, 'Text',None, 'The help strings used with the button. The text is optional.',),
('Registry','Name','Y',None, None, None, None, 'Formatted',None, 'The registry value name.',),
('Registry','Value','Y',None, None, None, None, 'Formatted',None, 'The registry value.',),
('Registry','Component_','N',None, None, 'Component',1,'Identifier',None, 'Foreign key into the Component table referencing component that controls the installing of the registry value.',),
('Registry','Key','N',None, None, None, None, 'RegPath',None, 'The key for the registry value.',),
('Registry','Registry','N',None, None, None, None, 'Identifier',None, 'Primary key, non-localized token.',),
('Registry','Root','N',-1,3,None, None, None, None, 'The predefined root key for the registry value, one of rrkEnum.',),
('RegLocator','Name','Y',None, None, None, None, 'Formatted',None, 'The registry value name.',),
('RegLocator','Type','Y',0,18,None, None, None, None, 'An integer value that determines if the registry value is a filename or a directory location or to be used as is w/o interpretation.',),
('RegLocator','Signature_','N',None, None, None, None, 'Identifier',None, 'The table key. The Signature_ represents a unique file signature and is also the foreign key in the Signature table. If the type is 0, the registry values refers a directory, and _Signature is not a foreign key.',),
('RegLocator','Key','N',None, None, None, None, 'RegPath',None, 'The key for the registry value.',),
('RegLocator','Root','N',0,3,None, None, None, None, 'The predefined root key for the registry value, one of rrkEnum.',),
('RemoveFile','Component_','N',None, None, 'Component',1,'Identifier',None, 'Foreign key referencing Component that controls the file to be removed.',),
('RemoveFile','FileKey','N',None, None, None, None, 'Identifier',None, 'Primary key used to identify a particular file entry',),
('RemoveFile','FileName','Y',None, None, None, None, 'WildCardFilename',None, 'Name of the file to be removed.',),
('RemoveFile','DirProperty','N',None, None, None, None, 'Identifier',None, 'Name of a property whose value is assumed to resolve to the full pathname to the folder of the file to be removed.',),
('RemoveFile','InstallMode','N',None, None, None, None, None, '1;2;3','Installation option, one of iimEnum.',),
('RemoveIniFile','Action','N',None, None, None, None, None, '2;4','The type of modification to be made, one of iifEnum.',),
('RemoveIniFile','Value','Y',None, None, None, None, 'Formatted',None, 'The value to be deleted. The value is required when Action is iifIniRemoveTag',),
('RemoveIniFile','Component_','N',None, None, 'Component',1,'Identifier',None, 'Foreign key into the Component table referencing component that controls the deletion of the .INI value.',),
('RemoveIniFile','FileName','N',None, None, None, None, 'Filename',None, 'The .INI file name in which to delete the information',),
('RemoveIniFile','DirProperty','Y',None, None, None, None, 'Identifier',None, 'Foreign key into the Directory table denoting the directory where the .INI file is.',),
('RemoveIniFile','Key','N',None, None, None, None, 'Formatted',None, 'The .INI file key below Section.',),
('RemoveIniFile','Section','N',None, None, None, None, 'Formatted',None, 'The .INI file Section.',),
('RemoveIniFile','RemoveIniFile','N',None, None, None, None, 'Identifier',None, 'Primary key, non-localized token.',),
('RemoveRegistry','Name','Y',None, None, None, None, 'Formatted',None, 'The registry value name.',),
('RemoveRegistry','Component_','N',None, None, 'Component',1,'Identifier',None, 'Foreign key into the Component table referencing component that controls the deletion of the registry value.',),
('RemoveRegistry','Key','N',None, None, None, None, 'RegPath',None, 'The key for the registry value.',),
('RemoveRegistry','Root','N',-1,3,None, None, None, None, 'The predefined root key for the registry value, one of rrkEnum',),
('RemoveRegistry','RemoveRegistry','N',None, None, None, None, 'Identifier',None, 'Primary key, non-localized token.',),
('ReserveCost','Component_','N',None, None, 'Component',1,'Identifier',None, 'Reserve a specified amount of space if this component is to be installed.',),
('ReserveCost','ReserveFolder','Y',None, None, None, None, 'Identifier',None, 'Name of a property whose value is assumed to resolve to the full path to the destination directory',),
('ReserveCost','ReserveKey','N',None, None, None, None, 'Identifier',None, 'Primary key that uniquely identifies a particular ReserveCost record',),
('ReserveCost','ReserveLocal','N',0,2147483647,None, None, None, None, 'Disk space to reserve if linked component is installed locally.',),
('ReserveCost','ReserveSource','N',0,2147483647,None, None, None, None, 'Disk space to reserve if linked component is installed to run from the source location.',),
('SelfReg','File_','N',None, None, 'File',1,'Identifier',None, 'Foreign key into the File table denoting the module that needs to be registered.',),
('SelfReg','Cost','Y',0,32767,None, None, None, None, 'The cost of registering the module.',),
('ServiceControl','Name','N',None, None, None, None, 'Formatted',None, 'Name of a service. /, \\, comma and space are invalid',),
('ServiceControl','Component_','N',None, None, 'Component',1,'Identifier',None, 'Required foreign key into the Component Table that controls the startup of the service',),
('ServiceControl','Event','N',0,187,None, None, None, None, 'Bit field:  Install:  0x1 = Start, 0x2 = Stop, 0x8 = Delete, Uninstall: 0x10 = Start, 0x20 = Stop, 0x80 = Delete',),
('ServiceControl','ServiceControl','N',None, None, None, None, 'Identifier',None, 'Primary key, non-localized token.',),
('ServiceControl','Arguments','Y',None, None, None, None, 'Formatted',None, 'Arguments for the service.  Separate by [~].',),
('ServiceControl','Wait','Y',0,1,None, None, None, None, 'Boolean for whether to wait for the service to fully start',),
('ServiceInstall','Name','N',None, None, None, None, 'Formatted',None, 'Internal Name of the Service',),
('ServiceInstall','Description','Y',None, None, None, None, 'Text',None, 'Description of service.',),
('ServiceInstall','Component_','N',None, None, 'Component',1,'Identifier',None, 'Required foreign key into the Component Table that controls the startup of the service',),
('ServiceInstall','Arguments','Y',None, None, None, None, 'Formatted',None, 'Arguments to include in every start of the service, passed to WinMain',),
('ServiceInstall','ServiceInstall','N',None, None, None, None, 'Identifier',None, 'Primary key, non-localized token.',),
('ServiceInstall','Dependencies','Y',None, None, None, None, 'Formatted',None, 'Other services this depends on to start.  Separate by [~], and end with [~][~]',),
('ServiceInstall','DisplayName','Y',None, None, None, None, 'Formatted',None, 'External Name of the Service',),
('ServiceInstall','ErrorControl','N',-2147483647,2147483647,None, None, None, None, 'Severity of error if service fails to start',),
('ServiceInstall','LoadOrderGroup','Y',None, None, None, None, 'Formatted',None, 'LoadOrderGroup',),
('ServiceInstall','Password','Y',None, None, None, None, 'Formatted',None, 'password to run service with.  (with StartName)',),
('ServiceInstall','ServiceType','N',-2147483647,2147483647,None, None, None, None, 'Type of the service',),
('ServiceInstall','StartName','Y',None, None, None, None, 'Formatted',None, 'User or object name to run service as',),
('ServiceInstall','StartType','N',0,4,None, None, None, None, 'Type of the service',),
('Shortcut','Name','N',None, None, None, None, 'Filename',None, 'The name of the shortcut to be created.',),
('Shortcut','Description','Y',None, None, None, None, 'Text',None, 'The description for the shortcut.',),
('Shortcut','Component_','N',None, None, 'Component',1,'Identifier',None, 'Foreign key into the Component table denoting the component whose selection gates the shortcut creation/deletion.',),
('Shortcut','Icon_','Y',None, None, 'Icon',1,'Identifier',None, 'Foreign key into the File table denoting the external icon file for the shortcut.',),
('Shortcut','IconIndex','Y',-32767,32767,None, None, None, None, 'The icon index for the shortcut.',),
('Shortcut','Directory_','N',None, None, 'Directory',1,'Identifier',None, 'Foreign key into the Directory table denoting the directory where the shortcut file is created.',),
('Shortcut','Target','N',None, None, None, None, 'Shortcut',None, 'The shortcut target. This is usually a property that is expanded to a file or a folder that the shortcut points to.',),
('Shortcut','Arguments','Y',None, None, None, None, 'Formatted',None, 'The command-line arguments for the shortcut.',),
('Shortcut','Shortcut','N',None, None, None, None, 'Identifier',None, 'Primary key, non-localized token.',),
('Shortcut','Hotkey','Y',0,32767,None, None, None, None, 'The hotkey for the shortcut. It has the virtual-key code for the key in the low-order byte, and the modifier flags in the high-order byte. ',),
('Shortcut','ShowCmd','Y',None, None, None, None, None, '1;3;7','The show command for the application window.The following values may be used.',),
('Shortcut','WkDir','Y',None, None, None, None, 'Identifier',None, 'Name of property defining location of working directory.',),
('Signature','FileName','N',None, None, None, None, 'Filename',None, 'The name of the file. This may contain a "short name|long name" pair.',),
('Signature','Signature','N',None, None, None, None, 'Identifier',None, 'The table key. The Signature represents a unique file signature.',),
('Signature','Languages','Y',None, None, None, None, 'Language',None, 'The languages supported by the file.',),
('Signature','MaxDate','Y',0,2147483647,None, None, None, None, 'The maximum creation date of the file.',),
('Signature','MaxSize','Y',0,2147483647,None, None, None, None, 'The maximum size of the file. ',),
('Signature','MaxVersion','Y',None, None, None, None, 'Text',None, 'The maximum version of the file.',),
('Signature','MinDate','Y',0,2147483647,None, None, None, None, 'The minimum creation date of the file.',),
('Signature','MinSize','Y',0,2147483647,None, None, None, None, 'The minimum size of the file.',),
('Signature','MinVersion','Y',None, None, None, None, 'Text',None, 'The minimum version of the file.',),
('TextStyle','TextStyle','N',None, None, None, None, 'Identifier',None, 'Name of the style. The primary key of this table. This name is embedded in the texts to indicate a style change.',),
('TextStyle','Color','Y',0,16777215,None, None, None, None, 'An integer indicating the color of the string in the RGB format (Red, Green, Blue each 0-255, RGB = R + 256*G + 256^2*B).',),
('TextStyle','FaceName','N',None, None, None, None, 'Text',None, 'A string indicating the name of the font used. Required. The string must be at most 31 characters long.',),
('TextStyle','Size','N',0,32767,None, None, None, None, 'The size of the font used. This size is given in our units (1/12 of the system font height). Assuming that the system font is set to 12 point size, this is equivalent to the point size.',),
('TextStyle','StyleBits','Y',0,15,None, None, None, None, 'A combination of style bits.',),
('TypeLib','Description','Y',None, None, None, None, 'Text',None, None, ),
('TypeLib','Feature_','N',None, None, 'Feature',1,'Identifier',None, 'Required foreign key into the Feature Table, specifying the feature to validate or install in order for the type library to be operational.',),
('TypeLib','Component_','N',None, None, 'Component',1,'Identifier',None, 'Required foreign key into the Component Table, specifying the component for which to return a path when called through LocateComponent.',),
('TypeLib','Directory_','Y',None, None, 'Directory',1,'Identifier',None, 'Optional. The foreign key into the Directory table denoting the path to the help file for the type library.',),
('TypeLib','Language','N',0,32767,None, None, None, None, 'The language of the library.',),
('TypeLib','Version','Y',0,16777215,None, None, None, None, 'The version of the library. The minor version is in the lower 8 bits of the integer. The major version is in the next 16 bits. ',),
('TypeLib','Cost','Y',0,2147483647,None, None, None, None, 'The cost associated with the registration of the typelib. This column is currently optional.',),
('TypeLib','LibID','N',None, None, None, None, 'Guid',None, 'The GUID that represents the library.',),
('UIText','Text','Y',None, None, None, None, 'Text',None, 'The localized version of the string.',),
('UIText','Key','N',None, None, None, None, 'Identifier',None, 'A unique key that identifies the particular string.',),
('Upgrade','Attributes','N',0,2147483647,None, None, None, None, 'The attributes of this product set.',),
('Upgrade','Language','Y',None, None, None, None, 'Language',None, 'A comma-separated list of languages for either products in this set or products not in this set.',),
('Upgrade','ActionProperty','N',None, None, None, None, 'UpperCase',None, 'The property to set when a product in this set is found.',),
('Upgrade','Remove','Y',None, None, None, None, 'Formatted',None, 'The list of features to remove when uninstalling a product from this set.  The default is "ALL".',),
('Upgrade','UpgradeCode','N',None, None, None, None, 'Guid',None, 'The UpgradeCode GUID belonging to the products in this set.',),
('Upgrade','VersionMax','Y',None, None, None, None, 'Text',None, 'The maximum ProductVersion of the products in this set.  The set may or may not include products with this particular version.',),
('Upgrade','VersionMin','Y',None, None, None, None, 'Text',None, 'The minimum ProductVersion of the products in this set.  The set may or may not include products with this particular version.',),
('Verb','Sequence','Y',0,32767,None, None, None, None, 'Order within the verbs for a particular extension. Also used simply to specify the default verb.',),
('Verb','Argument','Y',None, None, None, None, 'Formatted',None, 'Optional value for the command arguments.',),
('Verb','Extension_','N',None, None, 'Extension',1,'Text',None, 'The extension associated with the table row.',),
('Verb','Verb','N',None, None, None, None, 'Text',None, 'The verb for the command.',),
('Verb','Command','Y',None, None, None, None, 'Formatted',None, 'The command text.',),
]
