# Revert to original values
if (Test-Path function:_OLD_VIRTUAL_PROMPT) {
    copy-item function:_OLD_VIRTUAL_PROMPT function:prompt
    remove-item function:_OLD_VIRTUAL_PROMPT
}

if (Test-Path env:_OLD_VIRTUAL_PYTHONHOME) {
    copy-item env:_OLD_VIRTUAL_PYTHONHOME env:PYTHONHOME
    remove-item env:_OLD_VIRTUAL_PYTHONHOME
}

if (Test-Path env:_OLD_VIRTUAL_PATH) {
    copy-item env:_OLD_VIRTUAL_PATH env:PATH
    remove-item env:_OLD_VIRTUAL_PATH
}

if (Test-Path env:VIRTUAL_ENV) {
    remove-item env:VIRTUAL_ENV
}
