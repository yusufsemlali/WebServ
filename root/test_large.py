#!/usr/bin/env python3

import os
import sys

print("Content-Type: text/html")
print("Status: 200 OK")
print()

print("<!DOCTYPE html>")
print("<html>")
print("<head><title>Large Data Test</title></head>")
print("<body>")
print("<h1>Large Data Handling Test</h1>")

request_method = os.environ.get('REQUEST_METHOD', '')
content_length = os.environ.get('CONTENT_LENGTH', '0')

if request_method == 'POST':
    if content_length.isdigit() and int(content_length) > 0:
        data = sys.stdin.read(int(content_length))
        print(f"<p><strong>Received {len(data)} bytes of POST data</strong></p>")
        print(f"<p>First 100 characters: <code>{data[:100]}</code></p>")
        if len(data) > 100:
            print(f"<p>Last 100 characters: <code>{data[-100:]}</code></p>")
    else:
        print("<p>No POST data received</p>")
else:
    print("<p>This endpoint expects POST data</p>")
    print("<p>Try sending a large amount of data to test buffer handling</p>")

# Generate some large output to test response handling
print("<h2>Large Response Test</h2>")
print("<div>")
for i in range(100):
    print(f"<p>Line {i+1}: This is a test line with some content to make the response larger. " * 5 + "</p>")
print("</div>")

print("</body>")
print("</html>")
