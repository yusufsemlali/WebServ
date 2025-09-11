#!/usr/bin/env python3

import os
import sys
import json
import time

print("Content-Type: text/html")
print()

print("""<!DOCTYPE html>
<html><head><title>JSON POST Test</title>
<style>
body { font-family: Arial, sans-serif; margin: 40px; background: #f0f8ff; }
.container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; }
.success { color: #2e7d32; background: #e8f5e9; padding: 15px; border-radius: 8px; margin: 15px 0; border-left: 4px solid #4caf50; }
.error { color: #c62828; background: #ffebee; padding: 15px; border-radius: 8px; margin: 15px 0; border-left: 4px solid #f44336; }
.info { color: #1565c0; background: #e3f2fd; padding: 15px; border-radius: 8px; margin: 15px 0; border-left: 4px solid #2196f3; }
.json-data { background: #2d3748; color: #e2e8f0; padding: 15px; border-radius: 8px; font-family: monospace; white-space: pre-wrap; margin: 10px 0; }
</style>
</head><body>
<div class="container">
<h1>🧪 JSON POST Test for WebServ</h1>""")

try:
    request_method = os.environ.get('REQUEST_METHOD', 'GET')
    content_type = os.environ.get('CONTENT_TYPE', 'Not set')
    content_length = int(os.environ.get('CONTENT_LENGTH', '0'))
    
    print(f"""<div class="info">
    <h3>📋 Request Information:</h3>
    <strong>Method:</strong> {request_method}<br>
    <strong>Content-Type:</strong> {content_type}<br>
    <strong>Content-Length:</strong> {content_length} bytes
    </div>""")
    
    if request_method == 'GET':
        print("""<div class="info">
        <h3>🔄 Ready for JSON POST Testing</h3>
        <p>This endpoint is ready to receive JSON POST requests. You can test it by sending JSON data to this URL.</p>
        
        <h4>Example curl commands to test:</h4>
        <div class="json-data">curl -X POST -H "Content-Type: application/json" \\
     -d '{"name":"WebServ","version":"1.0","features":["CGI","POST","JSON"]}' \\
     http://localhost:8080/json_test.py

curl -X POST -H "Content-Type: application/json" \\
     -d '{"user":"testuser","action":"upload","file":"zz.jpg","timestamp":"'$(date)'""}' \\
     http://localhost:8080/json_test.py</div>
        </div>""")
        
    elif request_method == 'POST':
        if content_length > 0:
            # Read the POST data
            raw_data = sys.stdin.buffer.read(content_length)
            print(f"<p>✅ <strong>POST data received:</strong> {len(raw_data)} bytes</p>")
            
            try:
                # Try to decode as text
                json_text = raw_data.decode('utf-8')
                print(f"""<div class="success">
                <h3>📄 Raw POST Data:</h3>
                <div class="json-data">{json_text}</div>
                </div>""")
                
                # Try to parse as JSON
                if content_type.startswith('application/json'):
                    try:
                        json_data = json.loads(json_text)
                        print(f"""<div class="success">
                        <h3>✅ JSON Successfully Parsed!</h3>
                        <p><strong>JSON Type:</strong> {type(json_data).__name__}</p>
                        </div>""")
                        
                        # Display parsed JSON structure
                        if isinstance(json_data, dict):
                            print("""<div class="success">
                            <h4>📊 Parsed JSON Object:</h4>
                            <table style="width: 100%; border-collapse: collapse;">""")
                            for key, value in json_data.items():
                                value_type = type(value).__name__
                                value_str = str(value)
                                if len(value_str) > 100:
                                    value_str = value_str[:100] + "..."
                                print(f"""<tr style="border: 1px solid #ddd;">
                                <td style="padding: 8px; background: #f8f9fa; font-weight: bold;">{key}</td>
                                <td style="padding: 8px;">{value_str}</td>
                                <td style="padding: 8px; font-style: italic; color: #666;">{value_type}</td>
                                </tr>""")
                            print("</table></div>")
                            
                        elif isinstance(json_data, list):
                            print(f"""<div class="success">
                            <h4>📋 Parsed JSON Array:</h4>
                            <p><strong>Length:</strong> {len(json_data)} items</p>
                            <div class="json-data">{json.dumps(json_data, indent=2)}</div>
                            </div>""")
                        
                        # Save JSON to file for persistence
                        os.makedirs("tmp", exist_ok=True)
                        timestamp = int(time.time())
                        filename = f"json_post_{timestamp}.json"
                        filepath = os.path.join("tmp", filename)
                        
                        with open(filepath, 'w') as f:
                            json.dump(json_data, f, indent=2)
                        
                        print(f"""<div class="success">
                        <h4>💾 JSON Data Saved:</h4>
                        <p><strong>File:</strong> {filepath}</p>
                        <p><strong>Size:</strong> {os.path.getsize(filepath)} bytes</p>
                        </div>""")
                        
                    except json.JSONDecodeError as e:
                        print(f"""<div class="error">
                        <h3>❌ JSON Parse Error:</h3>
                        <p><strong>Error:</strong> {str(e)}</p>
                        <p>The received data is not valid JSON format.</p>
                        </div>""")
                        
                else:
                    print(f"""<div class="info">
                    <h3>📝 Non-JSON POST Data Received:</h3>
                    <p>Content-Type is not application/json, but data was received successfully.</p>
                    </div>""")
                    
            except UnicodeDecodeError:
                print(f"""<div class="error">
                <h3>❌ Decode Error:</h3>
                <p>Could not decode POST data as UTF-8 text.</p>
                <p>Received {len(raw_data)} bytes of binary data.</p>
                </div>""")
        else:
            print("""<div class="error">
            <h3>❌ No POST Data:</h3>
            <p>POST request received but no data was sent (Content-Length: 0).</p>
            </div>""")
    
    # Show any saved JSON files
    print("""<hr style="margin: 30px 0;">
    <h3>📁 Saved JSON Files in tmp/:</h3>""")
    
    if os.path.exists("tmp"):
        json_files = [f for f in os.listdir("tmp") if f.endswith('.json')]
        if json_files:
            print("<ul>")
            for file in sorted(json_files):
                filepath = os.path.join("tmp", file)
                size = os.path.getsize(filepath)
                mtime = time.ctime(os.path.getmtime(filepath))
                print(f"<li><strong>{file}</strong> - {size} bytes (saved: {mtime})</li>")
            print("</ul>")
        else:
            print("<p>No JSON files saved yet.</p>")
    else:
        print("<p>tmp/ directory doesn't exist yet.</p>")

except Exception as e:
    print(f"""<div class="error">
    <h3>❌ Script Error:</h3>
    <p><strong>Error:</strong> {str(e)}</p>
    </div>""")
    import traceback
    print(f"<pre>{traceback.format_exc()}</pre>")

print("</div></body></html>")
