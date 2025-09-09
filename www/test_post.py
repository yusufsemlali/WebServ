#!/usr/bin/env python3

import os
import sys

print("Content-Type: text/html")
print("Status: 200 OK")
print()  # Empty line separates headers from body

print("<!DOCTYPE html>")
print("<html>")
print("<head><title>Python CGI POST Test</title></head>")
print("<body>")
print("<h1>Hello from Python CGI!</h1>")

# Check if this is a POST request
request_method = os.environ.get('REQUEST_METHOD', '')
print(f"<p>Request Method: {request_method}</p>")

if request_method == 'POST':
    # Read POST data from stdin
    content_length = os.environ.get('CONTENT_LENGTH', '0')
    if content_length and content_length.isdigit():
        post_data = sys.stdin.read(int(content_length))
        print(f"<p>POST Data Received: {post_data}</p>")
        
        # Parse the form data
        if post_data:
            pairs = post_data.split('&')
            print("<ul>")
            for pair in pairs:
                if '=' in pair:
                    key, value = pair.split('=', 1)
                    print(f"<li>{key} = {value}</li>")
            print("</ul>")
    else:
        print("<p>No POST data found</p>")
else:
    print("<p>This was not a POST request</p>")

print("</body>")
print("</html>")
