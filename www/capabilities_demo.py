#!/usr/bin/env python3

import cgi
import cgitb
import os
import sys
import json
import time

# Enable CGI error reporting
cgitb.enable()

def save_file(fileitem, upload_dir="tmp"):
    """Save uploaded file and return info"""
    if not fileitem.filename:
        return None
    
    # Create upload directory if it doesn't exist
    os.makedirs(upload_dir, exist_ok=True)
    
    # Create safe filename with timestamp
    timestamp = str(int(time.time()))
    filename = os.path.basename(fileitem.filename)
    safe_filename = f"{timestamp}_{filename}"
    filepath = os.path.join(upload_dir, safe_filename)
    
    # Save file
    try:
        with open(filepath, 'wb') as f:
            f.write(fileitem.file.read())
        
        file_size = os.path.getsize(filepath)
        return {
            'original_name': filename,
            'saved_name': safe_filename,
            'filepath': filepath,
            'size': file_size,
            'content_type': fileitem.type
        }
    except Exception as e:
        return {'error': str(e)}

def process_data_modification(action, data):
    """Simulate data modification operations"""
    results = []
    
    if action == 'create':
        # Create a new data file
        filename = f"data_{int(time.time())}.txt"
        filepath = os.path.join("tmp", filename)
        os.makedirs("tmp", exist_ok=True)
        
        with open(filepath, 'w') as f:
            f.write(f"Created: {time.ctime()}\nData: {data}\n")
        
        results.append(f"✅ Created file: {filename}")
    
    elif action == 'update':
        # Update existing files in tmp/
        updated_count = 0
        if os.path.exists("tmp"):
            for file in os.listdir("tmp"):
                if file.endswith('.txt'):
                    filepath = os.path.join("tmp", file)
                    with open(filepath, 'a') as f:
                        f.write(f"Updated: {time.ctime()} - {data}\n")
                    updated_count += 1
        
        results.append(f"✅ Updated {updated_count} files")
    
    elif action == 'delete':
        # Delete files matching pattern
        deleted_count = 0
        if os.path.exists("tmp"):
            for file in os.listdir("tmp"):
                if data in file:  # Delete files containing the data string
                    os.remove(os.path.join("tmp", file))
                    deleted_count += 1
        
        results.append(f"✅ Deleted {deleted_count} files containing '{data}'")
    
    return results

