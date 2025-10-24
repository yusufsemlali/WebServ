#!/usr/bin/env python3
"""
CGI Upload Handler for WebServ
Handles file uploads and manages files in the uploads directory
"""

import os
import sys
import cgi
import cgitb
from datetime import datetime

# Enable CGI error reporting for debugging
cgitb.enable()

# Configuration
UPLOAD_DIR = "./www/uploads"
MAX_FILE_SIZE = 10 * 1024 * 1024  # 10MB

def ensure_upload_dir():
    """Create upload directory if it doesn't exist"""
    if not os.path.exists(UPLOAD_DIR):
        os.makedirs(UPLOAD_DIR, exist_ok=True)

def get_safe_filename(filename):
    """Sanitize filename to prevent directory traversal attacks"""
    # Remove path components and keep only the filename
    filename = os.path.basename(filename)
    # Remove any potentially dangerous characters
    safe_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-"
    filename = ''.join(c if c in safe_chars else '_' for c in filename)
    return filename

def handle_upload():
    """Handle file upload via POST request"""
    ensure_upload_dir()
    
    # Parse the form data
    form = cgi.FieldStorage()
    
    if 'file' not in form:
        print("Status: 400 Bad Request")
        print("Content-Type: text/html")
        print()
        print("<html><body>")
        print("<h1>Error: No file uploaded</h1>")
        print("<p>Please select a file to upload.</p>")
        print("<a href='/uploads'>Back to uploads</a>")
        print("</body></html>")
        return
    
    fileitem = form['file']
    
    # Check if file was actually uploaded
    if not fileitem.filename:
        print("Status: 400 Bad Request")
        print("Content-Type: text/html")
        print()
        print("<html><body>")
        print("<h1>Error: No file selected</h1>")
        print("<a href='/uploads'>Back to uploads</a>")
        print("</body></html>")
        return
    
    # Get safe filename
    filename = get_safe_filename(fileitem.filename)
    filepath = os.path.join(UPLOAD_DIR, filename)
    
    # Check if file already exists and create unique name if needed
    base, ext = os.path.splitext(filename)
    counter = 1
    while os.path.exists(filepath):
        filename = f"{base}_{counter}{ext}"
        filepath = os.path.join(UPLOAD_DIR, filename)
        counter += 1
    
    try:
        # Write the file
        with open(filepath, 'wb') as f:
            # Read file data
            if hasattr(fileitem, 'file'):
                # Read in chunks to handle large files
                chunk_size = 8192
                while True:
                    chunk = fileitem.file.read(chunk_size)
                    if not chunk:
                        break
                    f.write(chunk)
            else:
                f.write(fileitem.value)
        
        # Get file size
        file_size = os.path.getsize(filepath)
        
        # Success response
        print("Status: 201 Created")
        print("Content-Type: text/html")
        print()
        print("<html>")
        print("<head><title>Upload Successful</title></head>")
        print("<body>")
        print("<h1>File Uploaded Successfully!</h1>")
        print(f"<p><strong>Filename:</strong> {filename}</p>")
        print(f"<p><strong>Size:</strong> {file_size} bytes</p>")
        print(f"<p><strong>Location:</strong> {filepath}</p>")
        print(f"<p><strong>Time:</strong> {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>")
        print("<br>")
        print("<a href='/uploads'>View all uploads</a>")
        print("</body>")
        print("</html>")
        
    except Exception as e:
        print("Status: 500 Internal Server Error")
        print("Content-Type: text/html")
        print()
        print("<html><body>")
        print("<h1>Upload Failed</h1>")
        print(f"<p>Error: {str(e)}</p>")
        print("<a href='/uploads'>Back to uploads</a>")
        print("</body></html>")

def handle_get():
    """Display upload form for GET requests"""
    print("Content-Type: text/html")
    print()
    print("""<!DOCTYPE html>
<html>
<head>
    <title>File Upload</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 600px;
            margin: 50px auto;
            padding: 20px;
        }
        .upload-form {
            border: 2px dashed #ccc;
            padding: 30px;
            text-align: center;
            border-radius: 10px;
        }
        input[type="file"] {
            margin: 20px 0;
        }
        button {
            background-color: #4CAF50;
            color: white;
            padding: 12px 30px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 16px;
        }
        button:hover {
            background-color: #45a049;
        }
    </style>
</head>
<body>
    <h1>Upload File</h1>
    <div class="upload-form">
        <form method="POST" enctype="multipart/form-data">
            <p>Select a file to upload:</p>
            <input type="file" name="file" required>
            <br>
            <button type="submit">Upload File</button>
        </form>
    </div>
    <br>
    <p><a href="/uploads">View uploaded files</a></p>
</body>
</html>""")

def main():
    """Main CGI handler"""
    request_method = os.environ.get('REQUEST_METHOD', 'GET')
    
    if request_method == 'POST':
        handle_upload()
    elif request_method == 'GET':
        handle_get()
    else:
        print("Status: 405 Method Not Allowed")
        print("Content-Type: text/html")
        print()
        print("<html><body>")
        print("<h1>405 Method Not Allowed</h1>")
        print(f"<p>Method {request_method} is not supported.</p>")
        print("</body></html>")

if __name__ == "__main__":
    main()
