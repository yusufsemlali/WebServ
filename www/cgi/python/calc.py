#!/usr/bin/env python3
import os
import sys
import urllib.parse

def parse_post_data():
    """Parse POST data from stdin"""
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    if content_length > 0:
        post_data = sys.stdin.read(content_length)
        return urllib.parse.parse_qs(post_data)
    return {}

def calculate(num1, num2, operation):
    """Perform the calculation based on operation"""
    try:
        n1 = float(num1)
        n2 = float(num2)
        
        operations = {
            'add': (n1 + n2, '+'),
            'subtract': (n1 - n2, '-'),
            'multiply': (n1 * n2, '×'),
            'divide': (n1 / n2 if n2 != 0 else None, '÷'),
            'power': (n1 ** n2, '^'),
            'modulo': (n1 % n2 if n2 != 0 else None, '%')
        }
        
        if operation not in operations:
            return None, None, "Invalid operation"
        
        result, symbol = operations[operation]
        
        if result is None:
            return None, symbol, "Division by zero"
        
        return result, symbol, None
        
    except ValueError:
        return None, None, "Invalid number format"
    except Exception as e:
        return None, None, str(e)

def main():
    # Parse POST data
    data = parse_post_data()
    
    # Extract parameters
    num1 = data.get('num1', [''])[0]
    num2 = data.get('num2', [''])[0]
    operation = data.get('operation', [''])[0]
    
    # Send HTTP headers
    print("Content-Type: text/html\r")
    print("\r")
    
    # Validate input
    if not num1 or not num2 or not operation:
        print("<h3>❌ Error</h3>")
        print("<p>Missing parameters. Please provide num1, num2, and operation.</p>")
        return
    
    # Calculate
    result, symbol, error = calculate(num1, num2, operation)
    
    if error:
        print("<h3>❌ Error</h3>")
        print(f"<p>{error}</p>")
    else:
        print("<h3>✅ Result</h3>")
        print(f"<div class='result-value'>{num1} {symbol} {num2} = {result}</div>")
        print(f"<p><strong>Operation:</strong> {operation.capitalize()}</p>")
        print(f"<p><strong>Language:</strong> Python 3 🐍</p>")

if __name__ == "__main__":
    main()
