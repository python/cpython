//-------------------------------------------------------------------------------------------------
// <copyright file="WixStandardBootstrapperApplication.cpp" company="Outercurve Foundation">
//   Copyright (c) 2004, Outercurve Foundation.
//   This software is released under Microsoft Reciprocal License (MS-RL).
//   The license and further copyright text can be found in the file
//   LICENSE.TXT at the root directory of the distribution.
// </copyright>
//-------------------------------------------------------------------------------------------------


#include "pch.h"

static const LPCWSTR PYBA_WINDOW_CLASS = L"PythonBA";
static const DWORD PYBA_ACQUIRE_PERCENTAGE = 30;
static const LPCWSTR PYBA_VARIABLE_BUNDLE_FILE_VERSION = L"WixBundleFileVersion";

enum PYBA_STATE {
    PYBA_STATE_INITIALIZING,
    PYBA_STATE_INITIALIZED,
    PYBA_STATE_HELP,
    PYBA_STATE_DETECTING,
    PYBA_STATE_DETECTED,
    PYBA_STATE_PLANNING,
    PYBA_STATE_PLANNED,
    PYBA_STATE_APPLYING,
    PYBA_STATE_CACHING,
    PYBA_STATE_CACHED,
    PYBA_STATE_EXECUTING,
    PYBA_STATE_EXECUTED,
    PYBA_STATE_APPLIED,
    PYBA_STATE_FAILED,
};

static const int WM_PYBA_SHOW_HELP = WM_APP + 100;
static const int WM_PYBA_DETECT_PACKAGES = WM_APP + 101;
static const int WM_PYBA_PLAN_PACKAGES = WM_APP + 102;
static const int WM_PYBA_APPLY_PACKAGES = WM_APP + 103;
static const int WM_PYBA_CHANGE_STATE = WM_APP + 104;
static const int WM_PYBA_SHOW_FAILURE = WM_APP + 105;

// This enum must be kept in the same order as the PAGE_NAMES array.
enum PAGE {
    PAGE_LOADING,
    PAGE_HELP,
    PAGE_INSTALL,
    PAGE_UPGRADE,
    PAGE_SIMPLE_INSTALL,
    PAGE_CUSTOM1,
    PAGE_CUSTOM2,
    PAGE_MODIFY,
    PAGE_PROGRESS,
    PAGE_PROGRESS_PASSIVE,
    PAGE_SUCCESS,
    PAGE_FAILURE,
    COUNT_PAGE,
};

// This array must be kept in the same order as the PAGE enum.
static LPCWSTR PAGE_NAMES[] = {
    L"Loading",
    L"Help",
    L"Install",
    L"Upgrade",
    L"SimpleInstall",
    L"Custom1",
    L"Custom2",
    L"Modify",
    L"Progress",
    L"ProgressPassive",
    L"Success",
    L"Failure",
};

enum CONTROL_ID {
    // Non-paged controls
    ID_CLOSE_BUTTON = THEME_FIRST_ASSIGN_CONTROL_ID,
    ID_MINIMIZE_BUTTON,

    // Welcome page
    ID_INSTALL_BUTTON,
    ID_INSTALL_CUSTOM_BUTTON,
    ID_INSTALL_SIMPLE_BUTTON,
    ID_INSTALL_UPGRADE_BUTTON,
    ID_INSTALL_UPGRADE_CUSTOM_BUTTON,
    ID_INSTALL_CANCEL_BUTTON,
    ID_INSTALL_LAUNCHER_ALL_USERS_CHECKBOX,

    // Customize Page
    ID_TARGETDIR_EDITBOX,
    ID_CUSTOM_ASSOCIATE_FILES_CHECKBOX,
    ID_CUSTOM_INSTALL_ALL_USERS_CHECKBOX,
    ID_CUSTOM_INSTALL_LAUNCHER_ALL_USERS_CHECKBOX,
    ID_CUSTOM_INCLUDE_LAUNCHER_HELP_LABEL,
    ID_CUSTOM_COMPILE_ALL_CHECKBOX,
    ID_CUSTOM_BROWSE_BUTTON,
    ID_CUSTOM_BROWSE_BUTTON_LABEL,
    ID_CUSTOM_INSTALL_BUTTON,
    ID_CUSTOM_NEXT_BUTTON,
    ID_CUSTOM1_BACK_BUTTON,
    ID_CUSTOM2_BACK_BUTTON,
    ID_CUSTOM1_CANCEL_BUTTON,
    ID_CUSTOM2_CANCEL_BUTTON,

    // Modify page
    ID_MODIFY_BUTTON,
    ID_REPAIR_BUTTON,
    ID_UNINSTALL_BUTTON,
    ID_MODIFY_CANCEL_BUTTON,

    // Progress page
    ID_CACHE_PROGRESS_PACKAGE_TEXT,
    ID_CACHE_PROGRESS_BAR,
    ID_CACHE_PROGRESS_TEXT,

    ID_EXECUTE_PROGRESS_PACKAGE_TEXT,
    ID_EXECUTE_PROGRESS_BAR,
    ID_EXECUTE_PROGRESS_TEXT,
    ID_EXECUTE_PROGRESS_ACTIONDATA_TEXT,

    ID_OVERALL_PROGRESS_PACKAGE_TEXT,
    ID_OVERALL_PROGRESS_BAR,
    ID_OVERALL_CALCULATED_PROGRESS_BAR,
    ID_OVERALL_PROGRESS_TEXT,

    ID_PROGRESS_CANCEL_BUTTON,

    // Success page
    ID_SUCCESS_TEXT,
    ID_SUCCESS_RESTART_TEXT,
    ID_SUCCESS_RESTART_BUTTON,
    ID_SUCCESS_CANCEL_BUTTON,
    ID_SUCCESS_MAX_PATH_BUTTON,

    // Failure page
    ID_FAILURE_LOGFILE_LINK,
    ID_FAILURE_MESSAGE_TEXT,
    ID_FAILURE_RESTART_TEXT,
    ID_FAILURE_RESTART_BUTTON,
    ID_FAILURE_CANCEL_BUTTON
};

static THEME_ASSIGN_CONTROL_ID CONTROL_ID_NAMES[] = {
    { ID_CLOSE_BUTTON, L"CloseButton" },
    { ID_MINIMIZE_BUTTON, L"MinimizeButton" },

    { ID_INSTALL_BUTTON, L"InstallButton" },
    { ID_INSTALL_CUSTOM_BUTTON, L"InstallCustomButton" },
    { ID_INSTALL_SIMPLE_BUTTON, L"InstallSimpleButton" },
    { ID_INSTALL_UPGRADE_BUTTON, L"InstallUpgradeButton" },
    { ID_INSTALL_UPGRADE_CUSTOM_BUTTON, L"InstallUpgradeCustomButton" },
    { ID_INSTALL_CANCEL_BUTTON, L"InstallCancelButton" },
    { ID_INSTALL_LAUNCHER_ALL_USERS_CHECKBOX, L"InstallLauncherAllUsers" },

    { ID_TARGETDIR_EDITBOX, L"TargetDir" },
    { ID_CUSTOM_ASSOCIATE_FILES_CHECKBOX, L"AssociateFiles" },
    { ID_CUSTOM_INSTALL_ALL_USERS_CHECKBOX, L"InstallAllUsers" },
    { ID_CUSTOM_INSTALL_LAUNCHER_ALL_USERS_CHECKBOX, L"CustomInstallLauncherAllUsers" },
    { ID_CUSTOM_INCLUDE_LAUNCHER_HELP_LABEL, L"Include_launcherHelp" },
    { ID_CUSTOM_COMPILE_ALL_CHECKBOX, L"CompileAll" },
    { ID_CUSTOM_BROWSE_BUTTON, L"CustomBrowseButton" },
    { ID_CUSTOM_BROWSE_BUTTON_LABEL, L"CustomBrowseButtonLabel" },
    { ID_CUSTOM_INSTALL_BUTTON, L"CustomInstallButton" },
    { ID_CUSTOM_NEXT_BUTTON, L"CustomNextButton" },
    { ID_CUSTOM1_BACK_BUTTON, L"Custom1BackButton" },
    { ID_CUSTOM2_BACK_BUTTON, L"Custom2BackButton" },
    { ID_CUSTOM1_CANCEL_BUTTON, L"Custom1CancelButton" },
    { ID_CUSTOM2_CANCEL_BUTTON, L"Custom2CancelButton" },

    { ID_MODIFY_BUTTON, L"ModifyButton" },
    { ID_REPAIR_BUTTON, L"RepairButton" },
    { ID_UNINSTALL_BUTTON, L"UninstallButton" },
    { ID_MODIFY_CANCEL_BUTTON, L"ModifyCancelButton" },

    { ID_CACHE_PROGRESS_PACKAGE_TEXT, L"CacheProgressPackageText" },
    { ID_CACHE_PROGRESS_BAR, L"CacheProgressbar" },
    { ID_CACHE_PROGRESS_TEXT, L"CacheProgressText" },
    { ID_EXECUTE_PROGRESS_PACKAGE_TEXT, L"ExecuteProgressPackageText" },
    { ID_EXECUTE_PROGRESS_BAR, L"ExecuteProgressbar" },
    { ID_EXECUTE_PROGRESS_TEXT, L"ExecuteProgressText" },
    { ID_EXECUTE_PROGRESS_ACTIONDATA_TEXT, L"ExecuteProgressActionDataText" },
    { ID_OVERALL_PROGRESS_PACKAGE_TEXT, L"OverallProgressPackageText" },
    { ID_OVERALL_PROGRESS_BAR, L"OverallProgressbar" },
    { ID_OVERALL_CALCULATED_PROGRESS_BAR, L"OverallCalculatedProgressbar" },
    { ID_OVERALL_PROGRESS_TEXT, L"OverallProgressText" },
    { ID_PROGRESS_CANCEL_BUTTON, L"ProgressCancelButton" },

    { ID_SUCCESS_TEXT, L"SuccessText" },
    { ID_SUCCESS_RESTART_TEXT, L"SuccessRestartText" },
    { ID_SUCCESS_RESTART_BUTTON, L"SuccessRestartButton" },
    { ID_SUCCESS_CANCEL_BUTTON, L"SuccessCancelButton" },
    { ID_SUCCESS_MAX_PATH_BUTTON, L"SuccessMaxPathButton" },

    { ID_FAILURE_LOGFILE_LINK, L"FailureLogFileLink" },
    { ID_FAILURE_MESSAGE_TEXT, L"FailureMessageText" },
    { ID_FAILURE_RESTART_TEXT, L"FailureRestartText" },
    { ID_FAILURE_RESTART_BUTTON, L"FailureRestartButton" },
    { ID_FAILURE_CANCEL_BUTTON, L"FailureCancelButton" },
};

static struct { LPCWSTR regName; LPCWSTR variableName; } OPTIONAL_FEATURES[] = {
    { L"core_d", L"Include_debug" },
    { L"core_pdb", L"Include_symbols" },
    { L"dev", L"Include_dev" },
    { L"doc", L"Include_doc" },
    { L"exe", L"Include_exe" },
    { L"lib", L"Include_lib" },
    { L"path", L"PrependPath" },
    { L"pip", L"Include_pip" },
    { L"tcltk", L"Include_tcltk" },
    { L"test", L"Include_test" },
    { L"tools", L"Include_tools" },
    { L"Shortcuts", L"Shortcuts" },
    // Include_launcher and AssociateFiles are handled separately and so do
    // not need to be included in this list.
    { nullptr, nullptr }
};



class PythonBootstrapperApplication : public CBalBaseBootstrapperApplication {
    void ShowPage(DWORD newPageId) {
        // Process each control for special handling in the new page.
        ProcessPageControls(ThemeGetPage(_theme, newPageId));

        // Enable disable controls per-page.
        if (_pageIds[PAGE_INSTALL] == newPageId ||
            _pageIds[PAGE_SIMPLE_INSTALL] == newPageId ||
            _pageIds[PAGE_UPGRADE] == newPageId) {
            InstallPage_Show();
        } else if (_pageIds[PAGE_CUSTOM1] == newPageId) {
            Custom1Page_Show();
        } else if (_pageIds[PAGE_CUSTOM2] == newPageId) {
            Custom2Page_Show();
        } else if (_pageIds[PAGE_MODIFY] == newPageId) {
            ModifyPage_Show();
        } else if (_pageIds[PAGE_SUCCESS] == newPageId) {
            SuccessPage_Show();
        } else if (_pageIds[PAGE_FAILURE] == newPageId) {
            FailurePage_Show();
        }

        // Prevent repainting while switching page to avoid ugly flickering
        _suppressPaint = TRUE;
        ThemeShowPage(_theme, newPageId, SW_SHOW);
        ThemeShowPage(_theme, _visiblePageId, SW_HIDE);
        _suppressPaint = FALSE;
        InvalidateRect(_theme->hwndParent, nullptr, TRUE);
        _visiblePageId = newPageId;

        // On the install page set the focus to the install button or
        // the next enabled control if install is disabled
        if (_pageIds[PAGE_INSTALL] == newPageId) {
            ThemeSetFocus(_theme, ID_INSTALL_BUTTON);
        } else if (_pageIds[PAGE_SIMPLE_INSTALL] == newPageId) {
            ThemeSetFocus(_theme, ID_INSTALL_SIMPLE_BUTTON);
        }
    }

