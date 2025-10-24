#!/usr/bin/python3

import platform
import sys
import os

print("Content-Type: text/html\r\n\r\n")
print("<html><body style='font-family: Arial, sans-serif; padding: 20px;'>")
print("<h1>CGI Environment Information</h1>")

print("<h2>Python Info</h2>")
print("<p><strong>Python Version:</strong> " + sys.version + "</p>")
print("<p><strong>Python Executable:</strong> " + sys.executable + "</p>")
print("<p><strong>Platform:</strong> " + platform.platform() + "</p>")

print("<h2>CGI Standard Variables</h2>")
print("<p><strong>REQUEST_METHOD:</strong> " + os.environ.get('REQUEST_METHOD', 'N/A') + "</p>")
print("<p><strong>SERVER_SOFTWARE:</strong> " + os.environ.get('SERVER_SOFTWARE', 'N/A') + "</p>")
print("<p><strong>SERVER_PROTOCOL:</strong> " + os.environ.get('SERVER_PROTOCOL', 'N/A') + "</p>")
print("<p><strong>QUERY_STRING:</strong> " + os.environ.get('QUERY_STRING', '') + "</p>")
print("<p><strong>REQUEST_URI:</strong> " + os.environ.get('REQUEST_URI', 'N/A') + "</p>")
print("<p><strong>SCRIPT_NAME:</strong> " + os.environ.get('SCRIPT_NAME', 'N/A') + "</p>")

print("<h2>HTTP Headers (from client)</h2>")
print("<table border='1' cellpadding='5' style='border-collapse: collapse;'>")
print("<tr><th>Header</th><th>Value</th></tr>")
for key, value in sorted(os.environ.items()):
    if key.startswith('HTTP_'):
        header_name = key[5:].replace('_', '-').title()
        print("<tr><td>" + header_name + "</td><td>" + value + "</td></tr>")
print("</table>")

print("</body></html>")
