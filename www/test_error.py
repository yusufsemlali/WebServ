#!/usr/bin/env python3

import os
import sys
import time

print("Content-Type: text/html")
print("Status: 200 OK")
print()

print("<!DOCTYPE html>")
print("<html>")
print("<head><title>CGI Error Test</title></head>")
print("<body>")
print("<h1>CGI Error Handling Test</h1>")

# Test different types of potential errors
test_type = os.environ.get('QUERY_STRING', '')

if test_type == 'stderr':
    print("<p>Testing stderr output...</p>")
    sys.stderr.write("This is an error message to stderr\n")
    sys.stderr.flush()
    print("<p>Error message sent to stderr</p>")

elif test_type == 'exit1':
    print("<p>Testing exit code 1...</p>")
    print("</body></html>")
    sys.exit(1)

elif test_type == 'timeout':
    print("<p>Testing timeout (sleeping for 10 seconds)...</p>")
    print("</body></html>")
    time.sleep(10)

elif test_type == 'exception':
    print("<p>Testing Python exception...</p>")
    print("</body></html>")
    raise Exception("This is a test exception")

else:
    print("<p>Choose a test type:</p>")
    print("<ul>")
    print("<li><a href='/test_error.py?stderr'>Test stderr output</a></li>")
    print("<li><a href='/test_error.py?exit1'>Test exit code 1</a></li>")
    print("<li><a href='/test_error.py?timeout'>Test timeout</a></li>")
    print("<li><a href='/test_error.py?exception'>Test Python exception</a></li>")
    print("</ul>")

print("</body>")
print("</html>")
