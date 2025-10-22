#!/bin/bash

# Test script to verify 400 error page is served correctly from config

echo "Testing 400 Bad Request error page..."
echo "======================================"

# Send malformed HTTP request (missing HTTP version)
echo -e "GET /test\r\n\r\n" | nc localhost 1025 > /tmp/400_response.txt

# Check if response contains custom error page content
if grep -q "malformed syntax" /tmp/400_response.txt; then
    echo "✓ SUCCESS: Custom 400.html error page is being served!"
    echo ""
    echo "Response preview:"
    head -n 20 /tmp/400_response.txt
else
    echo "✗ FAILED: Custom error page not found"
    echo ""
    echo "Response received:"
    cat /tmp/400_response.txt
fi

rm -f /tmp/400_response.txt
