#!/bin/bash

echo "Setting up test environment..."

# Create directory structure
mkdir -p www/api www/uploads www/files www/cgi-bin errors

# Create main index file
cat > www/index.html << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>WebServ Test Server</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        h1 { color: #333; }
        .links { margin: 20px 0; }
        .links a { display: block; margin: 5px 0; color: #0066cc; }
    </style>
</head>
<body>
    <h1>Welcome to WebServ!</h1>
    <p>This is the main page of your WebServ HTTP server.</p>
    
    <div class="links">
        <h2>Test Links:</h2>
        <a href="/files/">Browse Files Directory</a>
        <a href="/api/">API Directory</a>
        <a href="/uploads/">Uploads Directory</a>
        <a href="/files/test.txt">Download Test File</a>
        <a href="/nonexistent">Test 404 Error</a>
    </div>
    
    <p><strong>Server Status:</strong> Running successfully!</p>
</body>
</html>
EOF

# Create API page
cat > www/api/index.html << 'EOF'
<!DOCTYPE html>
<html>
<head><title>API Directory</title></head>
<body>
    <h1>API Endpoints</h1>
    <p>This is the API directory.</p>
    <p>You can send POST requests here to test the server.</p>
</body>
</html>
EOF

# Create test files
echo "This is a test file for download and testing." > www/files/test.txt
echo "Another test file" > www/files/sample.txt

# Create error pages
cat > errors/404.html << 'EOF'
<!DOCTYPE html>
<html>
<head><title>404 - Page Not Found</title></head>
<body>
    <h1>404 - Page Not Found</h1>
    <p>The requested page could not be found.</p>
    <a href="/">Return to Home</a>
</body>
</html>
EOF

cat > errors/500.html << 'EOF'
<!DOCTYPE html>
<html>
<head><title>500 - Internal Server Error</title></head>
<body>
    <h1>500 - Internal Server Error</h1>
    <p>Something went wrong on the server.</p>
    <a href="/">Return to Home</a>
</body>
</html>
EOF

# Set permissions
chmod 644 www/index.html www/api/index.html www/files/test.txt www/files/sample.txt
chmod 644 errors/404.html errors/500.html
chmod 755 www www/api www/files www/uploads www/cgi-bin errors

echo "Test environment setup complete!"
echo ""
echo "Directory structure:"
echo "www/"
echo "├── index.html (main page)"
echo "├── api/"
echo "│   └── index.html"
echo "├── files/"
echo "│   ├── test.txt"
echo "│   └── sample.txt"
echo "├── uploads/ (empty)"
echo "└── cgi-bin/ (empty)"
echo ""
echo "errors/"
echo "├── 404.html"
echo "└── 500.html"
echo ""
echo "Now run: ./webserv configs/sample.conf"
echo "Then open: http://localhost:8080/"