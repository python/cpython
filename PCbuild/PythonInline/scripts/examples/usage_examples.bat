# Example usage scripts for Python Inline Runner

# Example 1: Simple Hello World
python_inline -c "print('Hello, World!')"

# Example 2: Math calculations
python_inline -c "import math; print(f'Pi is approximately {math.pi:.6f}')"

# Example 3: Working with sys.argv
python_inline -c "import sys; print(f'Arguments: {sys.argv[1:]}')" arg1 arg2 arg3

# Example 4: JSON processing
python_inline --script="import json; data={'name': 'Python', 'version': '3.x'}; print(json.dumps(data, indent=2))"

# Example 5: File operations
python_inline -c "
import os
import sys
print(f'Current directory: {os.getcwd()}')
print(f'Python executable: {sys.executable}')
print(f'Platform: {sys.platform}')
"

# Example 6: Interactive mode
python_inline -i -c "x = 42; print(f'x = {x}')"

# Example 7: Running from file
python_inline --file=example_script.py

# Example 8: Web request (if requests is available)
python_inline -c "
try:
    import urllib.request
    with urllib.request.urlopen('https://httpbin.org/json') as response:
        import json
        data = json.loads(response.read().decode())
        print(f'Response: {data}')
except Exception as e:
    print(f'Error: {e}')
"

# Example 9: Date and time
python_inline -c "
from datetime import datetime
now = datetime.now()
print(f'Current time: {now.strftime(\"%Y-%m-%d %H:%M:%S\")}')
"

# Example 10: List comprehension and data processing
python_inline -c "
numbers = range(1, 11)
squares = [x**2 for x in numbers if x % 2 == 0]
print(f'Even squares: {squares}')
"