#!/usr/bin/python3

import sys
import os

content_length = int(os.environ.get('CONTENT_LENGTH', 0))

print("Content-Type: text/html\r\n\r\n")
print("<html><body style='font-family: monospace; padding: 20px;'>")
print("<h1>CGI Large Upload Test</h1>")

if content_length == 0:
    print("<p style='color: red;'>No data received (Content-Length: 0)</p>")
else:
    bytes_read = 0
    chunk_size = 8192
    
    print("<h2>Reading POST Data...</h2>")
    print("<p><strong>Content-Length:</strong> {} bytes ({:.2f} MB)</p>".format(
        content_length, content_length / 1024.0 / 1024.0))
    
    try:
        while bytes_read < content_length:
            chunk = sys.stdin.buffer.read(min(chunk_size, content_length - bytes_read))
            if not chunk:
                break
            bytes_read += len(chunk)
        
        print("<p><strong>Bytes Read:</strong> {} bytes ({:.2f} MB)</p>".format(
            bytes_read, bytes_read / 1024.0 / 1024.0))
        
        if bytes_read == content_length:
            print("<p style='color: green; font-weight: bold;'>✓ SUCCESS: All data received!</p>")
        else:
            print("<p style='color: orange; font-weight: bold;'>⚠ WARNING: Partial data received</p>")
            print("<p>Expected: {} bytes, Got: {} bytes</p>".format(content_length, bytes_read))
            
    except Exception as e:
        print("<p style='color: red;'><strong>ERROR:</strong> {}</p>".format(str(e)))

print("<h2>Environment</h2>")
print("<p><strong>REQUEST_METHOD:</strong> {}</p>".format(os.environ.get('REQUEST_METHOD', 'N/A')))
print("<p><strong>CONTENT_TYPE:</strong> {}</p>".format(os.environ.get('CONTENT_TYPE', 'N/A')))
print("<p><strong>SERVER_PROTOCOL:</strong> {}</p>".format(os.environ.get('SERVER_PROTOCOL', 'N/A')))

print("</body></html>")