    //
    // Handles control clicks
    //
    void OnCommand(CONTROL_ID id) {
        LPWSTR defaultDir = nullptr;
        LPWSTR targetDir = nullptr;
        LONGLONG elevated, crtInstalled, installAllUsers;
        BOOL checked, launcherChecked;
        WCHAR wzPath[MAX_PATH] = { };
        BROWSEINFOW browseInfo = { };
        PIDLIST_ABSOLUTE pidl = nullptr;
        DWORD pageId;
        HRESULT hr = S_OK;

        switch(id) {
        case ID_CLOSE_BUTTON:
            OnClickCloseButton();
            break;

        // Install commands
        case ID_INSTALL_SIMPLE_BUTTON: __fallthrough;
        case ID_INSTALL_UPGRADE_BUTTON: __fallthrough;
        case ID_INSTALL_BUTTON:
            SavePageSettings();

            if (!WillElevate() && !QueryElevateForCrtInstall()) {
                break;
            }

            hr = BalGetNumericVariable(L"InstallAllUsers", &installAllUsers);
            ExitOnFailure(hr, L"Failed to get install scope");

            hr = _engine->SetVariableNumeric(L"CompileAll", installAllUsers);
            ExitOnFailure(hr, L"Failed to update CompileAll");

            hr = EnsureTargetDir();
            ExitOnFailure(hr, L"Failed to set TargetDir");

            OnPlan(BOOTSTRAPPER_ACTION_INSTALL);
            break;

        case ID_CUSTOM1_BACK_BUTTON:
            SavePageSettings();
            if (_modifying) {
                GoToPage(PAGE_MODIFY);
            } else if (_upgrading) {
                GoToPage(PAGE_UPGRADE);
            } else {
                GoToPage(PAGE_INSTALL);
            }
            break;

        case ID_INSTALL_CUSTOM_BUTTON: __fallthrough;
        case ID_INSTALL_UPGRADE_CUSTOM_BUTTON: __fallthrough;
        case ID_CUSTOM2_BACK_BUTTON:
            SavePageSettings();
            GoToPage(PAGE_CUSTOM1);
            break;

        case ID_CUSTOM_NEXT_BUTTON:
            SavePageSettings();
            GoToPage(PAGE_CUSTOM2);
            break;

        case ID_CUSTOM_INSTALL_BUTTON:
            SavePageSettings();

            hr = EnsureTargetDir();
            ExitOnFailure(hr, L"Failed to set TargetDir");

            hr = BalGetStringVariable(L"TargetDir", &targetDir);
            if (SUCCEEDED(hr)) {
                // TODO: Check whether directory exists and contains another installation
                ReleaseStr(targetDir);
            }

            if (!WillElevate() && !QueryElevateForCrtInstall()) {
                break;
            }

            OnPlan(_command.action);
            break;

        case ID_INSTALL_LAUNCHER_ALL_USERS_CHECKBOX:
            checked = ThemeIsControlChecked(_theme, ID_INSTALL_LAUNCHER_ALL_USERS_CHECKBOX);
            _engine->SetVariableNumeric(L"InstallLauncherAllUsers", checked);

            ThemeControlElevates(_theme, ID_INSTALL_BUTTON, WillElevate());
            break;

        case ID_CUSTOM_INSTALL_LAUNCHER_ALL_USERS_CHECKBOX:
            checked = ThemeIsControlChecked(_theme, ID_CUSTOM_INSTALL_LAUNCHER_ALL_USERS_CHECKBOX);
            _engine->SetVariableNumeric(L"InstallLauncherAllUsers", checked);

            ThemeControlElevates(_theme, ID_CUSTOM_INSTALL_BUTTON, WillElevate());
            break;

        case ID_CUSTOM_INSTALL_ALL_USERS_CHECKBOX:
            checked = ThemeIsControlChecked(_theme, ID_CUSTOM_INSTALL_ALL_USERS_CHECKBOX);
            _engine->SetVariableNumeric(L"InstallAllUsers", checked);

            ThemeControlElevates(_theme, ID_CUSTOM_INSTALL_BUTTON, WillElevate());
            ThemeControlEnable(_theme, ID_CUSTOM_BROWSE_BUTTON_LABEL, !checked);
            if (checked) {
                _engine->SetVariableNumeric(L"CompileAll", 1);
                ThemeSendControlMessage(_theme, ID_CUSTOM_COMPILE_ALL_CHECKBOX, BM_SETCHECK, BST_CHECKED, 0);
            }
            ThemeGetTextControl(_theme, ID_TARGETDIR_EDITBOX, &targetDir);
            if (targetDir) {
                // Check the current value against the default to see
                // if we should switch it automatically.
                hr = BalGetStringVariable(
                    checked ? L"DefaultJustForMeTargetDir" : L"DefaultAllUsersTargetDir",
                    &defaultDir
                );
                
                if (SUCCEEDED(hr) && defaultDir) {
                    LPWSTR formatted = nullptr;
                    if (defaultDir[0] && SUCCEEDED(BalFormatString(defaultDir, &formatted))) {
                        if (wcscmp(formatted, targetDir) == 0) {
                            ReleaseStr(defaultDir);
                            defaultDir = nullptr;
                            ReleaseStr(formatted);
                            formatted = nullptr;
                            
                            hr = BalGetStringVariable(
                                checked ? L"DefaultAllUsersTargetDir" : L"DefaultJustForMeTargetDir",
                                &defaultDir
                            );
                            if (SUCCEEDED(hr) && defaultDir && defaultDir[0] && SUCCEEDED(BalFormatString(defaultDir, &formatted))) {
                                ThemeSetTextControl(_theme, ID_TARGETDIR_EDITBOX, formatted);
                                ReleaseStr(formatted);
                            }
                        } else {
                            ReleaseStr(formatted);
                        }
                    }
                    
                    ReleaseStr(defaultDir);
                }
            }
            break;

        case ID_CUSTOM_BROWSE_BUTTON:
            browseInfo.hwndOwner = _hWnd;
            browseInfo.pszDisplayName = wzPath;
            browseInfo.lpszTitle = _theme->sczCaption;
            browseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
            pidl = ::SHBrowseForFolderW(&browseInfo);
            if (pidl && ::SHGetPathFromIDListW(pidl, wzPath)) {
                ThemeSetTextControl(_theme, ID_TARGETDIR_EDITBOX, wzPath);
            }

            if (pidl) {
                ::CoTaskMemFree(pidl);
            }
            break;

        // Modify commands
        case ID_MODIFY_BUTTON:
            // Some variables cannot be modified
            _engine->SetVariableString(L"InstallAllUsersState", L"disable");
            _engine->SetVariableString(L"InstallLauncherAllUsersState", L"disable");
            _engine->SetVariableString(L"TargetDirState", L"disable");
            _engine->SetVariableString(L"CustomBrowseButtonState", L"disable");
            _modifying = TRUE;
            GoToPage(PAGE_CUSTOM1);
            break;

        case ID_REPAIR_BUTTON:
            OnPlan(BOOTSTRAPPER_ACTION_REPAIR);
            break;

        case ID_UNINSTALL_BUTTON:
            OnPlan(BOOTSTRAPPER_ACTION_UNINSTALL);
            break;

        case ID_SUCCESS_MAX_PATH_BUTTON:
            EnableMaxPathSupport();
            ThemeControlEnable(_theme, ID_SUCCESS_MAX_PATH_BUTTON, FALSE);
            break;
        }

    LExit:
        return;
    }

    void InstallPage_Show() {
        // Ensure the All Users install button has a UAC shield
        BOOL elevated = WillElevate();
        ThemeControlElevates(_theme, ID_INSTALL_BUTTON, elevated);
        ThemeControlElevates(_theme, ID_INSTALL_SIMPLE_BUTTON, elevated);
        ThemeControlElevates(_theme, ID_INSTALL_UPGRADE_BUTTON, elevated);
    }

    void Custom1Page_Show() {
        LONGLONG installLauncherAllUsers;

        if (FAILED(BalGetNumericVariable(L"InstallLauncherAllUsers", &installLauncherAllUsers))) {
            installLauncherAllUsers = 0;
        }

        ThemeSendControlMessage(_theme, ID_CUSTOM_INSTALL_LAUNCHER_ALL_USERS_CHECKBOX, BM_SETCHECK,
            installLauncherAllUsers ? BST_CHECKED : BST_UNCHECKED, 0);

        LOC_STRING *pLocString = nullptr;
        LPCWSTR locKey = L"#(loc.Include_launcherHelp)";
        LONGLONG detectedLauncher;

        if (SUCCEEDED(BalGetNumericVariable(L"DetectedLauncher", &detectedLauncher)) && detectedLauncher) {
            locKey = L"#(loc.Include_launcherRemove)";
        } else if (SUCCEEDED(BalGetNumericVariable(L"DetectedOldLauncher", &detectedLauncher)) && detectedLauncher) {
            locKey = L"#(loc.Include_launcherUpgrade)";
        }

        if (SUCCEEDED(LocGetString(_wixLoc, locKey, &pLocString)) && pLocString) {
            ThemeSetTextControl(_theme, ID_CUSTOM_INCLUDE_LAUNCHER_HELP_LABEL, pLocString->wzText);
        }
    }

    void Custom2Page_Show() {
        HRESULT hr;
        LONGLONG installAll, includeLauncher;
        
        if (FAILED(BalGetNumericVariable(L"InstallAllUsers", &installAll))) {
            installAll = 0;
        }

        if (WillElevate()) {
            ThemeControlElevates(_theme, ID_CUSTOM_INSTALL_BUTTON, TRUE);
            ThemeShowControl(_theme, ID_CUSTOM_BROWSE_BUTTON_LABEL, SW_HIDE);
        } else {
            ThemeControlElevates(_theme, ID_CUSTOM_INSTALL_BUTTON, FALSE);
            ThemeShowControl(_theme, ID_CUSTOM_BROWSE_BUTTON_LABEL, SW_SHOW);
        }

        if (SUCCEEDED(BalGetNumericVariable(L"Include_launcher", &includeLauncher)) && includeLauncher) {
            ThemeControlEnable(_theme, ID_CUSTOM_ASSOCIATE_FILES_CHECKBOX, TRUE);
        } else {
            ThemeSendControlMessage(_theme, ID_CUSTOM_ASSOCIATE_FILES_CHECKBOX, BM_SETCHECK, BST_UNCHECKED, 0);
            ThemeControlEnable(_theme, ID_CUSTOM_ASSOCIATE_FILES_CHECKBOX, FALSE);
        }

        LPWSTR targetDir = nullptr;
        hr = BalGetStringVariable(L"TargetDir", &targetDir);
        if (SUCCEEDED(hr) && targetDir && targetDir[0]) {
            ThemeSetTextControl(_theme, ID_TARGETDIR_EDITBOX, targetDir);
            StrFree(targetDir);
        } else if (SUCCEEDED(hr)) {
            StrFree(targetDir);
            targetDir = nullptr;

            LPWSTR defaultTargetDir = nullptr;
            hr = BalGetStringVariable(L"DefaultCustomTargetDir", &defaultTargetDir);
            if (SUCCEEDED(hr) && defaultTargetDir && !defaultTargetDir[0]) {
                StrFree(defaultTargetDir);
                defaultTargetDir = nullptr;
                
                hr = BalGetStringVariable(
                    installAll ? L"DefaultAllUsersTargetDir" : L"DefaultJustForMeTargetDir",
                    &defaultTargetDir
                );
            }
            if (SUCCEEDED(hr) && defaultTargetDir) {
                if (defaultTargetDir[0] && SUCCEEDED(BalFormatString(defaultTargetDir, &targetDir))) {
                    ThemeSetTextControl(_theme, ID_TARGETDIR_EDITBOX, targetDir);
                    StrFree(targetDir);
                }
                StrFree(defaultTargetDir);
            }
        }
    }

    void ModifyPage_Show() {
        ThemeControlEnable(_theme, ID_REPAIR_BUTTON, !_suppressRepair);
    }

    void SuccessPage_Show() {
        // on the "Success" page, check if the restart button should be enabled.
        BOOL showRestartButton = FALSE;
        LOC_STRING *successText = nullptr;
        HRESULT hr = S_OK;
        
        if (_restartRequired) {
            if (BOOTSTRAPPER_RESTART_PROMPT == _command.restart) {
                showRestartButton = TRUE;
            }
        }

        switch (_plannedAction) {
        case BOOTSTRAPPER_ACTION_INSTALL:
            hr = LocGetString(_wixLoc, L"#(loc.SuccessInstallMessage)", &successText);
            break;
        case BOOTSTRAPPER_ACTION_MODIFY:
            hr = LocGetString(_wixLoc, L"#(loc.SuccessModifyMessage)", &successText);
            break;
        case BOOTSTRAPPER_ACTION_REPAIR:
            hr = LocGetString(_wixLoc, L"#(loc.SuccessRepairMessage)", &successText);
            break;
        case BOOTSTRAPPER_ACTION_UNINSTALL:
            hr = LocGetString(_wixLoc, L"#(loc.SuccessRemoveMessage)", &successText);
            break;
        }

        if (successText) {
            LPWSTR formattedString = nullptr;
            BalFormatString(successText->wzText, &formattedString);
            if (formattedString) {
                ThemeSetTextControl(_theme, ID_SUCCESS_TEXT, formattedString);
                StrFree(formattedString);
            }
        }

        ThemeControlEnable(_theme, ID_SUCCESS_RESTART_TEXT, showRestartButton);
        ThemeControlEnable(_theme, ID_SUCCESS_RESTART_BUTTON, showRestartButton);

        if (_command.action != BOOTSTRAPPER_ACTION_INSTALL ||
            !IsWindowsVersionOrGreater(10, 0, 0)) {
            ThemeControlEnable(_theme, ID_SUCCESS_MAX_PATH_BUTTON, FALSE);
        } else {
            DWORD dataType = 0, buffer = 0, bufferLen = sizeof(buffer);
            HKEY hKey;
            LRESULT res = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Control\\FileSystem",
                0,
                KEY_READ,
                &hKey
            );
            if (res == ERROR_SUCCESS) {
                res = RegQueryValueExW(hKey, L"LongPathsEnabled", nullptr, &dataType,
                    (LPBYTE)&buffer, &bufferLen);
                RegCloseKey(hKey);
            }
            else {
                BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Failed to open SYSTEM\\CurrentControlSet\\Control\\FileSystem: error code %d", res);
            }
            if (res == ERROR_SUCCESS && dataType == REG_DWORD && buffer == 0) {
                ThemeControlElevates(_theme, ID_SUCCESS_MAX_PATH_BUTTON, TRUE);
            }
            else {
                if (res == ERROR_SUCCESS)
                    BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Failed to read LongPathsEnabled value: error code %d", res);
                else
                    BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Hiding MAX_PATH button because it is already enabled");
                ThemeControlEnable(_theme, ID_SUCCESS_MAX_PATH_BUTTON, FALSE);
            }
        }
    }

    void FailurePage_Show() {
        // on the "Failure" page, show error message and check if the restart button should be enabled.

        // if there is a log file variable then we'll assume the log file exists.
        BOOL showLogLink = (_bundle.sczLogVariable && *_bundle.sczLogVariable);
        BOOL showErrorMessage = FALSE;
        BOOL showRestartButton = FALSE;

        if (FAILED(_hrFinal)) {
            LPWSTR unformattedText = nullptr;
            LPWSTR text = nullptr;

            // If we know the failure message, use that.
            if (_failedMessage && *_failedMessage) {
                StrAllocString(&unformattedText, _failedMessage, 0);
            } else {
                // try to get the error message from the error code.
                StrAllocFromError(&unformattedText, _hrFinal, nullptr);
                if (!unformattedText || !*unformattedText) {
                    StrAllocFromError(&unformattedText, E_FAIL, nullptr);
                }
            }

            if (E_WIXSTDBA_CONDITION_FAILED == _hrFinal) {
                if (unformattedText) {
                    StrAllocString(&text, unformattedText, 0);
                }
            } else {
                StrAllocFormatted(&text, L"0x%08x - %ls", _hrFinal, unformattedText);
            }

            if (text) {
                ThemeSetTextControl(_theme, ID_FAILURE_MESSAGE_TEXT, text);
                showErrorMessage = TRUE;
            }

            ReleaseStr(text);
            ReleaseStr(unformattedText);
        }

        if (_restartRequired && BOOTSTRAPPER_RESTART_PROMPT == _command.restart) {
            showRestartButton = TRUE;
        }

        ThemeControlEnable(_theme, ID_FAILURE_LOGFILE_LINK, showLogLink);
        ThemeControlEnable(_theme, ID_FAILURE_MESSAGE_TEXT, showErrorMessage);
        ThemeControlEnable(_theme, ID_FAILURE_RESTART_TEXT, showRestartButton);
        ThemeControlEnable(_theme, ID_FAILURE_RESTART_BUTTON, showRestartButton);
    }

    static void EnableMaxPathSupport() {
        LPWSTR targetDir = nullptr, defaultDir = nullptr;
        HRESULT hr = BalGetStringVariable(L"TargetDir", &targetDir);
        if (FAILED(hr) || !targetDir || !targetDir[0]) {
            BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Failed to get TargetDir");
            return;
        }

        LPWSTR pythonw = nullptr;
        StrAllocFormatted(&pythonw, L"%ls\\pythonw.exe", targetDir);
        if (!pythonw || !pythonw[0]) {
            BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Failed to construct pythonw.exe path");
            return;
        }

        LPCWSTR arguments = L"-c \"import winreg; "
            "winreg.SetValueEx("
                "winreg.CreateKey(winreg.HKEY_LOCAL_MACHINE, "
                    "r'SYSTEM\\CurrentControlSet\\Control\\FileSystem'), "
                "'LongPathsEnabled', "
                "None, "
                "winreg.REG_DWORD, "
                "1"
            ")\"";
        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Executing %ls %ls", pythonw, arguments);
        HINSTANCE res = ShellExecuteW(0, L"runas", pythonw, arguments, NULL, SW_HIDE);
        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "return code 0x%08x", res);
    }

