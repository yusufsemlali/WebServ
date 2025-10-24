#!/bin/bash

echo "Testing CGI Upload Handler..."
echo

# Create a test file
echo "This is a test file for CGI upload" > /tmp/test_upload.txt

# Test 1: GET request to see upload form
echo "Test 1: GET /uploads (should show HTML form)"
curl -s http://localhost:1025/uploads | head -20
echo
echo "---"
echo

# Test 2: POST file upload
echo "Test 2: POST /uploads (uploading file)"
curl -X POST -F "file=@/tmp/test_upload.txt" http://localhost:1025/uploads
echo
echo "---"
echo

# Test 3: Verify file was created
echo "Test 3: Check if file exists in www/uploads/"
ls -lh www/uploads/
echo

# Cleanup
rm -f /tmp/test_upload.txt

echo "Tests complete!"
