#!/usr/bin/env python3

print("Content-Type: text/html")
print("Status: 200 OK")
print()  # Empty line separates headers from body

print("<!DOCTYPE html>")
print("<html>")
print("<head><title>Python CGI Test</title></head>")
print("<body>")
print("<h1>Hello from Python CGI!</h1>")
print("<p>This is a test CGI script written in Python.</p>")
print("<p>Server generated this page dynamically.</p>")
print("</body>")
print("</html>")