#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#define GLOBAL_ERROR_BASE                 25501

#define msierrSecureObjectsFailedCreateSD    25520
#define msierrSecureObjectsFailedSet         25521
#define msierrSecureObjectsUnknownType       25522

#define msierrXmlFileFailedRead         25530
#define msierrXmlFileFailedOpen         25531
#define msierrXmlFileFailedSelect       25532
#define msierrXmlFileFailedSave         25533

#define msierrXmlConfigFailedRead         25540
#define msierrXmlConfigFailedOpen         25541
#define msierrXmlConfigFailedSelect       25542
#define msierrXmlConfigFailedSave         25543

#define msierrFirewallCannotConnect       25580

//---------------------------------------------------------------------------
// Server CustomAction Errors
// SERVER range: 26001-26100
#define SERVER_ERROR_BASE                      26000

#define msierrIISCannotConnect                 26001
#define msierrIISFailedReadWebSite             26002
#define msierrIISFailedReadWebDirs             26003
#define msierrIISFailedReadVDirs               26004
#define msierrIISFailedReadFilters             26005
#define msierrIISFailedReadAppPool             26006
#define msierrIISFailedReadMimeMap             26007
#define msierrIISFailedReadProp                26008
#define msierrIISFailedReadWebSvcExt           26009
#define msierrIISFailedReadWebError            26010
#define msierrIISFailedReadHttpHeader          26011

#define msierrIISFailedSchedTransaction        26031
#define msierrIISFailedSchedInstallWebs        26032
#define msierrIISFailedSchedInstallWebDirs     26033
#define msierrIISFailedSchedInstallVDirs       26034
#define msierrIISFailedSchedInstallFilters     26035
#define msierrIISFailedSchedInstallAppPool     26036
#define msierrIISFailedSchedInstallProp        26037
#define msierrIISFailedSchedInstallWebSvcExt   26038

#define msierrIISFailedSchedUninstallWebs      26051
#define msierrIISFailedSchedUninstallWebDirs   26052
#define msierrIISFailedSchedUninstallVDirs     26053
#define msierrIISFailedSchedUninstallFilters   26054
#define msierrIISFailedSchedUninstallAppPool   26055
#define msierrIISFailedSchedUninstallProp      26056
#define msierrIISFailedSchedUninstallWebSvcExt 26057

#define msierrIISFailedStartTransaction        26101
#define msierrIISFailedOpenKey                 26102
#define msierrIISFailedCreateKey               26103
#define msierrIISFailedWriteData               26104
#define msierrIISFailedCreateApp               26105
#define msierrIISFailedDeleteKey               26106
#define msierrIISFailedDeleteApp               26107
#define msierrIISFailedDeleteValue             26108
#define msierrIISFailedCommitInUse             26109

#define msierrSQLFailedCreateDatabase          26201
#define msierrSQLFailedDropDatabase            26202
#define msierrSQLFailedConnectDatabase         26203
#define msierrSQLFailedExecString              26204
#define msierrSQLDatabaseAlreadyExists         26205

#define msierrPERFMONFailedRegisterDLL         26251
#define msierrPERFMONFailedUnregisterDLL       26252
#define msierrInstallPerfCounterData           26253
#define msierrUninstallPerfCounterData         26254

#define msierrSMBFailedCreate                  26301
#define msierrSMBFailedDrop                    26302

#define msierrCERTFailedOpen                   26351
#define msierrCERTFailedAdd                    26352

#define msierrUSRFailedUserCreate              26401
#define msierrUSRFailedUserCreatePswd          26402
#define msierrUSRFailedUserGroupAdd            26403
#define msierrUSRFailedUserCreateExists        26404
#define msierrUSRFailedGrantLogonAsService     26405

#define msierrDependencyMissingDependencies    26451
#define msierrDependencyHasDependents          26452

//--------------------------------------------------------------------------
// Managed code CustomAction Errors
// MANAGED range: 27000-27100
#define MANAGED_ERROR_BASE                     27000

#define msierrDotNetRuntimeRequired            27000
//---------------------------------------------------------------------------
// Public CustomAction Errors
// PUBLIC range: 28001-28100
#define PUBLIC_ERROR_BASE                            28000

#define msierrComPlusCannotConnect                   28001
#define msierrComPlusPartitionReadFailed             28002
#define msierrComPlusPartitionRoleReadFailed         28003
#define msierrComPlusUserInPartitionRoleReadFailed   28004
#define msierrComPlusPartitionUserReadFailed         28005
#define msierrComPlusApplicationReadFailed           28006
#define msierrComPlusApplicationRoleReadFailed       28007
#define msierrComPlusUserInApplicationRoleReadFailed 28008
#define msierrComPlusAssembliesReadFailed            28009
#define msierrComPlusSubscriptionReadFailed          28010
#define msierrComPlusPartitionDependency             28011
#define msierrComPlusPartitionNotFound               28012
#define msierrComPlusPartitionIdConflict             28013
#define msierrComPlusPartitionNameConflict           28014
#define msierrComPlusApplicationDependency           28015
#define msierrComPlusApplicationNotFound             28016
#define msierrComPlusApplicationIdConflict           28017
#define msierrComPlusApplicationNameConflict         28018
#define msierrComPlusApplicationRoleDependency       28019
#define msierrComPlusApplicationRoleNotFound         28020
#define msierrComPlusApplicationRoleConflict         28021
#define msierrComPlusAssemblyDependency              28022
#define msierrComPlusSubscriptionIdConflict          28023
#define msierrComPlusSubscriptionNameConflict        28024
#define msierrComPlusFailedLookupNames               28025

#define msierrMsmqCannotConnect                      28101
