#!/usr/bin/env python3

import os
import sys

print("Content-Type: application/json")
print("Status: 200 OK")
print()

# Read POST data if available
request_method = os.environ.get('REQUEST_METHOD', '')
content_length = os.environ.get('CONTENT_LENGTH', '0')

data = ""
if request_method == 'POST' and content_length.isdigit() and int(content_length) > 0:
    data = sys.stdin.read(int(content_length))

# Create JSON response
import json

response = {
    "method": request_method,
    "content_type": os.environ.get('CONTENT_TYPE', ''),
    "content_length": content_length,
    "query_string": os.environ.get('QUERY_STRING', ''),
    "server_info": {
        "name": os.environ.get('SERVER_NAME', ''),
        "port": os.environ.get('SERVER_PORT', ''),
        "software": os.environ.get('SERVER_SOFTWARE', '')
    },
    "post_data": data,
    "post_data_length": len(data)
}

print(json.dumps(response, indent=2))
