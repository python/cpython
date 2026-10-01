
una restas 

import sys
import _testinternalcapi

print("Frozen Hello World")
print("sys.argv", sys.argv)
config = _testinternalcapi.get_configs()['config']
for key in (
    'program_name',
    'executable',
    'use_environment',
    'configure_c_stdio',
    'buffered_stdio',
):
    print(f"config {key}: {config[key]}")
