#!/usr/bin/python3

import sys
import os
import urllib.parse

# Get filename from query string
query_string = os.environ.get('QUERY_STRING', '')
params = urllib.parse.parse_qs(query_string)
filename = params.get('filename', ['uploaded_file'])[0]

# Sanitize filename (remove path components, keep only basename)
filename = os.path.basename(filename)
if not filename:
    filename = 'uploaded_file'

# Target directory (relative to server root)
upload_dir = './www/files'
file_path = os.path.join(upload_dir, filename)

# Get content length
content_length = int(os.environ.get('CONTENT_LENGTH', 0))

try:
    # Ensure upload directory exists
    os.makedirs(upload_dir, exist_ok=True)
    
    if content_length == 0:
        print("Status: 400 Bad Request\r")
        print("Content-Type: text/plain\r\n\r")
        print("Error: No data received (Content-Length: 0)")
        sys.exit(0)
    
    # Read and save file in chunks
    bytes_read = 0
    chunk_size = 8192
    
    with open(file_path, 'wb') as f:
        while bytes_read < content_length:
            chunk = sys.stdin.buffer.read(min(chunk_size, content_length - bytes_read))
            if not chunk:
                break
            f.write(chunk)
            bytes_read += len(chunk)
    
    # Check if all data was received
    if bytes_read != content_length:
        print("Status: 500 Internal Server Error\r")
        print("Content-Type: text/plain\r\n\r")
        print(f"Error: Incomplete upload. Expected {content_length} bytes, got {bytes_read} bytes")
        # Clean up partial file
        if os.path.exists(file_path):
            os.remove(file_path)
        sys.exit(0)
    
    # Success
    print("Status: 201 Created\r")
    print("Content-Type: application/json\r\n\r")
    print('{{')
    print('  "status": "success",')
    print('  "message": "File uploaded successfully",')
    print(f'  "filename": "{filename}",')
    print(f'  "size": {bytes_read},')
    print(f'  "path": "{file_path}"')
    print('}}')
    
except Exception as e:
    print("Status: 500 Internal Server Error\r")
    print("Content-Type: text/plain\r\n\r")
    print(f"Error: {str(e)}")
    # Clean up partial file on error
    if os.path.exists(file_path):
        try:
            os.remove(file_path)
        except:
            pass
