#!/bin/bash

echo "Content-Type: text/html"
echo "Status: 200 OK"
echo ""

echo "<!DOCTYPE html>"
echo "<html>"
echo "<head><title>PHP-CGI Test</title></head>"
echo "<body>"
echo "<h1>Hello from PHP-CGI!</h1>"
echo "<p>This is a simulated PHP-CGI response</p>"
echo "<p>Current time: $(date)</p>"
echo "<p>Server: $SERVER_NAME:$SERVER_PORT</p>"
echo "<p>Request Method: $REQUEST_METHOD</p>"
echo "<p>Query String: $QUERY_STRING</p>"

if [ "$REQUEST_METHOD" = "POST" ] && [ -n "$CONTENT_LENGTH" ] && [ "$CONTENT_LENGTH" -gt 0 ]; then
    echo "<p>POST Data Length: $CONTENT_LENGTH</p>"
    echo "<p>POST Data: $(cat)</p>"
fi

echo "</body>"
echo "</html>"
