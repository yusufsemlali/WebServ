#!/usr/bin/env python3

import os
import sys

print("Content-Type: text/html")
print("Status: 200 OK")
print()  # Empty line separates headers from body

print("<!DOCTYPE html>")
print("<html>")
print("<head><title>CGI Environment Test</title></head>")
print("<body>")
print("<h1>CGI Environment Variables</h1>")

# Display all CGI environment variables
cgi_vars = [
    'GATEWAY_INTERFACE', 'SERVER_SOFTWARE', 'SERVER_PROTOCOL', 
    'SERVER_NAME', 'SERVER_PORT', 'REQUEST_METHOD', 'SCRIPT_NAME',
    'QUERY_STRING', 'CONTENT_TYPE', 'CONTENT_LENGTH', 'REMOTE_ADDR',
    'REMOTE_HOST', 'PATH_INFO', 'PATH_TRANSLATED'
]

print("<table border='1'>")
print("<tr><th>Variable</th><th>Value</th></tr>")
for var in cgi_vars:
    value = os.environ.get(var, 'Not Set')
    print(f"<tr><td>{var}</td><td>{value}</td></tr>")
print("</table>")

print("<h2>All Environment Variables</h2>")
print("<table border='1'>")
print("<tr><th>Variable</th><th>Value</th></tr>")
for key, value in sorted(os.environ.items()):
    print(f"<tr><td>{key}</td><td>{value}</td></tr>")
print("</table>")

print("</body>")
print("</html>")
