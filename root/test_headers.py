#!/usr/bin/env python3

import os
import sys

# Test custom headers
print("Content-Type: text/html")
print("X-Custom-Header: Test-Value")
print("Cache-Control: no-cache")
print("Set-Cookie: test_cookie=test_value; Path=/")
print("Status: 200 OK")
print()

print("<!DOCTYPE html>")
print("<html>")
print("<head><title>Custom Headers Test</title></head>")
print("<body>")
print("<h1>Custom Headers Test</h1>")
print("<p>This response includes custom headers:</p>")
print("<ul>")
print("<li>X-Custom-Header: Test-Value</li>")
print("<li>Cache-Control: no-cache</li>") 
print("<li>Set-Cookie: test_cookie=test_value</li>")
print("</ul>")
print("<p>Check your browser's developer tools to see the headers</p>")
print("</body>")
print("</html>")
