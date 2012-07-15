$env:VIRTUAL_ENV="__VENV_DIR__"

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

# Set the prompt to include the env name
copy-item function:prompt function:_OLD_VIRTUAL_PROMPT
function prompt {
    Write-Host -NoNewline -ForegroundColor Green '[__VENV_NAME__]'
    _OLD_VIRTUAL_PROMPT
}

# Clear PYTHONHOME
if (Test-Path env:PYTHONHOME) {
    copy-item env:PYTHONHOME env:_OLD_VIRTUAL_PYTHONHOME
    remove-item env:PYTHONHOME
}

# Add the venv to the PATH
copy-item env:PATH env:_OLD_VIRTUAL_PATH
$env:PATH = "$env:VIRTUAL_ENV\__VENV_BIN_NAME__;$env:PATH"