public: // IBootstrapperApplication
    virtual STDMETHODIMP OnStartup() {
        HRESULT hr = S_OK;
        DWORD dwUIThreadId = 0;

        // create UI thread
        _hUiThread = ::CreateThread(nullptr, 0, UiThreadProc, this, 0, &dwUIThreadId);
        if (!_hUiThread) {
            ExitWithLastError(hr, "Failed to create UI thread.");
        }

    LExit:
        return hr;
    }


    virtual STDMETHODIMP_(int) OnShutdown() {
        int nResult = IDNOACTION;

        // wait for UI thread to terminate
        if (_hUiThread) {
            ::WaitForSingleObject(_hUiThread, INFINITE);
            ReleaseHandle(_hUiThread);
        }

        // If a restart was required.
        if (_restartRequired && _allowRestart) {
            nResult = IDRESTART;
        }

        return nResult;
    }

    virtual STDMETHODIMP_(int) OnDetectRelatedMsiPackage(
        __in_z LPCWSTR wzPackageId,
        __in_z LPCWSTR /*wzProductCode*/,
        __in BOOL fPerMachine,
        __in DWORD64 /*dw64Version*/,
        __in BOOTSTRAPPER_RELATED_OPERATION operation
    ) {
        if (BOOTSTRAPPER_RELATED_OPERATION_MAJOR_UPGRADE == operation && 
            (CSTR_EQUAL == ::CompareStringW(LOCALE_NEUTRAL, 0, wzPackageId, -1, L"launcher_AllUsers", -1) ||
             CSTR_EQUAL == ::CompareStringW(LOCALE_NEUTRAL, 0, wzPackageId, -1, L"launcher_JustForMe", -1))) {
            auto hr = LoadAssociateFilesStateFromKey(_engine, fPerMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER);
            if (hr == S_OK) {
                _engine->SetVariableNumeric(L"AssociateFiles", 1);
            } else if (FAILED(hr)) {
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Failed to load AssociateFiles state: error code 0x%08X", hr);
            }

            _engine->SetVariableNumeric(L"Include_launcher", 1);
            _engine->SetVariableNumeric(L"DetectedOldLauncher", 1);
            _engine->SetVariableNumeric(L"InstallLauncherAllUsers", fPerMachine ? 1 : 0);
        }
        return CheckCanceled() ? IDCANCEL : IDNOACTION;
    }

    virtual STDMETHODIMP_(int) OnDetectRelatedBundle(
        __in LPCWSTR wzBundleId,
        __in BOOTSTRAPPER_RELATION_TYPE relationType,
        __in LPCWSTR /*wzBundleTag*/,
        __in BOOL fPerMachine,
        __in DWORD64 /*dw64Version*/,
        __in BOOTSTRAPPER_RELATED_OPERATION operation
    ) {
        BalInfoAddRelatedBundleAsPackage(&_bundle.packages, wzBundleId, relationType, fPerMachine);

        // Remember when our bundle would cause a downgrade.
        if (BOOTSTRAPPER_RELATED_OPERATION_DOWNGRADE == operation) {
            _downgradingOtherVersion = TRUE;
        } else if (BOOTSTRAPPER_RELATED_OPERATION_MAJOR_UPGRADE == operation) {
            BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Detected previous version - planning upgrade");
            _upgrading = TRUE;

            LoadOptionalFeatureStates(_engine);
        } else if (BOOTSTRAPPER_RELATED_OPERATION_NONE == operation) {
            if (_command.action == BOOTSTRAPPER_ACTION_INSTALL) {
                LOC_STRING *pLocString = nullptr;
                if (SUCCEEDED(LocGetString(_wixLoc, L"#(loc.FailureExistingInstall)", &pLocString)) && pLocString) {
                    BalFormatString(pLocString->wzText, &_failedMessage);
                } else {
                    BalFormatString(L"Cannot install [WixBundleName] because it is already installed.", &_failedMessage);
                }
                BalLog(
                    BOOTSTRAPPER_LOG_LEVEL_ERROR,
                    "Related bundle %ls is preventing install",
                    wzBundleId
                );
                SetState(PYBA_STATE_FAILED, E_WIXSTDBA_CONDITION_FAILED);
            }
        }

        return CheckCanceled() ? IDCANCEL : IDOK;
    }


    virtual STDMETHODIMP_(void) OnDetectPackageComplete(
        __in LPCWSTR wzPackageId,
        __in HRESULT hrStatus,
        __in BOOTSTRAPPER_PACKAGE_STATE state
    ) {
        if (FAILED(hrStatus)) {
            return;
        }

        BOOL detectedLauncher = FALSE;
        HKEY hkey = HKEY_LOCAL_MACHINE;
        if (CSTR_EQUAL == ::CompareStringW(LOCALE_NEUTRAL, 0, wzPackageId, -1, L"launcher_AllUsers", -1)) {
            if (BOOTSTRAPPER_PACKAGE_STATE_PRESENT == state || BOOTSTRAPPER_PACKAGE_STATE_OBSOLETE == state) {
                detectedLauncher = TRUE;
                _engine->SetVariableNumeric(L"InstallLauncherAllUsers", 1);
            }
        } else if (CSTR_EQUAL == ::CompareStringW(LOCALE_NEUTRAL, 0, wzPackageId, -1, L"launcher_JustForMe", -1)) {
            if (BOOTSTRAPPER_PACKAGE_STATE_PRESENT == state || BOOTSTRAPPER_PACKAGE_STATE_OBSOLETE == state) {
                detectedLauncher = TRUE;
                _engine->SetVariableNumeric(L"InstallLauncherAllUsers", 0);
            }
        }

        if (detectedLauncher) {
            /* When we detect the current version of the launcher. */
            _engine->SetVariableNumeric(L"Include_launcher", 1);
            _engine->SetVariableNumeric(L"DetectedLauncher", 1);
            _engine->SetVariableString(L"Include_launcherState", L"disable");
            _engine->SetVariableString(L"InstallLauncherAllUsersState", L"disable");

            auto hr = LoadAssociateFilesStateFromKey(_engine, hkey);
            if (hr == S_OK) {
                _engine->SetVariableNumeric(L"AssociateFiles", 1);
            } else if (FAILED(hr)) {
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Failed to load AssociateFiles state: error code 0x%08X", hr);
            }
        }
    }


    virtual STDMETHODIMP_(void) OnDetectComplete(__in HRESULT hrStatus) {
        if (SUCCEEDED(hrStatus) && _baFunction) {
            BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Running detect complete BA function");
            _baFunction->OnDetectComplete();
        }

        if (SUCCEEDED(hrStatus)) {
            hrStatus = EvaluateConditions();
        }

        if (SUCCEEDED(hrStatus)) {
            // Ensure the default path has been set
            hrStatus = EnsureTargetDir();
        }

        SetState(PYBA_STATE_DETECTED, hrStatus);

        // If we're not interacting with the user or we're doing a layout or we're just after a force restart
        // then automatically start planning.
        if (BOOTSTRAPPER_DISPLAY_FULL > _command.display ||
            BOOTSTRAPPER_ACTION_LAYOUT == _command.action ||
            BOOTSTRAPPER_ACTION_UNINSTALL == _command.action ||
            BOOTSTRAPPER_RESUME_TYPE_REBOOT == _command.resumeType) {
            if (SUCCEEDED(hrStatus)) {
                ::PostMessageW(_hWnd, WM_PYBA_PLAN_PACKAGES, 0, _command.action);
            }
        }
    }


    virtual STDMETHODIMP_(int) OnPlanRelatedBundle(
        __in_z LPCWSTR /*wzBundleId*/,
        __inout_z BOOTSTRAPPER_REQUEST_STATE* pRequestedState
    ) {
        return CheckCanceled() ? IDCANCEL : IDOK;
    }


    virtual STDMETHODIMP_(int) OnPlanPackageBegin(
        __in_z LPCWSTR wzPackageId,
        __inout BOOTSTRAPPER_REQUEST_STATE *pRequestState
    ) {
        HRESULT hr = S_OK;
        BAL_INFO_PACKAGE* pPackage = nullptr;

        if (_nextPackageAfterRestart) {
            // After restart we need to finish the dependency registration for our package so allow the package
            // to go present.
            if (CSTR_EQUAL == ::CompareStringW(LOCALE_NEUTRAL, 0, wzPackageId, -1, _nextPackageAfterRestart, -1)) {
                // Do not allow a repair because that could put us in a perpetual restart loop.
                if (BOOTSTRAPPER_REQUEST_STATE_REPAIR == *pRequestState) {
                    *pRequestState = BOOTSTRAPPER_REQUEST_STATE_PRESENT;
                }

                ReleaseNullStr(_nextPackageAfterRestart); // no more skipping now.
            } else {
                // not the matching package, so skip it.
                BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Skipping package: %ls, after restart because it was applied before the restart.", wzPackageId);

                *pRequestState = BOOTSTRAPPER_REQUEST_STATE_NONE;
            }
        } else if ((_plannedAction == BOOTSTRAPPER_ACTION_INSTALL || _plannedAction == BOOTSTRAPPER_ACTION_MODIFY) &&
                   SUCCEEDED(BalInfoFindPackageById(&_bundle.packages, wzPackageId, &pPackage))) {
            BOOL f = FALSE;
            if (SUCCEEDED(_engine->EvaluateCondition(pPackage->sczInstallCondition, &f)) && f) {
                *pRequestState = BOOTSTRAPPER_REQUEST_STATE_PRESENT;
            }
        }

        return CheckCanceled() ? IDCANCEL : IDOK;
    }

    virtual STDMETHODIMP_(int) OnPlanMsiFeature(
        __in_z LPCWSTR wzPackageId,
        __in_z LPCWSTR wzFeatureId,
        __inout BOOTSTRAPPER_FEATURE_STATE* pRequestedState
    ) {
        LONGLONG install;
        
        if (wcscmp(wzFeatureId, L"AssociateFiles") == 0 || wcscmp(wzFeatureId, L"Shortcuts") == 0) {
            if (SUCCEEDED(_engine->GetVariableNumeric(wzFeatureId, &install)) && install) {
                *pRequestedState = BOOTSTRAPPER_FEATURE_STATE_LOCAL;
            } else {
                *pRequestedState = BOOTSTRAPPER_FEATURE_STATE_ABSENT;
            }
        } else {
            *pRequestedState = BOOTSTRAPPER_FEATURE_STATE_LOCAL;
        }
        return CheckCanceled() ? IDCANCEL : IDNOACTION;
    }

    virtual STDMETHODIMP_(void) OnPlanComplete(__in HRESULT hrStatus) {
        if (SUCCEEDED(hrStatus) && _baFunction) {
            BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Running plan complete BA function");
            _baFunction->OnPlanComplete();
        }

        SetState(PYBA_STATE_PLANNED, hrStatus);

        if (SUCCEEDED(hrStatus)) {
            ::PostMessageW(_hWnd, WM_PYBA_APPLY_PACKAGES, 0, 0);
        }

        _startedExecution = FALSE;
        _calculatedCacheProgress = 0;
        _calculatedExecuteProgress = 0;
    }


    virtual STDMETHODIMP_(int) OnCachePackageBegin(
        __in_z LPCWSTR wzPackageId,
        __in DWORD cCachePayloads,
        __in DWORD64 dw64PackageCacheSize
    ) {
        if (wzPackageId && *wzPackageId) {
            BAL_INFO_PACKAGE* pPackage = nullptr;
            HRESULT hr = BalInfoFindPackageById(&_bundle.packages, wzPackageId, &pPackage);
            LPCWSTR wz = (SUCCEEDED(hr) && pPackage->sczDisplayName) ? pPackage->sczDisplayName : wzPackageId;

            ThemeSetTextControl(_theme, ID_CACHE_PROGRESS_PACKAGE_TEXT, wz);

            // If something started executing, leave it in the overall progress text.
            if (!_startedExecution) {
                ThemeSetTextControl(_theme, ID_OVERALL_PROGRESS_PACKAGE_TEXT, wz);
            }
        }

        return __super::OnCachePackageBegin(wzPackageId, cCachePayloads, dw64PackageCacheSize);
    }


    virtual STDMETHODIMP_(int) OnCacheAcquireProgress(
        __in_z LPCWSTR wzPackageOrContainerId,
        __in_z_opt LPCWSTR wzPayloadId,
        __in DWORD64 dw64Progress,
        __in DWORD64 dw64Total,
        __in DWORD dwOverallPercentage
    ) {
        WCHAR wzProgress[5] = { };

#ifdef DEBUG
        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "PYBA: OnCacheAcquireProgress() - container/package: %ls, payload: %ls, progress: %I64u, total: %I64u, overall progress: %u%%", wzPackageOrContainerId, wzPayloadId, dw64Progress, dw64Total, dwOverallPercentage);
#endif

        ::StringCchPrintfW(wzProgress, countof(wzProgress), L"%u%%", dwOverallPercentage);
        ThemeSetTextControl(_theme, ID_CACHE_PROGRESS_TEXT, wzProgress);

        ThemeSetProgressControl(_theme, ID_CACHE_PROGRESS_BAR, dwOverallPercentage);

        _calculatedCacheProgress = dwOverallPercentage * PYBA_ACQUIRE_PERCENTAGE / 100;
        ThemeSetProgressControl(_theme, ID_OVERALL_CALCULATED_PROGRESS_BAR, _calculatedCacheProgress + _calculatedExecuteProgress);

        SetTaskbarButtonProgress(_calculatedCacheProgress + _calculatedExecuteProgress);

        return __super::OnCacheAcquireProgress(wzPackageOrContainerId, wzPayloadId, dw64Progress, dw64Total, dwOverallPercentage);
    }


    virtual STDMETHODIMP_(int) OnCacheAcquireComplete(
        __in_z LPCWSTR wzPackageOrContainerId,
        __in_z_opt LPCWSTR wzPayloadId,
        __in HRESULT hrStatus,
        __in int nRecommendation
    ) {
        SetProgressState(hrStatus);
        return __super::OnCacheAcquireComplete(wzPackageOrContainerId, wzPayloadId, hrStatus, nRecommendation);
    }


    virtual STDMETHODIMP_(int) OnCacheVerifyComplete(
        __in_z LPCWSTR wzPackageId,
        __in_z LPCWSTR wzPayloadId,
        __in HRESULT hrStatus,
        __in int nRecommendation
    ) {
        SetProgressState(hrStatus);
        return __super::OnCacheVerifyComplete(wzPackageId, wzPayloadId, hrStatus, nRecommendation);
    }


    virtual STDMETHODIMP_(void) OnCacheComplete(__in HRESULT /*hrStatus*/) {
        ThemeSetTextControl(_theme, ID_CACHE_PROGRESS_PACKAGE_TEXT, L"");
        SetState(PYBA_STATE_CACHED, S_OK); // we always return success here and let OnApplyComplete() deal with the error.
    }


    virtual STDMETHODIMP_(int) OnError(
        __in BOOTSTRAPPER_ERROR_TYPE errorType,
        __in LPCWSTR wzPackageId,
        __in DWORD dwCode,
        __in_z LPCWSTR wzError,
        __in DWORD dwUIHint,
        __in DWORD /*cData*/,
        __in_ecount_z_opt(cData) LPCWSTR* /*rgwzData*/,
        __in int nRecommendation
    ) {
        int nResult = nRecommendation;
        LPWSTR sczError = nullptr;

        if (BOOTSTRAPPER_DISPLAY_EMBEDDED == _command.display) {
            HRESULT hr = _engine->SendEmbeddedError(dwCode, wzError, dwUIHint, &nResult);
            if (FAILED(hr)) {
                nResult = IDERROR;
            }
        } else if (BOOTSTRAPPER_DISPLAY_FULL == _command.display) {
            // If this is an authentication failure, let the engine try to handle it for us.
            if (BOOTSTRAPPER_ERROR_TYPE_HTTP_AUTH_SERVER == errorType || BOOTSTRAPPER_ERROR_TYPE_HTTP_AUTH_PROXY == errorType) {
                nResult = IDTRYAGAIN;
            } else // show a generic error message box.
            {
                BalRetryErrorOccurred(wzPackageId, dwCode);

                if (!_showingInternalUIThisPackage) {
                    // If no error message was provided, use the error code to try and get an error message.
                    if (!wzError || !*wzError || BOOTSTRAPPER_ERROR_TYPE_WINDOWS_INSTALLER != errorType) {
                        HRESULT hr = StrAllocFromError(&sczError, dwCode, nullptr);
                        if (FAILED(hr) || !sczError || !*sczError) {
                            StrAllocFormatted(&sczError, L"0x%x", dwCode);
                        }
                    }

                    nResult = ::MessageBoxW(_hWnd, sczError ? sczError : wzError, _theme->sczCaption, dwUIHint);
                }
            }

            SetProgressState(HRESULT_FROM_WIN32(dwCode));
        } else {
            // just take note of the error code and let things continue.
            BalRetryErrorOccurred(wzPackageId, dwCode);
        }

        ReleaseStr(sczError);
        return nResult;
    }


    virtual STDMETHODIMP_(int) OnExecuteMsiMessage(
        __in_z LPCWSTR wzPackageId,
        __in INSTALLMESSAGE mt,
        __in UINT uiFlags,
        __in_z LPCWSTR wzMessage,
        __in DWORD cData,
        __in_ecount_z_opt(cData) LPCWSTR* rgwzData,
        __in int nRecommendation
    ) {
#ifdef DEBUG
        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "PYBA: OnExecuteMsiMessage() - package: %ls, message: %ls", wzPackageId, wzMessage);
