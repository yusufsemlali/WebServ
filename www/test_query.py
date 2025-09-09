#!/usr/bin/env python3

import os
import sys
import urllib.parse

print("Content-Type: text/html")
print("Status: 200 OK")
print()

print("<!DOCTYPE html>")
print("<html>")
print("<head><title>CGI Query String Test</title></head>")
print("<body>")
print("<h1>Query String Processing</h1>")

query_string = os.environ.get('QUERY_STRING', '')
print(f"<p><strong>Raw Query String:</strong> {query_string}</p>")

if query_string:
    print("<h2>Parsed Parameters:</h2>")
    print("<ul>")
    pairs = query_string.split('&')
    for pair in pairs:
        if '=' in pair:
            key, value = pair.split('=', 1)
            # URL decode
            key = urllib.parse.unquote_plus(key)
            value = urllib.parse.unquote_plus(value)
            print(f"<li><strong>{key}:</strong> {value}</li>")
        else:
            print(f"<li><strong>{pair}:</strong> (no value)</li>")
    print("</ul>")
else:
    print("<p>No query string provided</p>")
    print("<p><a href='/test_query.py?name=John&age=25&city=Paris'>Test with sample query</a></p>")

print("</body>")
print("</html>")
