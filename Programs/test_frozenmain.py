import sys
import _testinternalcapi

print("Frozen Hello World")
print("sys.argv", sys.argv)
config = _testinternalcapi.get_configs()['config']
print(f"config program_name: {config['program_name']}")
print(f"config executable: {config['executable']}")
print(f"config use_environment: {config['use_environment']}")