#endif
        if (BOOTSTRAPPER_DISPLAY_FULL == _command.display && (INSTALLMESSAGE_WARNING == mt || INSTALLMESSAGE_USER == mt)) {
            int nResult = ::MessageBoxW(_hWnd, wzMessage, _theme->sczCaption, uiFlags);
            return nResult;
        }

        if (INSTALLMESSAGE_ACTIONSTART == mt) {
            ThemeSetTextControl(_theme, ID_EXECUTE_PROGRESS_ACTIONDATA_TEXT, wzMessage);
        }

        return __super::OnExecuteMsiMessage(wzPackageId, mt, uiFlags, wzMessage, cData, rgwzData, nRecommendation);
    }


    virtual STDMETHODIMP_(int) OnProgress(__in DWORD dwProgressPercentage, __in DWORD dwOverallProgressPercentage) {
        WCHAR wzProgress[5] = { };

#ifdef DEBUG
        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "PYBA: OnProgress() - progress: %u%%, overall progress: %u%%", dwProgressPercentage, dwOverallProgressPercentage);
#endif

        ::StringCchPrintfW(wzProgress, countof(wzProgress), L"%u%%", dwOverallProgressPercentage);
        ThemeSetTextControl(_theme, ID_OVERALL_PROGRESS_TEXT, wzProgress);

        ThemeSetProgressControl(_theme, ID_OVERALL_PROGRESS_BAR, dwOverallProgressPercentage);
        SetTaskbarButtonProgress(dwOverallProgressPercentage);

        return __super::OnProgress(dwProgressPercentage, dwOverallProgressPercentage);
    }


    virtual STDMETHODIMP_(int) OnExecutePackageBegin(__in_z LPCWSTR wzPackageId, __in BOOL fExecute) {
        LPWSTR sczFormattedString = nullptr;

        _startedExecution = TRUE;

        if (wzPackageId && *wzPackageId) {
            BAL_INFO_PACKAGE* pPackage = nullptr;
            BalInfoFindPackageById(&_bundle.packages, wzPackageId, &pPackage);

            LPCWSTR wz = wzPackageId;
            if (pPackage) {
                LOC_STRING* pLocString = nullptr;

                switch (pPackage->type) {
                case BAL_INFO_PACKAGE_TYPE_BUNDLE_ADDON:
                    LocGetString(_wixLoc, L"#(loc.ExecuteAddonRelatedBundleMessage)", &pLocString);
                    break;

                case BAL_INFO_PACKAGE_TYPE_BUNDLE_PATCH:
                    LocGetString(_wixLoc, L"#(loc.ExecutePatchRelatedBundleMessage)", &pLocString);
                    break;

                case BAL_INFO_PACKAGE_TYPE_BUNDLE_UPGRADE:
                    LocGetString(_wixLoc, L"#(loc.ExecuteUpgradeRelatedBundleMessage)", &pLocString);
                    break;
                }

                if (pLocString) {
                    // If the wix developer is showing a hidden variable in the UI, then obviously they don't care about keeping it safe
                    // so don't go down the rabbit hole of making sure that this is securely freed.
                    BalFormatString(pLocString->wzText, &sczFormattedString);
                }

                wz = sczFormattedString ? sczFormattedString : pPackage->sczDisplayName ? pPackage->sczDisplayName : wzPackageId;
            }

            _showingInternalUIThisPackage = pPackage && pPackage->fDisplayInternalUI;

            ThemeSetTextControl(_theme, ID_EXECUTE_PROGRESS_PACKAGE_TEXT, wz);
            ThemeSetTextControl(_theme, ID_OVERALL_PROGRESS_PACKAGE_TEXT, wz);
        } else {
            _showingInternalUIThisPackage = FALSE;
        }

        ReleaseStr(sczFormattedString);
        return __super::OnExecutePackageBegin(wzPackageId, fExecute);
    }


    virtual int __stdcall OnExecuteProgress(
        __in_z LPCWSTR wzPackageId,
        __in DWORD dwProgressPercentage,
        __in DWORD dwOverallProgressPercentage
    ) {
        WCHAR wzProgress[8] = { };

#ifdef DEBUG
        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "PYBA: OnExecuteProgress() - package: %ls, progress: %u%%, overall progress: %u%%", wzPackageId, dwProgressPercentage, dwOverallProgressPercentage);
#endif

        ::StringCchPrintfW(wzProgress, countof(wzProgress), L"%u%%", dwOverallProgressPercentage);
        ThemeSetTextControl(_theme, ID_EXECUTE_PROGRESS_TEXT, wzProgress);

        ThemeSetProgressControl(_theme, ID_EXECUTE_PROGRESS_BAR, dwOverallProgressPercentage);

        _calculatedExecuteProgress = dwOverallProgressPercentage * (100 - PYBA_ACQUIRE_PERCENTAGE) / 100;
        ThemeSetProgressControl(_theme, ID_OVERALL_CALCULATED_PROGRESS_BAR, _calculatedCacheProgress + _calculatedExecuteProgress);

        SetTaskbarButtonProgress(_calculatedCacheProgress + _calculatedExecuteProgress);

        return __super::OnExecuteProgress(wzPackageId, dwProgressPercentage, dwOverallProgressPercentage);
    }


    virtual STDMETHODIMP_(int) OnExecutePackageComplete(
        __in_z LPCWSTR wzPackageId,
        __in HRESULT hrExitCode,
        __in BOOTSTRAPPER_APPLY_RESTART restart,
        __in int nRecommendation
    ) {
        SetProgressState(hrExitCode);

        if (_wcsnicmp(wzPackageId, L"path_", 5) == 0 && SUCCEEDED(hrExitCode)) {
            SendMessageTimeoutW(
                HWND_BROADCAST,
                WM_SETTINGCHANGE,
                0,
                reinterpret_cast<LPARAM>(L"Environment"),
                SMTO_ABORTIFHUNG,
                1000,
                nullptr
            );
        }

        int nResult = __super::OnExecutePackageComplete(wzPackageId, hrExitCode, restart, nRecommendation);

        return nResult;
    }


    virtual STDMETHODIMP_(void) OnExecuteComplete(__in HRESULT hrStatus) {
        ThemeSetTextControl(_theme, ID_EXECUTE_PROGRESS_PACKAGE_TEXT, L"");
        ThemeSetTextControl(_theme, ID_EXECUTE_PROGRESS_ACTIONDATA_TEXT, L"");
        ThemeSetTextControl(_theme, ID_OVERALL_PROGRESS_PACKAGE_TEXT, L"");
        ThemeControlEnable(_theme, ID_PROGRESS_CANCEL_BUTTON, FALSE); // no more cancel.

        SetState(PYBA_STATE_EXECUTED, S_OK); // we always return success here and let OnApplyComplete() deal with the error.
        SetProgressState(hrStatus);
    }


    virtual STDMETHODIMP_(int) OnResolveSource(
        __in_z LPCWSTR wzPackageOrContainerId,
        __in_z_opt LPCWSTR wzPayloadId,
        __in_z LPCWSTR wzLocalSource,
        __in_z_opt LPCWSTR wzDownloadSource
    ) {
        int nResult = IDERROR; // assume we won't resolve source and that is unexpected.

        if (BOOTSTRAPPER_DISPLAY_FULL == _command.display) {
            if (wzDownloadSource) {
                nResult = IDDOWNLOAD;
            } else {
                // prompt to change the source location.
                OPENFILENAMEW ofn = { };
                WCHAR wzFile[MAX_PATH] = { };

                ::StringCchCopyW(wzFile, countof(wzFile), wzLocalSource);

                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = _hWnd;
                ofn.lpstrFile = wzFile;
                ofn.nMaxFile = countof(wzFile);
                ofn.lpstrFilter = L"All Files\0*.*\0";
                ofn.nFilterIndex = 1;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                ofn.lpstrTitle = _theme->sczCaption;

                if (::GetOpenFileNameW(&ofn)) {
                    HRESULT hr = _engine->SetLocalSource(wzPackageOrContainerId, wzPayloadId, ofn.lpstrFile);
                    nResult = SUCCEEDED(hr) ? IDRETRY : IDERROR;
                } else {
                    nResult = IDCANCEL;
                }
            }
        } else if (wzDownloadSource) {
            // If doing a non-interactive install and download source is available, let's try downloading the package silently
            nResult = IDDOWNLOAD;
        }
        // else there's nothing more we can do in non-interactive mode

        return CheckCanceled() ? IDCANCEL : nResult;
    }


    virtual STDMETHODIMP_(int) OnApplyComplete(__in HRESULT hrStatus, __in BOOTSTRAPPER_APPLY_RESTART restart) {
        _restartResult = restart; // remember the restart result so we return the correct error code no matter what the user chooses to do in the UI.

        // If a restart was encountered and we are not suppressing restarts, then restart is required.
        _restartRequired = (BOOTSTRAPPER_APPLY_RESTART_NONE != restart && BOOTSTRAPPER_RESTART_NEVER < _command.restart);
        // If a restart is required and we're not displaying a UI or we are not supposed to prompt for restart then allow the restart.
        _allowRestart = _restartRequired && (BOOTSTRAPPER_DISPLAY_FULL > _command.display || BOOTSTRAPPER_RESTART_PROMPT < _command.restart);

        // If we are showing UI, wait a beat before moving to the final screen.
        if (BOOTSTRAPPER_DISPLAY_NONE < _command.display) {
            ::Sleep(250);
        }

        SetState(PYBA_STATE_APPLIED, hrStatus);
        SetTaskbarButtonProgress(100); // show full progress bar, green, yellow, or red

        return IDNOACTION;
    }

    virtual STDMETHODIMP_(void) OnLaunchApprovedExeComplete(__in HRESULT hrStatus, __in DWORD /*processId*/) {
    }


