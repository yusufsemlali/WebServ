#!/usr/bin/env python3
import os
import sys

# Read POST data
content_length = int(os.environ.get('CONTENT_LENGTH', 0))
post_data = sys.stdin.read(content_length) if content_length > 0 else ""

# Send response
print("Content-Type: text/html\r")
print("\r")
print("<html><body>")
print(f"<h1>POST Request Received</h1>")
print(f"<p><strong>Content-Length:</strong> {content_length}</p>")
print(f"<p><strong>Content-Type:</strong> {os.environ.get('CONTENT_TYPE', 'not set')}</p>")
print(f"<p><strong>Request Method:</strong> {os.environ.get('REQUEST_METHOD')}</p>")
print(f"<h2>POST Data:</h2>")
print(f"<pre>{post_data}</pre>")
print("</body></html>")