#!/usr/bin/env python3
import os
import json

def format_bytes(bytes_val):
    """Convert bytes to human readable format"""
    if bytes_val >= 1000000:
        return f"{bytes_val / 1000000:.1f} MB"
    elif bytes_val >= 1000:
        return f"{bytes_val / 1000:.1f} KB"
    else:
        return f"{bytes_val} bytes"

def main():
    # Send HTTP headers
    print("Content-Type: application/json\r")
    print("\r")
    
    # Try to get from environment variable (if server passes it)
    # Otherwise use default from config
    max_size = int(os.environ.get('CLIENT_MAX_BODY_SIZE', '10000000'))
    
    info = {
        "max_upload_size_bytes": max_size,
        "max_upload_size_formatted": format_bytes(max_size),
        "allowed_methods": ["GET", "POST", "DELETE"]
    }
    
    print(json.dumps(info))

if __name__ == "__main__":
    main()