private:
    //
    // UiThreadProc - entrypoint for UI thread.
    //
    static DWORD WINAPI UiThreadProc(__in LPVOID pvContext) {
        HRESULT hr = S_OK;
        PythonBootstrapperApplication* pThis = (PythonBootstrapperApplication*)pvContext;
        BOOL comInitialized = FALSE;
        BOOL ret = FALSE;
        MSG msg = { };

        // Initialize COM and theme.
        hr = ::CoInitialize(nullptr);
        BalExitOnFailure(hr, "Failed to initialize COM.");
        comInitialized = TRUE;

        hr = ThemeInitialize(pThis->_hModule);
        BalExitOnFailure(hr, "Failed to initialize theme manager.");

        hr = pThis->InitializeData();
        BalExitOnFailure(hr, "Failed to initialize data in bootstrapper application.");

        // Create main window.
        pThis->InitializeTaskbarButton();
        hr = pThis->CreateMainWindow();
        BalExitOnFailure(hr, "Failed to create main window.");

        pThis->ValidateOperatingSystem();

        if (FAILED(pThis->_hrFinal)) {
            pThis->SetState(PYBA_STATE_FAILED, hr);
            ::PostMessageW(pThis->_hWnd, WM_PYBA_SHOW_FAILURE, 0, 0);
        } else {
            // Okay, we're ready for packages now.
            pThis->SetState(PYBA_STATE_INITIALIZED, hr);
            ::PostMessageW(pThis->_hWnd, BOOTSTRAPPER_ACTION_HELP == pThis->_command.action ? WM_PYBA_SHOW_HELP : WM_PYBA_DETECT_PACKAGES, 0, 0);
        }

        // message pump
        while (0 != (ret = ::GetMessageW(&msg, nullptr, 0, 0))) {
            if (-1 == ret) {
                hr = E_UNEXPECTED;
                BalExitOnFailure(hr, "Unexpected return value from message pump.");
            } else if (!ThemeHandleKeyboardMessage(pThis->_theme, msg.hwnd, &msg)) {
                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
            }
        }

        // Succeeded thus far, check to see if anything went wrong while actually
        // executing changes.
        if (FAILED(pThis->_hrFinal)) {
            hr = pThis->_hrFinal;
        } else if (pThis->CheckCanceled()) {
            hr = HRESULT_FROM_WIN32(ERROR_INSTALL_USEREXIT);
        }

    LExit:
        // destroy main window
        pThis->DestroyMainWindow();

        // initiate engine shutdown
        DWORD dwQuit = HRESULT_CODE(hr);
        if (BOOTSTRAPPER_APPLY_RESTART_INITIATED == pThis->_restartResult) {
            dwQuit = ERROR_SUCCESS_REBOOT_INITIATED;
        } else if (BOOTSTRAPPER_APPLY_RESTART_REQUIRED == pThis->_restartResult) {
            dwQuit = ERROR_SUCCESS_REBOOT_REQUIRED;
        }
        pThis->_engine->Quit(dwQuit);

        ReleaseTheme(pThis->_theme);
        ThemeUninitialize();

        // uninitialize COM
        if (comInitialized) {
            ::CoUninitialize();
        }

        return hr;
    }

    //
    // ParseVariablesFromUnattendXml - reads options from unattend.xml if it
    // exists
    //
    HRESULT ParseVariablesFromUnattendXml() {
        HRESULT hr = S_OK;
        LPWSTR sczUnattendXmlPath = nullptr;
        IXMLDOMDocument *pixdUnattend = nullptr;
        IXMLDOMNodeList *pNodes = nullptr;
        IXMLDOMNode *pNode = nullptr;
        long cNodes;
        DWORD dwAttr;
        LPWSTR scz = nullptr;
        BOOL bValue;
        int iValue;
        BOOL tryConvert;
        BSTR bstrValue = nullptr;

        hr = BalFormatString(L"[WixBundleOriginalSourceFolder]unattend.xml", &sczUnattendXmlPath);
        BalExitOnFailure(hr, "Failed to calculate path to unattend.xml");

        if (!FileExistsEx(sczUnattendXmlPath, &dwAttr)) {
            BalLog(BOOTSTRAPPER_LOG_LEVEL_VERBOSE, "Did not find %ls", sczUnattendXmlPath);
            hr = S_FALSE;
            goto LExit;
        }

        hr = XmlLoadDocumentFromFile(sczUnattendXmlPath, &pixdUnattend);
        BalExitOnFailure1(hr, "Failed to read %ls", sczUnattendXmlPath);

        // get the list of variables users have overridden
        hr = XmlSelectNodes(pixdUnattend, L"/Options/Option", &pNodes);
        if (S_FALSE == hr) {
            ExitFunction1(hr = S_OK);
        }
        BalExitOnFailure(hr, "Failed to select option nodes.");

        hr = pNodes->get_length((long*)&cNodes);
        BalExitOnFailure(hr, "Failed to get option node count.");

        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Reading settings from %ls", sczUnattendXmlPath);

        for (DWORD i = 0; i < cNodes; ++i) {
            hr = XmlNextElement(pNodes, &pNode, nullptr);
            BalExitOnFailure(hr, "Failed to get next node.");

            // @Name
            hr = XmlGetAttributeEx(pNode, L"Name", &scz);
            BalExitOnFailure(hr, "Failed to get @Name.");

            tryConvert = TRUE;
            hr = XmlGetAttribute(pNode, L"Value", &bstrValue);
            if (FAILED(hr) || !bstrValue || !*bstrValue) {
                hr = XmlGetText(pNode, &bstrValue);
                tryConvert = FALSE;
            }
            BalExitOnFailure(hr, "Failed to get @Value.");

            if (tryConvert &&
                CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, bstrValue, -1, L"yes", -1)) {
                _engine->SetVariableNumeric(scz, 1);
            } else if (tryConvert &&
                       CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, bstrValue, -1, L"no", -1)) {
                _engine->SetVariableNumeric(scz, 0);
            } else if (tryConvert && ::StrToIntExW(bstrValue, STIF_DEFAULT, &iValue)) {
                _engine->SetVariableNumeric(scz, iValue);
            } else {
                _engine->SetVariableString(scz, bstrValue);
            }

            ReleaseNullBSTR(bstrValue);
            ReleaseNullStr(scz);
            ReleaseNullObject(pNode);
        }

        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Finished reading from %ls", sczUnattendXmlPath);

    LExit:
        ReleaseObject(pNode);
        ReleaseObject(pNodes);
        ReleaseObject(pixdUnattend);
        ReleaseStr(sczUnattendXmlPath);

        return hr;
    }


    //
    // InitializeData - initializes all the package information.
    //
    HRESULT InitializeData() {
        HRESULT hr = S_OK;
        LPWSTR sczModulePath = nullptr;
        IXMLDOMDocument *pixdManifest = nullptr;

        hr = BalManifestLoad(_hModule, &pixdManifest);
        BalExitOnFailure(hr, "Failed to load bootstrapper application manifest.");

        hr = ParseOverridableVariablesFromXml(pixdManifest);
        BalExitOnFailure(hr, "Failed to read overridable variables.");

        hr = ParseVariablesFromUnattendXml();
        ExitOnFailure(hr, "Failed to read unattend.ini file.");

        hr = ProcessCommandLine(&_language);
        ExitOnFailure(hr, "Unknown commandline parameters.");

        hr = PathRelativeToModule(&sczModulePath, nullptr, _hModule);
        BalExitOnFailure(hr, "Failed to get module path.");

        hr = LoadLocalization(sczModulePath, _language);
        ExitOnFailure(hr, "Failed to load localization.");

        hr = LoadTheme(sczModulePath, _language);
        ExitOnFailure(hr, "Failed to load theme.");

        hr = BalInfoParseFromXml(&_bundle, pixdManifest);
        BalExitOnFailure(hr, "Failed to load bundle information.");

        hr = BalConditionsParseFromXml(&_conditions, pixdManifest, _wixLoc);
        BalExitOnFailure(hr, "Failed to load conditions from XML.");

        hr = LoadBootstrapperBAFunctions();
        BalExitOnFailure(hr, "Failed to load bootstrapper functions.");

        hr = UpdateUIStrings(_command.action);
        BalExitOnFailure(hr, "Failed to load UI strings.");

        if (_command.action == BOOTSTRAPPER_ACTION_MODIFY) {
            LoadOptionalFeatureStates(_engine);
        }

        GetBundleFileVersion();
        // don't fail if we couldn't get the version info; best-effort only
    LExit:
        ReleaseObject(pixdManifest);
        ReleaseStr(sczModulePath);

        return hr;
    }


    //
    // ProcessCommandLine - process the provided command line arguments.
    //
    HRESULT ProcessCommandLine(__inout LPWSTR* psczLanguage) {
        HRESULT hr = S_OK;
        int argc = 0;
        LPWSTR* argv = nullptr;
        LPWSTR sczVariableName = nullptr;
        LPWSTR sczVariableValue = nullptr;

        if (_command.wzCommandLine && *_command.wzCommandLine) {
            argv = ::CommandLineToArgvW(_command.wzCommandLine, &argc);
            ExitOnNullWithLastError(argv, hr, "Failed to get command line.");

            for (int i = 0; i < argc; ++i) {
                if (argv[i][0] == L'-' || argv[i][0] == L'/') {
                    if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, &argv[i][1], -1, L"lang", -1)) {
                        if (i + 1 >= argc) {
                            hr = E_INVALIDARG;
                            BalExitOnFailure(hr, "Must specify a language.");
                        }

                        ++i;

                        hr = StrAllocString(psczLanguage, &argv[i][0], 0);
                        BalExitOnFailure(hr, "Failed to copy language.");
                    } else if (CSTR_EQUAL == ::CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, &argv[i][1], -1, L"simple", -1)) {
                        _engine->SetVariableNumeric(L"SimpleInstall", 1);
                    }
                } else if (_overridableVariables) {
                    int value;
                    const wchar_t* pwc = wcschr(argv[i], L'=');
                    if (pwc) {
                        hr = StrAllocString(&sczVariableName, argv[i], pwc - argv[i]);
                        BalExitOnFailure(hr, "Failed to copy variable name.");

                        hr = DictKeyExists(_overridableVariables, sczVariableName);
                        if (E_NOTFOUND == hr) {
                            BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Ignoring attempt to set non-overridable variable: '%ls'.", sczVariableName);
                            hr = S_OK;
                            continue;
                        }
                        ExitOnFailure(hr, "Failed to check the dictionary of overridable variables.");

                        hr = StrAllocString(&sczVariableValue, ++pwc, 0);
                        BalExitOnFailure(hr, "Failed to copy variable value.");

                        if (::StrToIntEx(sczVariableValue, STIF_DEFAULT, &value)) {
                            hr = _engine->SetVariableNumeric(sczVariableName, value);
                        } else {
                            hr = _engine->SetVariableString(sczVariableName, sczVariableValue);
                        }
                        BalExitOnFailure(hr, "Failed to set variable.");
                    } else {
                        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Ignoring unknown argument: %ls", argv[i]);
                    }
                }
            }
        }

    LExit:
        if (argv) {
            ::LocalFree(argv);
        }

        ReleaseStr(sczVariableName);
        ReleaseStr(sczVariableValue);

        return hr;
    }

    HRESULT LoadLocalization(__in_z LPCWSTR wzModulePath, __in_z_opt LPCWSTR wzLanguage) {
        HRESULT hr = S_OK;
        LPWSTR sczLocPath = nullptr;
        LPCWSTR wzLocFileName = L"Default.wxl";

        hr = LocProbeForFile(wzModulePath, wzLocFileName, wzLanguage, &sczLocPath);
        BalExitOnFailure2(hr, "Failed to probe for loc file: %ls in path: %ls", wzLocFileName, wzModulePath);

        hr = LocLoadFromFile(sczLocPath, &_wixLoc);
        BalExitOnFailure1(hr, "Failed to load loc file from path: %ls", sczLocPath);

        if (WIX_LOCALIZATION_LANGUAGE_NOT_SET != _wixLoc->dwLangId) {
            ::SetThreadLocale(_wixLoc->dwLangId);
        }

        hr = StrAllocString(&_confirmCloseMessage, L"#(loc.ConfirmCancelMessage)", 0);
        ExitOnFailure(hr, "Failed to initialize confirm message loc identifier.");

        hr = LocLocalizeString(_wixLoc, &_confirmCloseMessage);
        BalExitOnFailure1(hr, "Failed to localize confirm close message: %ls", _confirmCloseMessage);

    LExit:
        ReleaseStr(sczLocPath);

        return hr;
    }


    HRESULT LoadTheme(__in_z LPCWSTR wzModulePath, __in_z_opt LPCWSTR wzLanguage) {
        HRESULT hr = S_OK;
        LPWSTR sczThemePath = nullptr;
        LPCWSTR wzThemeFileName = L"Default.thm";
        LPWSTR sczCaption = nullptr;

        hr = LocProbeForFile(wzModulePath, wzThemeFileName, wzLanguage, &sczThemePath);
        BalExitOnFailure2(hr, "Failed to probe for theme file: %ls in path: %ls", wzThemeFileName, wzModulePath);

        hr = ThemeLoadFromFile(sczThemePath, &_theme);
        BalExitOnFailure1(hr, "Failed to load theme from path: %ls", sczThemePath);

        hr = ThemeLocalize(_theme, _wixLoc);
        BalExitOnFailure1(hr, "Failed to localize theme: %ls", sczThemePath);

        // Update the caption if there are any formatted strings in it.
        // If the wix developer is showing a hidden variable in the UI, then
        // obviously they don't care about keeping it safe so don't go down the
        // rabbit hole of making sure that this is securely freed.
        hr = BalFormatString(_theme->sczCaption, &sczCaption);
        if (SUCCEEDED(hr)) {
            ThemeUpdateCaption(_theme, sczCaption);
        }

    LExit:
        ReleaseStr(sczCaption);
        ReleaseStr(sczThemePath);

        return hr;
    }


    HRESULT ParseOverridableVariablesFromXml(__in IXMLDOMDocument* pixdManifest) {
        HRESULT hr = S_OK;
        IXMLDOMNode* pNode = nullptr;
        IXMLDOMNodeList* pNodes = nullptr;
        DWORD cNodes = 0;
        LPWSTR scz = nullptr;
        BOOL hidden = FALSE;

        // get the list of variables users can override on the command line
        hr = XmlSelectNodes(pixdManifest, L"/BootstrapperApplicationData/WixStdbaOverridableVariable", &pNodes);
        if (S_FALSE == hr) {
            ExitFunction1(hr = S_OK);
        }
        ExitOnFailure(hr, "Failed to select overridable variable nodes.");

        hr = pNodes->get_length((long*)&cNodes);
        ExitOnFailure(hr, "Failed to get overridable variable node count.");

        if (cNodes) {
            hr = DictCreateStringList(&_overridableVariables, 32, DICT_FLAG_NONE);
            ExitOnFailure(hr, "Failed to create the string dictionary.");

            for (DWORD i = 0; i < cNodes; ++i) {
                hr = XmlNextElement(pNodes, &pNode, nullptr);
                ExitOnFailure(hr, "Failed to get next node.");

                // @Name
                hr = XmlGetAttributeEx(pNode, L"Name", &scz);
                ExitOnFailure(hr, "Failed to get @Name.");

                hr = XmlGetYesNoAttribute(pNode, L"Hidden", &hidden);

                if (!hidden) {
                    hr = DictAddKey(_overridableVariables, scz);
                    ExitOnFailure1(hr, "Failed to add \"%ls\" to the string dictionary.", scz);
                }

                // prepare next iteration
                ReleaseNullObject(pNode);
            }
        }

    LExit:
        ReleaseObject(pNode);
        ReleaseObject(pNodes);
        ReleaseStr(scz);
        return hr;
    }


    //
    // Get the file version of the bootstrapper and record in bootstrapper log file
    //
    HRESULT GetBundleFileVersion() {
        HRESULT hr = S_OK;
        ULARGE_INTEGER uliVersion = { };
        LPWSTR sczCurrentPath = nullptr;

        hr = PathForCurrentProcess(&sczCurrentPath, nullptr);
        BalExitOnFailure(hr, "Failed to get bundle path.");

        hr = FileVersion(sczCurrentPath, &uliVersion.HighPart, &uliVersion.LowPart);
        BalExitOnFailure(hr, "Failed to get bundle file version.");

        hr = _engine->SetVariableVersion(PYBA_VARIABLE_BUNDLE_FILE_VERSION, uliVersion.QuadPart);
        BalExitOnFailure(hr, "Failed to set WixBundleFileVersion variable.");

    LExit:
        ReleaseStr(sczCurrentPath);

        return hr;
    }


    //
    // CreateMainWindow - creates the main install window.
    //
    HRESULT CreateMainWindow() {
        HRESULT hr = S_OK;
        HICON hIcon = reinterpret_cast<HICON>(_theme->hIcon);
        WNDCLASSW wc = { };
        DWORD dwWindowStyle = 0;
        int x = CW_USEDEFAULT;
        int y = CW_USEDEFAULT;
        POINT ptCursor = { };
        HMONITOR hMonitor = nullptr;
        MONITORINFO mi = { };
        COLORREF fg, bg;
        HBRUSH bgBrush;

        // If the theme did not provide an icon, try using the icon from the bundle engine.
        if (!hIcon) {
            HMODULE hBootstrapperEngine = ::GetModuleHandleW(nullptr);
            if (hBootstrapperEngine) {
                hIcon = ::LoadIconW(hBootstrapperEngine, MAKEINTRESOURCEW(1));
            }
        }

        fg = RGB(0, 0, 0);
        bg = RGB(255, 255, 255);
        bgBrush = (HBRUSH)(COLOR_WINDOW+1);
        if (_theme->dwFontId < _theme->cFonts) {
            THEME_FONT *font = &_theme->rgFonts[_theme->dwFontId];
            fg = font->crForeground;
            bg = font->crBackground;
            bgBrush = font->hBackground;
            RemapColor(&fg, &bg, &bgBrush);
        }

        // Register the window class and create the window.
        wc.lpfnWndProc = PythonBootstrapperApplication::WndProc;
        wc.hInstance = _hModule;
        wc.hIcon = hIcon;
        wc.hCursor = ::LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
        wc.hbrBackground = bgBrush;
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = PYBA_WINDOW_CLASS;
        if (!::RegisterClassW(&wc)) {
            ExitWithLastError(hr, "Failed to register window.");
        }

        _registered = TRUE;

        // Calculate the window style based on the theme style and command display value.
        dwWindowStyle = _theme->dwStyle;
        if (BOOTSTRAPPER_DISPLAY_NONE >= _command.display) {
            dwWindowStyle &= ~WS_VISIBLE;
        }

        // Don't show the window if there is a splash screen (it will be made visible when the splash screen is hidden)
        if (::IsWindow(_command.hwndSplashScreen)) {
            dwWindowStyle &= ~WS_VISIBLE;
        }

        // Center the window on the monitor with the mouse.
        if (::GetCursorPos(&ptCursor)) {
            hMonitor = ::MonitorFromPoint(ptCursor, MONITOR_DEFAULTTONEAREST);
            if (hMonitor) {
                mi.cbSize = sizeof(mi);
                if (::GetMonitorInfoW(hMonitor, &mi)) {
                    x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - _theme->nWidth) / 2;
                    y = mi.rcWork.top + (mi.rcWork.bottom - mi.rcWork.top - _theme->nHeight) / 2;
                }
            }
        }

        _hWnd = ::CreateWindowExW(
            0,
            wc.lpszClassName,
            _theme->sczCaption,
            dwWindowStyle,
            x,
            y,
            _theme->nWidth,
            _theme->nHeight,
            HWND_DESKTOP,
            nullptr,
            _hModule,
            this
        );
        ExitOnNullWithLastError(_hWnd, hr, "Failed to create window.");

        hr = S_OK;

    LExit:
        return hr;
    }


    //
    // InitializeTaskbarButton - initializes taskbar button for progress.
    //
    void InitializeTaskbarButton() {
        HRESULT hr = S_OK;

        hr = ::CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_ALL, __uuidof(ITaskbarList3), reinterpret_cast<LPVOID*>(&_taskbarList));
        if (REGDB_E_CLASSNOTREG == hr) {
            // not supported before Windows 7
            ExitFunction1(hr = S_OK);
        }
        BalExitOnFailure(hr, "Failed to create ITaskbarList3. Continuing.");

        _taskbarButtonCreatedMessage = ::RegisterWindowMessageW(L"TaskbarButtonCreated");
        BalExitOnNullWithLastError(_taskbarButtonCreatedMessage, hr, "Failed to get TaskbarButtonCreated message. Continuing.");

    LExit:
        return;
    }

    //
    // DestroyMainWindow - clean up all the window registration.
    //
    void DestroyMainWindow() {
        if (::IsWindow(_hWnd)) {
            ::DestroyWindow(_hWnd);
            _hWnd = nullptr;
            _taskbarButtonOK = FALSE;
        }

        if (_registered) {
            ::UnregisterClassW(PYBA_WINDOW_CLASS, _hModule);
            _registered = FALSE;
        }
    }


    //
    // WndProc - standard windows message handler.
    //
    static LRESULT CALLBACK WndProc(
        __in HWND hWnd,
        __in UINT uMsg,
        __in WPARAM wParam,
        __in LPARAM lParam
    ) {
#pragma warning(suppress:4312)
        auto pBA = reinterpret_cast<PythonBootstrapperApplication*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));

        switch (uMsg) {
        case WM_NCCREATE: {
            LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
            pBA = reinterpret_cast<PythonBootstrapperApplication*>(lpcs->lpCreateParams);
#pragma warning(suppress:4244)
            ::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pBA));
            break;
        }

        case WM_NCDESTROY: {
            LRESULT lres = ThemeDefWindowProc(pBA ? pBA->_theme : nullptr, hWnd, uMsg, wParam, lParam);
            ::SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
            return lres;
        }

        case WM_CREATE: 
            if (!pBA->OnCreate(hWnd)) {
                return -1;
            }
            break;

        case WM_QUERYENDSESSION:
            return IDCANCEL != pBA->OnSystemShutdown(static_cast<DWORD>(lParam), IDCANCEL);

        case WM_CLOSE:
            // If the user chose not to close, do *not* let the default window proc handle the message.
            if (!pBA->OnClose()) {
                return 0;
            }
            break;

        case WM_DESTROY:
            ::PostQuitMessage(0);
            break;

        case WM_PAINT: __fallthrough;
        case WM_ERASEBKGND:
            if (pBA && pBA->_suppressPaint) {
                return TRUE;
            }
            break;

        case WM_PYBA_SHOW_HELP:
            pBA->OnShowHelp();
            return 0;

        case WM_PYBA_DETECT_PACKAGES:
            pBA->OnDetect();
            return 0;

        case WM_PYBA_PLAN_PACKAGES:
            pBA->OnPlan(static_cast<BOOTSTRAPPER_ACTION>(lParam));
            return 0;

        case WM_PYBA_APPLY_PACKAGES:
            pBA->OnApply();
            return 0;

        case WM_PYBA_CHANGE_STATE:
            pBA->OnChangeState(static_cast<PYBA_STATE>(lParam));
            return 0;

        case WM_PYBA_SHOW_FAILURE:
            pBA->OnShowFailure();
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            // Customize commands
            // Success/failure commands
            case ID_SUCCESS_RESTART_BUTTON: __fallthrough;
            case ID_FAILURE_RESTART_BUTTON:
                pBA->OnClickRestartButton();
                return 0;

            case IDCANCEL: __fallthrough;
            case ID_INSTALL_CANCEL_BUTTON: __fallthrough;
            case ID_CUSTOM1_CANCEL_BUTTON: __fallthrough;
            case ID_CUSTOM2_CANCEL_BUTTON: __fallthrough;
            case ID_MODIFY_CANCEL_BUTTON: __fallthrough;
            case ID_PROGRESS_CANCEL_BUTTON: __fallthrough;
            case ID_SUCCESS_CANCEL_BUTTON: __fallthrough;
            case ID_FAILURE_CANCEL_BUTTON: __fallthrough;
            case ID_CLOSE_BUTTON:
                pBA->OnCommand(ID_CLOSE_BUTTON);
                return 0;

            default:
                pBA->OnCommand((CONTROL_ID)LOWORD(wParam));
            }
            break;

        case WM_NOTIFY:
            if (lParam) {
                LPNMHDR pnmhdr = reinterpret_cast<LPNMHDR>(lParam);
                switch (pnmhdr->code) {
                case NM_CLICK: __fallthrough;
                case NM_RETURN:
                    switch (static_cast<DWORD>(pnmhdr->idFrom)) {
                    case ID_FAILURE_LOGFILE_LINK:
                        pBA->OnClickLogFileLink();
                        return 1;
                    }
                }
            }
            break;

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
            if (pBA) {
                HBRUSH brush = nullptr;
                if (pBA->SetControlColor((HWND)lParam, (HDC)wParam, &brush)) {
                    return (LRESULT)brush;
                }
            }
            break;
        }

        if (pBA && pBA->_taskbarList && uMsg == pBA->_taskbarButtonCreatedMessage) {
            pBA->_taskbarButtonOK = TRUE;
            return 0;
        }

        return ThemeDefWindowProc(pBA ? pBA->_theme : nullptr, hWnd, uMsg, wParam, lParam);
    }

    //
    // OnCreate - finishes loading the theme.
    //
    BOOL OnCreate(__in HWND hWnd) {
        HRESULT hr = S_OK;

        hr = ThemeLoadControls(_theme, hWnd, CONTROL_ID_NAMES, countof(CONTROL_ID_NAMES));
        BalExitOnFailure(hr, "Failed to load theme controls.");

        C_ASSERT(COUNT_PAGE == countof(PAGE_NAMES));
        C_ASSERT(countof(_pageIds) == countof(PAGE_NAMES));

        ThemeGetPageIds(_theme, PAGE_NAMES, _pageIds, countof(_pageIds));

        // Initialize the text on all "application" (non-page) controls.
        for (DWORD i = 0; i < _theme->cControls; ++i) {
            THEME_CONTROL* pControl = _theme->rgControls + i;
            LPWSTR text = nullptr;

            if (!pControl->wPageId && pControl->sczText && *pControl->sczText) {
                HRESULT hrFormat;
                
                // If the wix developer is showing a hidden variable in the UI,
                // then obviously they don't care about keeping it safe so don't
                // go down the rabbit hole of making sure that this is securely
                // freed.
                hrFormat = BalFormatString(pControl->sczText, &text);
                if (SUCCEEDED(hrFormat)) {
                    ThemeSetTextControl(_theme, pControl->wId, text);
                    ReleaseStr(text);
                }
            }
        }

    LExit:
        return SUCCEEDED(hr);
    }

    void RemapColor(COLORREF *fg, COLORREF *bg, HBRUSH *bgBrush) {
        if (*fg == RGB(0, 0, 0)) {
            *fg = GetSysColor(COLOR_WINDOWTEXT);
        } else if (*fg == RGB(128, 128, 128)) {
            *fg = GetSysColor(COLOR_GRAYTEXT);
        }
        if (*bgBrush && *bg == RGB(255, 255, 255)) {
            *bg = GetSysColor(COLOR_WINDOW);
            *bgBrush = GetSysColorBrush(COLOR_WINDOW);
        }
    }

    BOOL SetControlColor(HWND hWnd, HDC hDC, HBRUSH *brush) {
        for (int i = 0; i < _theme->cControls; ++i) {
            if (_theme->rgControls[i].hWnd != hWnd) {
                continue;
            }

            DWORD fontId = _theme->rgControls[i].dwFontId;
            if (fontId > _theme->cFonts) {
                fontId = 0;
            }
            THEME_FONT *fnt = &_theme->rgFonts[fontId];

            COLORREF fg = fnt->crForeground, bg = fnt->crBackground;
            *brush = fnt->hBackground;
            RemapColor(&fg, &bg, brush);
            ::SetTextColor(hDC, fg);
            ::SetBkColor(hDC, bg);

            return TRUE;
        }
        return FALSE;
    }

    //
    // OnShowFailure - display the failure page.
    //
    void OnShowFailure() {
        SetState(PYBA_STATE_FAILED, S_OK);

        // If the UI should be visible, display it now and hide the splash screen
        if (BOOTSTRAPPER_DISPLAY_NONE < _command.display) {
            ::ShowWindow(_theme->hwndParent, SW_SHOW);
        }

        _engine->CloseSplashScreen();

        return;
    }


    //
    // OnShowHelp - display the help page.
    //
    void OnShowHelp() {
        SetState(PYBA_STATE_HELP, S_OK);

        // If the UI should be visible, display it now and hide the splash screen
        if (BOOTSTRAPPER_DISPLAY_NONE < _command.display) {
            ::ShowWindow(_theme->hwndParent, SW_SHOW);
        }

        _engine->CloseSplashScreen();

        return;
    }


    //
    // OnDetect - start the processing of packages.
    //
    void OnDetect() {
        HRESULT hr = S_OK;

        if (_baFunction) {
            BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Running detect BA function");
            hr = _baFunction->OnDetect();
            BalExitOnFailure(hr, "Failed calling detect BA function.");
        }

        SetState(PYBA_STATE_DETECTING, hr);

        // If the UI should be visible, display it now and hide the splash screen
        if (BOOTSTRAPPER_DISPLAY_NONE < _command.display) {
            ::ShowWindow(_theme->hwndParent, SW_SHOW);
        }

        _engine->CloseSplashScreen();

        // Tell the core we're ready for the packages to be processed now.
        hr = _engine->Detect();
        BalExitOnFailure(hr, "Failed to start detecting chain.");

    LExit:
        if (FAILED(hr)) {
            SetState(PYBA_STATE_DETECTING, hr);
        }

        return;
    }

    HRESULT UpdateUIStrings(__in BOOTSTRAPPER_ACTION action) {
        HRESULT hr = S_OK;
        LPCWSTR likeInstalling = nullptr;
        LPCWSTR likeInstallation = nullptr;
        switch (action) {
        case BOOTSTRAPPER_ACTION_INSTALL:
            likeInstalling = L"Installing";
            likeInstallation = L"Installation";
            break;
        case BOOTSTRAPPER_ACTION_MODIFY:
            // For modify, we actually want to pass INSTALL
            action = BOOTSTRAPPER_ACTION_INSTALL;
            likeInstalling = L"Modifying";
            likeInstallation = L"Modification";
            break;
        case BOOTSTRAPPER_ACTION_REPAIR:
            likeInstalling = L"Repairing";
            likeInstallation = L"Repair";
            break;
        case BOOTSTRAPPER_ACTION_UNINSTALL:
            likeInstalling = L"Uninstalling";
            likeInstallation = L"Uninstallation";
            break;
        }

        if (likeInstalling) {
            LPWSTR locName = nullptr;
            LOC_STRING *locText = nullptr;
            hr = StrAllocFormatted(&locName, L"#(loc.%ls)", likeInstalling);
            if (SUCCEEDED(hr)) {
                hr = LocGetString(_wixLoc, locName, &locText);
                ReleaseStr(locName);
            }
            _engine->SetVariableString(
                L"ActionLikeInstalling",
                SUCCEEDED(hr) && locText ? locText->wzText : likeInstalling
            );
        }

        if (likeInstallation) {
            LPWSTR locName = nullptr;
            LOC_STRING *locText = nullptr;
            hr = StrAllocFormatted(&locName, L"#(loc.%ls)", likeInstallation);
            if (SUCCEEDED(hr)) {
                hr = LocGetString(_wixLoc, locName, &locText);
                ReleaseStr(locName);
            }
            _engine->SetVariableString(
                L"ActionLikeInstallation",
                SUCCEEDED(hr) && locText ? locText->wzText : likeInstallation
            );
        }
        return hr;
    }

    //
    // OnPlan - plan the detected changes.
    //
    void OnPlan(__in BOOTSTRAPPER_ACTION action) {
        HRESULT hr = S_OK;

        _plannedAction = action;

        hr = UpdateUIStrings(action);
        BalExitOnFailure(hr, "Failed to update strings");

        // If we are going to apply a downgrade, bail.
        if (_downgradingOtherVersion && BOOTSTRAPPER_ACTION_UNINSTALL < action) {
            if (_suppressDowngradeFailure) {
                BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "A newer version of this product is installed but downgrade failure has been suppressed; continuing...");
            } else {
                hr = HRESULT_FROM_WIN32(ERROR_PRODUCT_VERSION);
                BalExitOnFailure(hr, "Cannot install a product when a newer version is installed.");
            }
        }

        SetState(PYBA_STATE_PLANNING, hr);

        if (_baFunction) {
            BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Running plan BA function");
            _baFunction->OnPlan();
        }

        hr = _engine->Plan(action);
        BalExitOnFailure(hr, "Failed to start planning packages.");

    LExit:
        if (FAILED(hr)) {
            SetState(PYBA_STATE_PLANNING, hr);
        }

        return;
    }


    //
    // OnApply - apply the packages.
    //
    void OnApply() {
        HRESULT hr = S_OK;

        SetState(PYBA_STATE_APPLYING, hr);
        SetProgressState(hr);
        SetTaskbarButtonProgress(0);

        hr = _engine->Apply(_hWnd);
        BalExitOnFailure(hr, "Failed to start applying packages.");

        ThemeControlEnable(_theme, ID_PROGRESS_CANCEL_BUTTON, TRUE); // ensure the cancel button is enabled before starting.

    LExit:
        if (FAILED(hr)) {
            SetState(PYBA_STATE_APPLYING, hr);
        }

        return;
    }


    //
    // OnChangeState - change state.
    //
    void OnChangeState(__in PYBA_STATE state) {
        LPWSTR unformattedText = nullptr;

        _state = state;

        // If our install is at the end (success or failure) and we're not showing full UI
        // then exit (prompt for restart if required).
        if ((PYBA_STATE_APPLIED <= _state && BOOTSTRAPPER_DISPLAY_FULL > _command.display)) {
            // If a restart was required but we were not automatically allowed to
            // accept the reboot then do the prompt.
            if (_restartRequired && !_allowRestart) {
                StrAllocFromError(&unformattedText, HRESULT_FROM_WIN32(ERROR_SUCCESS_REBOOT_REQUIRED), nullptr);

                _allowRestart = IDOK == ::MessageBoxW(
                    _hWnd,
                    unformattedText ? unformattedText : L"The requested operation is successful. Changes will not be effective until the system is rebooted.",
                    _theme->sczCaption,
                    MB_ICONEXCLAMATION | MB_OKCANCEL
                );
            }

            // Quietly exit.
            ::PostMessageW(_hWnd, WM_CLOSE, 0, 0);
        } else { // try to change the pages.
            DWORD newPageId = 0;
            DeterminePageId(_state, &newPageId);

            if (_visiblePageId != newPageId) {
                ShowPage(newPageId);
            }
        }

        ReleaseStr(unformattedText);
    }

    //
    // Called before showing a page to handle all controls.
    //
    void ProcessPageControls(THEME_PAGE *pPage) {
        if (!pPage) {
            return;
        }

        for (DWORD i = 0; i < pPage->cControlIndices; ++i) {
            THEME_CONTROL* pControl = _theme->rgControls + pPage->rgdwControlIndices[i];
            BOOL enableControl = TRUE;

            // If this is a named control, try to set its default state.
            if (pControl->sczName && *pControl->sczName) {
                // If this is a checkable control, try to set its default state
                // to the state of a matching named Burn variable.
                if (IsCheckable(pControl)) {
                    LONGLONG llValue = 0;
                    HRESULT hr = BalGetNumericVariable(pControl->sczName, &llValue);

                    // If the control value isn't set then disable it.
                    if (!SUCCEEDED(hr)) {
                        enableControl = FALSE;
                    } else {
                        ThemeSendControlMessage(
                            _theme,
                            pControl->wId,
                            BM_SETCHECK,
                            SUCCEEDED(hr) && llValue ? BST_CHECKED : BST_UNCHECKED,
                            0
                        );
                    }
                }

                // Hide or disable controls based on the control name with 'State' appended
                LPWSTR controlName = nullptr;
                HRESULT hr = StrAllocFormatted(&controlName, L"%lsState", pControl->sczName);
                if (SUCCEEDED(hr)) {
                    LPWSTR controlState = nullptr;
                    hr = BalGetStringVariable(controlName, &controlState);
                    if (SUCCEEDED(hr) && controlState && *controlState) {
                        if (controlState[0] == '[') {
                            LPWSTR formatted = nullptr;
                            if (SUCCEEDED(BalFormatString(controlState, &formatted))) {
                                StrFree(controlState);
                                controlState = formatted;
                            }
                        }

                        if (CSTR_EQUAL == ::CompareStringW(LOCALE_NEUTRAL, 0, controlState, -1, L"disable", -1)) {
                            BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Disable control %ls", pControl->sczName);
                            enableControl = FALSE;
                        } else if (CSTR_EQUAL == ::CompareStringW(LOCALE_NEUTRAL, 0, controlState, -1, L"hide", -1)) {
                            BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Hide control %ls", pControl->sczName);
                            // TODO: This doesn't work
                            ThemeShowControl(_theme, pControl->wId, SW_HIDE);
                        } else {
                            // An explicit state can override the lack of a
                            // backing variable.
                            enableControl = TRUE;
                        }
                    }
                    StrFree(controlState);
                }
                StrFree(controlName);
                controlName = nullptr;


                // If a command link has a note, then add it.
                if ((pControl->dwStyle & BS_TYPEMASK) == BS_COMMANDLINK ||
                    (pControl->dwStyle & BS_TYPEMASK) == BS_DEFCOMMANDLINK) {
                    hr = StrAllocFormatted(&controlName, L"#(loc.%lsNote)", pControl->sczName);
                    if (SUCCEEDED(hr)) {
                        LOC_STRING *locText = nullptr;
                        hr = LocGetString(_wixLoc, controlName, &locText);
                        if (SUCCEEDED(hr) && locText && locText->wzText && locText->wzText[0]) {
                            LPWSTR text = nullptr;
                            hr = BalFormatString(locText->wzText, &text);
                            if (SUCCEEDED(hr) && text && text[0]) {
                                ThemeSendControlMessage(_theme, pControl->wId, BCM_SETNOTE, 0, (LPARAM)text);
                                ReleaseStr(text);
                                text = nullptr;
                            }
                        }
                        ReleaseStr(controlName);
                        controlName = nullptr;
                    }
                    hr = S_OK;
                }
            }

            ThemeControlEnable(_theme, pControl->wId, enableControl);

            // Format the text in each of the new page's controls
            if (pControl->sczText && *pControl->sczText) {
                // If the wix developer is showing a hidden variable
                // in the UI, then obviously they don't care about
                // keeping it safe so don't go down the rabbit hole
                // of making sure that this is securely freed.
                LPWSTR text = nullptr;
                HRESULT hr = BalFormatString(pControl->sczText, &text);
                if (SUCCEEDED(hr)) {
                    ThemeSetTextControl(_theme, pControl->wId, text);
                }
            }
        }
    }

    //
    // OnClose - called when the window is trying to be closed.
    //
    BOOL OnClose() {
        BOOL close = FALSE;

        // If we've already succeeded or failed or showing the help page, just close (prompts are annoying if the bootstrapper is done).
        if (PYBA_STATE_APPLIED <= _state || PYBA_STATE_HELP == _state) {
            close = TRUE;
        } else {
            // prompt the user or force the cancel if there is no UI.
            close = PromptCancel(
                _hWnd,
                BOOTSTRAPPER_DISPLAY_FULL != _command.display,
                _confirmCloseMessage ? _confirmCloseMessage : L"Are you sure you want to cancel?",
                _theme->sczCaption
            );
        }

        // If we're doing progress then we never close, we just cancel to let rollback occur.
        if (PYBA_STATE_APPLYING <= _state && PYBA_STATE_APPLIED > _state) {
            // If we canceled disable cancel button since clicking it again is silly.
            if (close) {
                ThemeControlEnable(_theme, ID_PROGRESS_CANCEL_BUTTON, FALSE);
            }

            close = FALSE;
        }

        return close;
    }

    //
    // OnClickCloseButton - close the application.
    //
    void OnClickCloseButton() {
        ::SendMessageW(_hWnd, WM_CLOSE, 0, 0);
    }



    //
    // OnClickRestartButton - allows the restart and closes the app.
    //
    void OnClickRestartButton() {
        AssertSz(_restartRequired, "Restart must be requested to be able to click on the restart button.");

        _allowRestart = TRUE;
        ::SendMessageW(_hWnd, WM_CLOSE, 0, 0);

        return;
    }


    //
    // OnClickLogFileLink - show the log file.
    //
    void OnClickLogFileLink() {
        HRESULT hr = S_OK;
        LPWSTR sczLogFile = nullptr;

        hr = BalGetStringVariable(_bundle.sczLogVariable, &sczLogFile);
        BalExitOnFailure1(hr, "Failed to get log file variable '%ls'.", _bundle.sczLogVariable);

        hr = ShelExec(L"notepad.exe", sczLogFile, L"open", nullptr, SW_SHOWDEFAULT, _hWnd, nullptr);
        BalExitOnFailure1(hr, "Failed to open log file target: %ls", sczLogFile);

    LExit:
        ReleaseStr(sczLogFile);

        return;
    }


    //
    // SetState
    //
    void SetState(__in PYBA_STATE state, __in HRESULT hrStatus) {
        if (FAILED(hrStatus)) {
            _hrFinal = hrStatus;
        }

        if (FAILED(_hrFinal)) {
            state = PYBA_STATE_FAILED;
        }

        if (_state != state) {
            ::PostMessageW(_hWnd, WM_PYBA_CHANGE_STATE, 0, state);
        }
    }

    //
    // GoToPage
    //
    void GoToPage(__in PAGE page) {
        _installPage = page;
        ::PostMessageW(_hWnd, WM_PYBA_CHANGE_STATE, 0, _state);
    }

    void DeterminePageId(__in PYBA_STATE state, __out DWORD* pdwPageId) {
        LONGLONG simple;

        if (BOOTSTRAPPER_DISPLAY_PASSIVE == _command.display) {
            switch (state) {
            case PYBA_STATE_INITIALIZED:
                *pdwPageId = BOOTSTRAPPER_ACTION_HELP == _command.action
                    ? _pageIds[PAGE_HELP]
                    : _pageIds[PAGE_LOADING];
                break;

            case PYBA_STATE_HELP:
                *pdwPageId = _pageIds[PAGE_HELP];
                break;

            case PYBA_STATE_DETECTING:
                *pdwPageId = _pageIds[PAGE_LOADING]
                    ? _pageIds[PAGE_LOADING]
                    : _pageIds[PAGE_PROGRESS_PASSIVE]
                        ? _pageIds[PAGE_PROGRESS_PASSIVE]
                        : _pageIds[PAGE_PROGRESS];
                break;

            case PYBA_STATE_DETECTED: __fallthrough;
            case PYBA_STATE_PLANNING: __fallthrough;
            case PYBA_STATE_PLANNED: __fallthrough;
            case PYBA_STATE_APPLYING: __fallthrough;
            case PYBA_STATE_CACHING: __fallthrough;
            case PYBA_STATE_CACHED: __fallthrough;
            case PYBA_STATE_EXECUTING: __fallthrough;
            case PYBA_STATE_EXECUTED:
                *pdwPageId = _pageIds[PAGE_PROGRESS_PASSIVE]
                    ? _pageIds[PAGE_PROGRESS_PASSIVE]
                    : _pageIds[PAGE_PROGRESS];
                break;

            default:
                *pdwPageId = 0;
                break;
            }
        } else if (BOOTSTRAPPER_DISPLAY_FULL == _command.display) {
            switch (state) {
            case PYBA_STATE_INITIALIZING:
                *pdwPageId = 0;
                break;

            case PYBA_STATE_INITIALIZED:
                *pdwPageId = BOOTSTRAPPER_ACTION_HELP == _command.action
                    ? _pageIds[PAGE_HELP]
                    : _pageIds[PAGE_LOADING];
                break;

            case PYBA_STATE_HELP:
                *pdwPageId = _pageIds[PAGE_HELP];
                break;

            case PYBA_STATE_DETECTING:
                *pdwPageId = _pageIds[PAGE_LOADING];
                break;

            case PYBA_STATE_DETECTED:
                if (_installPage == PAGE_LOADING) {
                    switch (_command.action) {
                    case BOOTSTRAPPER_ACTION_INSTALL:
                        if (_upgrading) {
                            _installPage = PAGE_UPGRADE;
                        } else if (SUCCEEDED(BalGetNumericVariable(L"SimpleInstall", &simple)) && simple) {
                            _installPage = PAGE_SIMPLE_INSTALL;
                        } else {
                            _installPage = PAGE_INSTALL;
                        }
                        break;

                    case BOOTSTRAPPER_ACTION_MODIFY: __fallthrough;
                    case BOOTSTRAPPER_ACTION_REPAIR: __fallthrough;
                    case BOOTSTRAPPER_ACTION_UNINSTALL:
                        _installPage = PAGE_MODIFY;
                        break;
                    }
                }
                *pdwPageId = _pageIds[_installPage];
                break;

            case PYBA_STATE_PLANNING: __fallthrough;
            case PYBA_STATE_PLANNED: __fallthrough;
            case PYBA_STATE_APPLYING: __fallthrough;
            case PYBA_STATE_CACHING: __fallthrough;
            case PYBA_STATE_CACHED: __fallthrough;
            case PYBA_STATE_EXECUTING: __fallthrough;
            case PYBA_STATE_EXECUTED:
                *pdwPageId = _pageIds[PAGE_PROGRESS];
                break;

            case PYBA_STATE_APPLIED:
                *pdwPageId = _pageIds[PAGE_SUCCESS];
                break;

            case PYBA_STATE_FAILED:
                *pdwPageId = _pageIds[PAGE_FAILURE];
                break;
            }
        }
    }

    BOOL WillElevate() {
        static BAL_CONDITION WILL_ELEVATE_CONDITION = {
            L"not WixBundleElevated and ("
                /*Elevate when installing for all users*/
                L"InstallAllUsers or "
                /*Elevate when installing the launcher for all users and it was not detected*/
                L"(Include_launcher and InstallLauncherAllUsers and not DetectedLauncher)"
            L")",
            L""
        };
        BOOL result;

        return SUCCEEDED(BalConditionEvaluate(&WILL_ELEVATE_CONDITION, _engine, &result, nullptr)) && result;
    }

    BOOL IsCrtInstalled() {
        if (_crtInstalledToken > 0) {
            return TRUE;
        } else if (_crtInstalledToken == 0) {
            return FALSE;
        }
        
        // Check whether at least CRT v10.0.10137.0 is available.
        // It should only be installed as a Windows Update package, which means
        // we don't need to worry about 32-bit/64-bit.
        LPCWSTR crtFile = L"ucrtbase.dll";

        DWORD cbVer = GetFileVersionInfoSizeW(crtFile, nullptr);
        if (!cbVer) {
            _crtInstalledToken = 0;
            return FALSE;
        }

        void *pData = malloc(cbVer);
        if (!pData) {
            _crtInstalledToken = 0;
            return FALSE;
        }

        if (!GetFileVersionInfoW(crtFile, 0, cbVer, pData)) {
            free(pData);
            _crtInstalledToken = 0;
            return FALSE;
        }

        VS_FIXEDFILEINFO *ffi;
        UINT cb;
        BOOL result = FALSE;

        if (VerQueryValueW(pData, L"\\", (LPVOID*)&ffi, &cb) &&
            ffi->dwFileVersionMS == 0x000A0000 && ffi->dwFileVersionLS >= 0x27990000) {
            result = TRUE;
        }
        
        free(pData);
        _crtInstalledToken = result ? 1 : 0;
        return result;
    }

    BOOL QueryElevateForCrtInstall() {
        // Called to prompt the user that even though they think they won't need
        // to elevate, they actually will because of the CRT install.
        if (IsCrtInstalled()) {
            // CRT is already installed - no need to prompt
            return TRUE;
        }
        
        LONGLONG elevated;
        HRESULT hr = BalGetNumericVariable(L"WixBundleElevated", &elevated);
        if (SUCCEEDED(hr) && elevated) {
            // Already elevated - no need to prompt
            return TRUE;
        }

        LOC_STRING *locStr;
        hr = LocGetString(_wixLoc, L"#(loc.ElevateForCRTInstall)", &locStr);
        if (FAILED(hr)) {
            BalLogError(hr, "Failed to get ElevateForCRTInstall string");
            return FALSE;
        }
        return ::MessageBoxW(_hWnd, locStr->wzText, _theme->sczCaption, MB_YESNO) != IDNO;
    }

    HRESULT EvaluateConditions() {
        HRESULT hr = S_OK;
        BOOL result = FALSE;

        for (DWORD i = 0; i < _conditions.cConditions; ++i) {
            BAL_CONDITION* pCondition = _conditions.rgConditions + i;

            hr = BalConditionEvaluate(pCondition, _engine, &result, &_failedMessage);
            BalExitOnFailure(hr, "Failed to evaluate condition.");

            if (!result) {
                // Hope they didn't have hidden variables in their message, because it's going in the log in plaintext.
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "%ls", _failedMessage);

                hr = E_WIXSTDBA_CONDITION_FAILED;
                // todo: remove in WiX v4, in case people are relying on v3.x logging behavior
                BalExitOnFailure1(hr, "Bundle condition evaluated to false: %ls", pCondition->sczCondition);
            }
        }

        ReleaseNullStrSecure(_failedMessage);

    LExit:
        return hr;
    }


    void SetTaskbarButtonProgress(__in DWORD dwOverallPercentage) {
        HRESULT hr = S_OK;

        if (_taskbarButtonOK) {
            hr = _taskbarList->SetProgressValue(_hWnd, dwOverallPercentage, 100UL);
            BalExitOnFailure1(hr, "Failed to set taskbar button progress to: %d%%.", dwOverallPercentage);
        }

    LExit:
        return;
    }


    void SetTaskbarButtonState(__in TBPFLAG tbpFlags) {
        HRESULT hr = S_OK;

        if (_taskbarButtonOK) {
            hr = _taskbarList->SetProgressState(_hWnd, tbpFlags);
            BalExitOnFailure1(hr, "Failed to set taskbar button state.", tbpFlags);
        }

    LExit:
        return;
    }


    void SetProgressState(__in HRESULT hrStatus) {
        TBPFLAG flag = TBPF_NORMAL;

        if (IsCanceled() || HRESULT_FROM_WIN32(ERROR_INSTALL_USEREXIT) == hrStatus) {
            flag = TBPF_PAUSED;
        } else if (IsRollingBack() || FAILED(hrStatus)) {
            flag = TBPF_ERROR;
        }

        SetTaskbarButtonState(flag);
    }


    HRESULT LoadBootstrapperBAFunctions() {
        HRESULT hr = S_OK;
        LPWSTR sczBafPath = nullptr;

        hr = PathRelativeToModule(&sczBafPath, L"bafunctions.dll", _hModule);
        BalExitOnFailure(hr, "Failed to get path to BA function DLL.");

#ifdef DEBUG
        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "PYBA: LoadBootstrapperBAFunctions() - BA function DLL %ls", sczBafPath);
