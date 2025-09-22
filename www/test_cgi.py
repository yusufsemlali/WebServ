#!/usr/bin/python3
print("Content-Type: text/html\r")
print("\r")
print("<html>")
print("<head><title>CGI Test</title></head>")
print("<body>")
print("<h1>Hello from Python CGI!</h1>")
print("<p>This CGI script is working!</p>")
print("<p>Server time: ", end="")
import datetime
print(datetime.datetime.now())
print("</p>")
print("</body>")
print("</html>")