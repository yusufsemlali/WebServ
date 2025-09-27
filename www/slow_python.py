#!/usr/bin/env python3

import time
import os

print("Content-Type: text/html")
print("")

print("<html>")
print("<head><title>Slow Python CGI</title></head>")
print("<body>")
print("<h1>Slow Python CGI Script</h1>")

# Simulate slow processing - 3 seconds
print("<p>Processing started at: ", end="")
print(time.strftime("%H:%M:%S"))
print("</p>")

time.sleep(3)

print("<p>Processing completed at: ", end="")
print(time.strftime("%H:%M:%S"))
print("</p>")
print("<p>This script took 3 seconds to complete!</p>")
print("</body>")
print("</html>")