#endif

        _hBAFModule = ::LoadLibraryW(sczBafPath);
        if (_hBAFModule) {
            auto pfnBAFunctionCreate = reinterpret_cast<PFN_BOOTSTRAPPER_BA_FUNCTION_CREATE>(::GetProcAddress(_hBAFModule, "CreateBootstrapperBAFunction"));
            BalExitOnNullWithLastError1(pfnBAFunctionCreate, hr, "Failed to get CreateBootstrapperBAFunction entry-point from: %ls", sczBafPath);

            hr = pfnBAFunctionCreate(_engine, _hBAFModule, &_baFunction);
            BalExitOnFailure(hr, "Failed to create BA function.");
        }
#ifdef DEBUG
        else {
            BalLogError(HRESULT_FROM_WIN32(::GetLastError()), "PYBA: LoadBootstrapperBAFunctions() - Failed to load DLL %ls", sczBafPath);
        }
#endif

    LExit:
        if (_hBAFModule && !_baFunction) {
            ::FreeLibrary(_hBAFModule);
            _hBAFModule = nullptr;
        }
        ReleaseStr(sczBafPath);

        return hr;
    }

    BOOL IsCheckable(THEME_CONTROL* pControl) {
        if (!pControl->sczName || !pControl->sczName[0]) {
            return FALSE;
        }

        if (pControl->type == THEME_CONTROL_TYPE_CHECKBOX) {
            return TRUE;
        }

        if (pControl->type == THEME_CONTROL_TYPE_BUTTON) {
            if ((pControl->dwStyle & BS_TYPEMASK) == BS_AUTORADIOBUTTON) {
                return TRUE;
            }
        }

        return FALSE;
    }

    void SavePageSettings() {
        DWORD pageId = 0;
        THEME_PAGE* pPage = nullptr;

        DeterminePageId(_state, &pageId);
        pPage = ThemeGetPage(_theme, pageId);
        if (!pPage) {
            return;
        }

        for (DWORD i = 0; i < pPage->cControlIndices; ++i) {
            // Loop through all the checkable controls and set a Burn variable
            // with that name to true or false.
            THEME_CONTROL* pControl = _theme->rgControls + pPage->rgdwControlIndices[i];
            if (IsCheckable(pControl) && ThemeControlEnabled(_theme, pControl->wId)) {
                BOOL checked = ThemeIsControlChecked(_theme, pControl->wId);
                _engine->SetVariableNumeric(pControl->sczName, checked ? 1 : 0);
            }

            // Loop through all the editbox controls with names and set a
            // Burn variable with that name to the contents.
            if (THEME_CONTROL_TYPE_EDITBOX == pControl->type && pControl->sczName && *pControl->sczName) {
                LPWSTR sczValue = nullptr;
                ThemeGetTextControl(_theme, pControl->wId, &sczValue);
                _engine->SetVariableString(pControl->sczName, sczValue);
            }
        }
    }

    static bool IsTargetPlatformx64(__in IBootstrapperEngine* pEngine) {
        WCHAR platform[8];
        DWORD platformLen = 8;

        if (FAILED(pEngine->GetVariableString(L"TargetPlatform", platform, &platformLen))) {
            return S_FALSE;
        }

        return ::CompareStringW(LOCALE_NEUTRAL, 0, platform, -1, L"x64", -1) == CSTR_EQUAL;
    }

    static HRESULT LoadOptionalFeatureStatesFromKey(
        __in IBootstrapperEngine* pEngine,
        __in HKEY hkHive,
        __in LPCWSTR subkey
    ) {
        HKEY hKey;
        LRESULT res;

        if (IsTargetPlatformx64(pEngine)) {
            res = RegOpenKeyExW(hkHive, subkey, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
        } else {
            res = RegOpenKeyExW(hkHive, subkey, 0, KEY_READ | KEY_WOW64_32KEY, &hKey);
        }
        if (res == ERROR_FILE_NOT_FOUND) {
            return S_FALSE;
        }
        if (res != ERROR_SUCCESS) {
            return HRESULT_FROM_WIN32(res);
        }

        for (auto p = OPTIONAL_FEATURES; p->regName; ++p) {
            res = RegQueryValueExW(hKey, p->regName, nullptr, nullptr, nullptr, nullptr);
            if (res == ERROR_FILE_NOT_FOUND) {
                pEngine->SetVariableNumeric(p->variableName, 0);
            } else if (res == ERROR_SUCCESS) {
                pEngine->SetVariableNumeric(p->variableName, 1);
            } else {
                RegCloseKey(hKey);
                return HRESULT_FROM_WIN32(res);
            }
        }

        RegCloseKey(hKey);
        return S_OK;
    }

    static HRESULT LoadTargetDirFromKey(
        __in IBootstrapperEngine* pEngine,
        __in HKEY hkHive,
        __in LPCWSTR subkey
    ) {
        HKEY hKey;
        LRESULT res;
        DWORD dataType;
        BYTE buffer[1024];
        DWORD bufferLen = sizeof(buffer);

        if (IsTargetPlatformx64(pEngine)) {
            res = RegOpenKeyExW(hkHive, subkey, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
        } else {
            res = RegOpenKeyExW(hkHive, subkey, 0, KEY_READ | KEY_WOW64_32KEY, &hKey);
        }
        if (res == ERROR_FILE_NOT_FOUND) {
            return S_FALSE;
        }
        if (res != ERROR_SUCCESS) {
            return HRESULT_FROM_WIN32(res);
        }

        res = RegQueryValueExW(hKey, nullptr, nullptr, &dataType, buffer, &bufferLen);
        if (res == ERROR_SUCCESS && dataType == REG_SZ && bufferLen < sizeof(buffer)) {
            pEngine->SetVariableString(L"TargetDir", reinterpret_cast<wchar_t*>(buffer));
        }
        RegCloseKey(hKey);
        return HRESULT_FROM_WIN32(res);
    }

    static HRESULT LoadAssociateFilesStateFromKey(
        __in IBootstrapperEngine* pEngine,
        __in HKEY hkHive
    ) {
        const LPCWSTR subkey = L"Software\\Python\\PyLauncher";
        HKEY hKey;
        LRESULT res;
        HRESULT hr;

        res = RegOpenKeyExW(hkHive, subkey, 0, KEY_READ | KEY_WOW64_32KEY, &hKey);

        if (res == ERROR_FILE_NOT_FOUND) {
            return S_FALSE;
        }
        if (res != ERROR_SUCCESS) {
            return HRESULT_FROM_WIN32(res);
        }

        res = RegQueryValueExW(hKey, L"AssociateFiles", nullptr, nullptr, nullptr, nullptr);
        if (res == ERROR_FILE_NOT_FOUND) {
            hr = S_FALSE;
        } else if (res == ERROR_SUCCESS) {
            hr = S_OK;
        } else {
            hr = HRESULT_FROM_WIN32(res);
        }

        RegCloseKey(hKey);
        return hr;
    }

    static void LoadOptionalFeatureStates(__in IBootstrapperEngine* pEngine) {
        WCHAR subkeyFmt[256];
        WCHAR subkey[256];
        DWORD subkeyLen;
        HRESULT hr;
        HKEY hkHive;

        // The launcher installation is separate from the Python install, so we
        // check its state later. For now, assume we don't want the launcher or
        // file associations, and if they have already been installed then
        // loading the state will reactivate these settings.
        pEngine->SetVariableNumeric(L"Include_launcher", 0);
        pEngine->SetVariableNumeric(L"AssociateFiles", 0);

        // Get the registry key from the bundle, to save having to duplicate it
        // in multiple places.
        subkeyLen = sizeof(subkeyFmt) / sizeof(subkeyFmt[0]);
        hr = pEngine->GetVariableString(L"OptionalFeaturesRegistryKey", subkeyFmt, &subkeyLen);
        BalExitOnFailure(hr, "Failed to locate registry key");
        subkeyLen = sizeof(subkey) / sizeof(subkey[0]);
        hr = pEngine->FormatString(subkeyFmt, subkey, &subkeyLen);
        BalExitOnFailure1(hr, "Failed to format %ls", subkeyFmt);

        // Check the current user's registry for existing features
        hkHive = HKEY_CURRENT_USER;
        hr = LoadOptionalFeatureStatesFromKey(pEngine, hkHive, subkey);
        BalExitOnFailure1(hr, "Failed to read from HKCU\\%ls", subkey);
        if (hr == S_FALSE) {
            // Now check the local machine registry
            hkHive = HKEY_LOCAL_MACHINE;
            hr = LoadOptionalFeatureStatesFromKey(pEngine, hkHive, subkey);
            BalExitOnFailure1(hr, "Failed to read from HKLM\\%ls", subkey);
            if (hr == S_OK) {
                // Found a system-wide install, so enable these settings.
                pEngine->SetVariableNumeric(L"InstallAllUsers", 1);
                pEngine->SetVariableNumeric(L"CompileAll", 1);
            }
        }

        if (hr == S_OK) {
            // Cannot change InstallAllUsersState when upgrading. While there's
            // no good reason to not allow installing a per-user and an all-user
            // version simultaneously, Burn can't handle the state management
            // and will need to uninstall the old one. 
            pEngine->SetVariableString(L"InstallAllUsersState", L"disable");

            // Get the previous install directory. This can be changed by the
            // user.
            subkeyLen = sizeof(subkeyFmt) / sizeof(subkeyFmt[0]);
            hr = pEngine->GetVariableString(L"TargetDirRegistryKey", subkeyFmt, &subkeyLen);
            BalExitOnFailure(hr, "Failed to locate registry key");
            subkeyLen = sizeof(subkey) / sizeof(subkey[0]);
            hr = pEngine->FormatString(subkeyFmt, subkey, &subkeyLen);
            BalExitOnFailure1(hr, "Failed to format %ls", subkeyFmt);
            LoadTargetDirFromKey(pEngine, hkHive, subkey);
        }

    LExit:
        return;
    }

    HRESULT EnsureTargetDir() {
        LONGLONG installAllUsers;
        LPWSTR targetDir = nullptr, defaultDir = nullptr;
        HRESULT hr = BalGetStringVariable(L"TargetDir", &targetDir);
        if (FAILED(hr) || !targetDir || !targetDir[0]) {
            ReleaseStr(targetDir);
            targetDir = nullptr;

            hr = BalGetNumericVariable(L"InstallAllUsers", &installAllUsers);
            ExitOnFailure(hr, L"Failed to get install scope");

            hr = BalGetStringVariable(
                installAllUsers ? L"DefaultAllUsersTargetDir" : L"DefaultJustForMeTargetDir",
                &defaultDir
            );
            BalExitOnFailure(hr, "Failed to get the default install directory");

            if (!defaultDir || !defaultDir[0]) {
                BalLogError(E_INVALIDARG, "Default install directory is blank");
            }

            hr = BalFormatString(defaultDir, &targetDir);
            BalExitOnFailure1(hr, "Failed to format '%ls'", defaultDir);

            hr = _engine->SetVariableString(L"TargetDir", targetDir);
            BalExitOnFailure(hr, "Failed to set install target directory");
        }
    LExit:
        ReleaseStr(defaultDir);
        ReleaseStr(targetDir);
        return hr;
    }

    void ValidateOperatingSystem() {
        LOC_STRING *pLocString = nullptr;
        
        if (IsWindowsServer()) {
            if (IsWindowsVersionOrGreater(6, 1, 1)) {
                BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Target OS is Windows Server 2008 R2 or later");
                return;
            } else if (IsWindowsVersionOrGreater(6, 1, 0)) {
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Detected Windows Server 2008 R2");
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Service Pack 1 is required to continue installation");
                LocGetString(_wixLoc, L"#(loc.FailureWS2K8R2MissingSP1)", &pLocString);
            } else if (IsWindowsVersionOrGreater(6, 0, 2)) {
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Target OS is Windows Server 2008 SP2 or later");
                return;
            } else if (IsWindowsVersionOrGreater(6, 0, 0)) {
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Detected Windows Server 2008");
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Service Pack 2 is required to continue installation");
                LocGetString(_wixLoc, L"#(loc.FailureWS2K8MissingSP2)", &pLocString);
            } else {
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Detected Windows Server 2003 or earlier");
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Windows Server 2008 SP2 or later is required to continue installation");
                LocGetString(_wixLoc, L"#(loc.FailureWS2K3OrEarlier)", &pLocString);
            }
        } else {
            if (IsWindows7SP1OrGreater()) {
                BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Target OS is Windows 7 SP1 or later");
                return;
            } else if (IsWindows7OrGreater()) {
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Detected Windows 7 RTM");
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Service Pack 1 is required to continue installation");
                LocGetString(_wixLoc, L"#(loc.FailureWin7MissingSP1)", &pLocString);
            } else if (IsWindowsVistaSP2OrGreater()) {
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Target OS is Windows Vista SP2");
                return;
            } else if (IsWindowsVistaOrGreater()) {
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Detected Windows Vista RTM or SP1");
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Service Pack 2 is required to continue installation");
                LocGetString(_wixLoc, L"#(loc.FailureVistaMissingSP2)", &pLocString);
            } else { 
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Detected Windows XP or earlier");
                BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "Windows Vista SP2 or later is required to continue installation");
                LocGetString(_wixLoc, L"#(loc.FailureXPOrEarlier)", &pLocString);
            }
        }

        if (pLocString && pLocString->wzText) {
            BalFormatString(pLocString->wzText, &_failedMessage);
        }
        
        _hrFinal = E_WIXSTDBA_CONDITION_FAILED;
    }

public:
    //
    // Constructor - initialize member variables.
    //
    PythonBootstrapperApplication(
        __in HMODULE hModule,
        __in BOOL fPrereq,
        __in HRESULT hrHostInitialization,
        __in IBootstrapperEngine* pEngine,
        __in const BOOTSTRAPPER_COMMAND* pCommand
    ) : CBalBaseBootstrapperApplication(pEngine, pCommand, 3, 3000) {
        _hModule = hModule;
        memcpy_s(&_command, sizeof(_command), pCommand, sizeof(BOOTSTRAPPER_COMMAND));

        LONGLONG llInstalled = 0;
        HRESULT hr = BalGetNumericVariable(L"WixBundleInstalled", &llInstalled);
        if (SUCCEEDED(hr) && BOOTSTRAPPER_RESUME_TYPE_REBOOT != _command.resumeType && 0 < llInstalled && BOOTSTRAPPER_ACTION_INSTALL == _command.action) {
            _command.action = BOOTSTRAPPER_ACTION_MODIFY;
        } else if (0 == llInstalled && (BOOTSTRAPPER_ACTION_MODIFY == _command.action || BOOTSTRAPPER_ACTION_REPAIR == _command.action)) {
            _command.action = BOOTSTRAPPER_ACTION_INSTALL;
        }

        _plannedAction = BOOTSTRAPPER_ACTION_UNKNOWN;


        // When resuming from restart doing some install-like operation, try to find the package that forced the
        // restart. We'll use this information during planning.
        _nextPackageAfterRestart = nullptr;

        if (BOOTSTRAPPER_RESUME_TYPE_REBOOT == _command.resumeType && BOOTSTRAPPER_ACTION_UNINSTALL < _command.action) {
            // Ensure the forced restart package variable is null when it is an empty string.
            HRESULT hr = BalGetStringVariable(L"WixBundleForcedRestartPackage", &_nextPackageAfterRestart);
            if (FAILED(hr) || !_nextPackageAfterRestart || !*_nextPackageAfterRestart) {
                ReleaseNullStr(_nextPackageAfterRestart);
            }
        }

        _crtInstalledToken = -1;
        pEngine->SetVariableNumeric(L"CRTInstalled", IsCrtInstalled() ? 1 : 0);

        _wixLoc = nullptr;
        memset(&_bundle, 0, sizeof(_bundle));
        memset(&_conditions, 0, sizeof(_conditions));
        _confirmCloseMessage = nullptr;
        _failedMessage = nullptr;

        _language = nullptr;
        _theme = nullptr;
        memset(_pageIds, 0, sizeof(_pageIds));
        _hUiThread = nullptr;
        _registered = FALSE;
        _hWnd = nullptr;

        _state = PYBA_STATE_INITIALIZING;
        _visiblePageId = 0;
        _installPage = PAGE_LOADING;
        _hrFinal = hrHostInitialization;

        _downgradingOtherVersion = FALSE;
        _restartResult = BOOTSTRAPPER_APPLY_RESTART_NONE;
        _restartRequired = FALSE;
        _allowRestart = FALSE;

        _suppressDowngradeFailure = FALSE;
        _suppressRepair = FALSE;
        _modifying = FALSE;
        _upgrading = FALSE;

        _overridableVariables = nullptr;
        _taskbarList = nullptr;
        _taskbarButtonCreatedMessage = UINT_MAX;
        _taskbarButtonOK = FALSE;
        _showingInternalUIThisPackage = FALSE;

        _suppressPaint = FALSE;

        pEngine->AddRef();
        _engine = pEngine;

        _hBAFModule = nullptr;
        _baFunction = nullptr;
    }


    //
    // Destructor - release member variables.
    //
    ~PythonBootstrapperApplication() {
        AssertSz(!::IsWindow(_hWnd), "Window should have been destroyed before destructor.");
        AssertSz(!_theme, "Theme should have been released before destructor.");

        ReleaseObject(_taskbarList);
        ReleaseDict(_overridableVariables);
        ReleaseStr(_failedMessage);
        ReleaseStr(_confirmCloseMessage);
        BalConditionsUninitialize(&_conditions);
        BalInfoUninitialize(&_bundle);
        LocFree(_wixLoc);

        ReleaseStr(_language);
        ReleaseStr(_nextPackageAfterRestart);
        ReleaseNullObject(_engine);

        if (_hBAFModule) {
            ::FreeLibrary(_hBAFModule);
            _hBAFModule = nullptr;
        }
    }

private:
    HMODULE _hModule;
    BOOTSTRAPPER_COMMAND _command;
    IBootstrapperEngine* _engine;
    BOOTSTRAPPER_ACTION _plannedAction;

    LPWSTR _nextPackageAfterRestart;

    WIX_LOCALIZATION* _wixLoc;
    BAL_INFO_BUNDLE _bundle;
    BAL_CONDITIONS _conditions;
    LPWSTR _failedMessage;
    LPWSTR _confirmCloseMessage;

    LPWSTR _language;
    THEME* _theme;
    DWORD _pageIds[countof(PAGE_NAMES)];
    HANDLE _hUiThread;
    BOOL _registered;
    HWND _hWnd;

    PYBA_STATE _state;
    HRESULT _hrFinal;
    DWORD _visiblePageId;
    PAGE _installPage;

    BOOL _startedExecution;
    DWORD _calculatedCacheProgress;
    DWORD _calculatedExecuteProgress;

    BOOL _downgradingOtherVersion;
    BOOTSTRAPPER_APPLY_RESTART _restartResult;
    BOOL _restartRequired;
    BOOL _allowRestart;

    BOOL _suppressDowngradeFailure;
    BOOL _suppressRepair;
    BOOL _modifying;
    BOOL _upgrading;

    int _crtInstalledToken;

    STRINGDICT_HANDLE _overridableVariables;

    ITaskbarList3* _taskbarList;
    UINT _taskbarButtonCreatedMessage;
    BOOL _taskbarButtonOK;
    BOOL _showingInternalUIThisPackage;

    BOOL _suppressPaint;

    HMODULE _hBAFModule;
    IBootstrapperBAFunction* _baFunction;
};

//
// CreateBootstrapperApplication - creates a new IBootstrapperApplication object.
//
HRESULT CreateBootstrapperApplication(
    __in HMODULE hModule,
    __in BOOL fPrereq,
    __in HRESULT hrHostInitialization,
    __in IBootstrapperEngine* pEngine,
    __in const BOOTSTRAPPER_COMMAND* pCommand,
    __out IBootstrapperApplication** ppApplication
    ) {
    HRESULT hr = S_OK;

    if (fPrereq) {
        hr = E_INVALIDARG;
        ExitWithLastError(hr, "Failed to create UI thread.");
    }

    PythonBootstrapperApplication* pApplication = nullptr;

    pApplication = new PythonBootstrapperApplication(hModule, fPrereq, hrHostInitialization, pEngine, pCommand);
    ExitOnNull(pApplication, hr, E_OUTOFMEMORY, "Failed to create new standard bootstrapper application object.");

    *ppApplication = pApplication;
    pApplication = nullptr;

LExit:
    ReleaseObject(pApplication);
    return hr;
}