def main():
    # Print Content-Type header
    print("Content-Type: text/html")
    print()  # Empty line required
    
    # Get request info
    request_method = os.environ.get('REQUEST_METHOD', 'GET')
    content_length = os.environ.get('CONTENT_LENGTH', '0')
    content_type = os.environ.get('CONTENT_TYPE', '')
    
    print("""<!DOCTYPE html>
<html>
<head>
    <title>🔧 WebServ POST Capabilities Demo</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f0f8ff; }
        .container { max-width: 1000px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
        .success { color: #2e7d32; background: #e8f5e9; padding: 15px; border-radius: 8px; margin: 15px 0; border-left: 4px solid #4caf50; }
        .info { color: #1565c0; background: #e3f2fd; padding: 15px; border-radius: 8px; margin: 15px 0; border-left: 4px solid #2196f3; }
        .section { background: #f8f9fa; padding: 20px; border-radius: 8px; margin: 20px 0; border: 2px solid #dee2e6; }
        input, textarea, select { margin: 10px 0; padding: 10px; border: 2px solid #dee2e6; border-radius: 5px; width: 100%; box-sizing: border-box; }
        input[type="submit"] { background: #007bff; color: white; padding: 12px 25px; border: none; border-radius: 6px; cursor: pointer; font-size: 16px; font-weight: bold; width: auto; }
        .demo-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin: 20px 0; }
        .capability { background: #fff; border: 2px solid #e0e0e0; padding: 20px; border-radius: 8px; }
        .result { background: #2d3748; color: #e2e8f0; padding: 15px; border-radius: 8px; font-family: monospace; margin: 10px 0; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔧 WebServ POST Capabilities Demonstration</h1>
        <p style="color: #6c757d;">Your WebServ CGI implementation supports all types of POST operations!</p>""")
    
    if request_method == 'GET':
        print(f"""
        <div class="info">
            <strong>📊 Current Status:</strong><br>
            • Method: {request_method}<br>
            • Ready to demonstrate POST capabilities<br>
            • All operations will modify server-side data
        </div>
        
        <div class="demo-grid">
            <div class="capability">
                <h3>📁 File Upload</h3>
                <form method="POST" enctype="multipart/form-data">
                    <input type="hidden" name="operation" value="upload">
                    <label>Select File:</label>
                    <input type="file" name="uploadfile" required>
                    <input type="submit" value="📤 Upload File">
                </form>
            </div>
            
            <div class="capability">
                <h3>📝 Data Modification</h3>
                <form method="POST">
                    <input type="hidden" name="operation" value="modify">
                    <label>Action:</label>
                    <select name="action" required>
                        <option value="create">Create New Data</option>
                        <option value="update">Update Existing</option>
                        <option value="delete">Delete Data</option>
                    </select>
                    <label>Data:</label>
                    <textarea name="data" placeholder="Enter data to process...">Hello from WebServ POST!</textarea>
                    <input type="submit" value="🔧 Modify Data">
                </form>
            </div>
        </div>
        
        <div class="section">
            <h3>🧪 Advanced POST Test</h3>
            <form method="POST">
                <input type="hidden" name="operation" value="advanced">
                <label>JSON Data:</label>
                <textarea name="json_data" rows="4">{{"name": "WebServ", "status": "working", "features": ["POST", "uploads", "CGI"]}}</textarea>
                <label>Custom Headers:</label>
                <input type="text" name="custom_header" placeholder="X-Custom-Header: value">
                <input type="submit" value="🚀 Send Advanced POST">
            </form>
        </div>""")
        
    elif request_method == 'POST':
        print(f"""<div class="success">✅ POST Request Received Successfully!</div>""")
        
        try:
            # Parse form data
            form = cgi.FieldStorage()
            operation = form.getvalue('operation', 'unknown')
            
            print(f"""<div class="info">
            <strong>📋 POST Request Details:</strong><br>
            • Operation: {operation}<br>
            • Content-Length: {content_length} bytes<br>
            • Content-Type: {content_type}<br>
            </div>""")
            
            if operation == 'upload':
                # Handle file upload
                if 'uploadfile' in form:
                    fileitem = form['uploadfile']
                    result = save_file(fileitem)
                    
                    if result and 'error' not in result:
                        print(f"""<div class="success">
                        <h3>📁 File Upload Successful!</h3>
                        • Original: {result['original_name']}<br>
                        • Saved as: {result['saved_name']}<br>
                        • Size: {result['size']} bytes<br>
                        • Type: {result['content_type']}<br>
                        • Location: {result['filepath']}
                        </div>""")
                    else:
                        print(f"""<div class="error">❌ Upload failed: {result.get('error', 'Unknown error')}</div>""")
                
            elif operation == 'modify':
                # Handle data modification
                action = form.getvalue('action', 'create')
                data = form.getvalue('data', 'default data')
                
                results = process_data_modification(action, data)
                
                print(f"""<div class="success">
                <h3>🔧 Data Modification Complete!</h3>""")
                for result in results:
                    print(f"• {result}<br>")
                print("</div>")
                
            elif operation == 'advanced':
                # Handle advanced POST
                json_data = form.getvalue('json_data', '{}')
                custom_header = form.getvalue('custom_header', '')
                
                try:
                    # Parse JSON
                    parsed_json = json.loads(json_data)
                    print(f"""<div class="success">
                    <h3>🚀 Advanced POST Processing!</h3>
                    • JSON parsed successfully<br>
                    • Keys found: {list(parsed_json.keys())}<br>
                    • Custom header: {custom_header}<br>
                    </div>""")
                    
                    # Show parsed data
                    print(f"""<div class="result">
Parsed JSON Data:
{json.dumps(parsed_json, indent=2)}
                    </div>""")
                    
                except json.JSONDecodeError as e:
                    print(f"""<div class="error">❌ JSON parse error: {e}</div>""")
            
            # Show all received form fields
            if len(form.keys()) > 0:
                print("""<div class="info">
                <h3>📋 All Received Form Fields:</h3>""")
                for key in form.keys():
                    if key != 'uploadfile':  # Skip file field for display
                        value = form.getvalue(key)
                        safe_value = str(value)[:200] + ("..." if len(str(value)) > 200 else "")
                        print(f"• <strong>{key}:</strong> {safe_value}<br>")
                print("</div>")
            
        except Exception as e:
            print(f"""<div class="error">❌ Processing error: {str(e)}</div>""")
        
        # Show test again option
        print("""
        <div style="text-align: center; margin: 30px 0;">
            <form method="GET" style="display: inline-block;">
                <input type="submit" value="🔄 Test More Operations" style="background: #28a745;">
            </form>
        </div>""")
    
    # Show current directory contents
    print("""
        <hr style="margin: 40px 0;">
        <h3>📂 Server-Side Files (tmp/ directory):</h3>
        <div class="result">""")
    
    try:
        if os.path.exists("tmp"):
            files = os.listdir("tmp")
            if files:
                for file in files:
                    filepath = os.path.join("tmp", file)
                    size = os.path.getsize(filepath)
                    mtime = time.ctime(os.path.getmtime(filepath))
                    print(f"{file} ({size} bytes, modified: {mtime})<br>")
            else:
                print("Directory is empty<br>")
        else:
            print("tmp/ directory does not exist yet<br>")
    except Exception as e:
        print(f"Error listing files: {e}<br>")
    
    print("""</div>
        
        <div class="info" style="margin-top: 30px;">
            <strong>✅ WebServ POST Capabilities Summary:</strong><br>
            • File Uploads: <span style="color: green; font-weight: bold;">SUPPORTED</span><br>
            • Data Modification: <span style="color: green; font-weight: bold;">SUPPORTED</span><br>
            • Form Processing: <span style="color: green; font-weight: bold;">SUPPORTED</span><br>
            • JSON Handling: <span style="color: green; font-weight: bold;">SUPPORTED</span><br>
            • File System Operations: <span style="color: green; font-weight: bold;">SUPPORTED</span>
        </div>
    </div>
</body>
</html>""")

if __name__ == "__main__":
    main()
