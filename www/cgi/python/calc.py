#!/usr/bin/env python3
import sys
import os

def parse_post_data():
    """Read and parse POST data from stdin"""
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    if content_length == 0:
        return {}
    
    post_data = sys.stdin.read(content_length)
    form = {}
    if post_data:
        for pair in post_data.split('&'):
            if '=' in pair:
                key, value = pair.split('=', 1)
                form[key] = value
    return form

def calculate(num1, num2, operation):
    """Perform calculation and return result with symbol"""
    n1 = float(num1)
    n2 = float(num2)
    
    operations = {
        'add': {'result': n1 + n2, 'symbol': '+'},
        'subtract': {'result': n1 - n2, 'symbol': '-'},
        'multiply': {'result': n1 * n2, 'symbol': '×'},
        'divide': {'result': n1 / n2 if n2 != 0 else None, 'symbol': '÷'},
        'power': {'result': n1 ** n2, 'symbol': '^'},
        'modulo': {'result': n1 % n2 if n2 != 0 else None, 'symbol': '%'}
    }
    
    if operation not in operations:
        return {'error': 'Invalid operation', 'symbol': None, 'result': None}
    
    op = operations[operation]
    
    if op['result'] is None:
        return {'error': 'Division by zero', 'symbol': op['symbol'], 'result': None}
    
    return {'result': op['result'], 'symbol': op['symbol'], 'error': None}

# Send HTTP headers
print("Content-Type: text/html\r")
print("\r")

# Parse POST data
data = parse_post_data()

# Extract parameters
num1 = data.get('num1', '')
num2 = data.get('num2', '')
operation = data.get('operation', '')

# Validate input
if not num1 or not num2 or not operation:
    print("<h3>❌ Error</h3>")
    print("<p>Missing parameters. Please provide num1, num2, and operation.</p>")
    sys.exit(0)

# Calculate
try:
    calc = calculate(num1, num2, operation)
    
    if calc['error']:
        print("<h3>❌ Error</h3>")
        print(f"<p>{calc['error']}</p>")
    else:
        print("<h3>✅ Result</h3>")
        print(f"<div class='result-value'>{num1} {calc['symbol']} {num2} = {calc['result']}</div>")
        print(f"<p><strong>Operation:</strong> {operation.capitalize()}</p>")
        print("<p><strong>Language:</strong> Python 🐍</p>")
except Exception as e:
    print("<h3>❌ Error</h3>")
    print(f"<p>Calculation error: {str(e)}</p>")
