import os
import sys

try:
    import winreg
except ImportError:
    sys.exit(0)

found = False
for d in os.environ['PATH'].split(os.pathsep):
    print('Checking', d)
    try:
        with os.scandir(d) as sd:
            for de in sd:
                if de.name.lower().endswith('sqlite3.dll') and de.is_file():
                    print('Found [_]sqlite3.dll at', de.path)
                    found = True
    except OSError as e:
        print(e)

try:
    value = winreg.QueryValueEx(
        winreg.OpenKeyEx(
            winreg.HKEY_LOCAL_MACHINE,
            'SYSTEM\\CurrentControlSet\\Control\\Session Manager'
        ),
        'CWDIllegalInDllSearch'
    )
except FileNotFoundError:
    print('CWDIllegalInDllSearch not set')
else:
    print('CWDIllegalInDllSearch set to', value)
    found = True

base_key = winreg.OpenKeyEx(
    winreg.HKEY_LOCAL_MACHINE,
    'Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options'
)
for i in range(winreg.QueryInfoKey(base_key)[0]):
    sub_key_name = winreg.EnumKey(base_key, i)
    print(sub_key_name)
    if 'py' not in sub_key_name:
        continue
    sub_key = winreg.OpenKeyEx(base_key, sub_key_name)
    for j in range(winreg.QueryInfoKey(sub_key)[1]):
        name, value, type = winreg.EnumValue(sub_key, j)
        if 'CWD' in name:
            print(name, value, type)
            found = True

if found:
    raise FileExistsError('Found it')
