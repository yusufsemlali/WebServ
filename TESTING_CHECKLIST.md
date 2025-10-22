# WebServ Testing Checklist - 42 Subject Compliance

## 🎯 Mandatory Requirements Testing

### 1. Server Basics
- [ ] Server starts with configuration file argument: `./webserv configs/default.conf`
- [ ] Server starts with default config if no argument provided
- [ ] Server uses non-blocking I/O (poll/select/epoll/kqueue)
- [ ] Server never hangs or blocks
- [ ] Server handles multiple simultaneous connections
- [ ] Server properly handles client disconnections
- [ ] Server doesn't crash under any circumstances

### 2. HTTP Methods
- [ ] **GET**: Retrieve resources (files, directories)
- [ ] **POST**: Send data (forms, file uploads, JSON)
- [ ] **DELETE**: Remove resources
- [ ] Per-route method restrictions work correctly
- [ ] 405 Method Not Allowed returned when appropriate

### 3. Static Website Serving
- [ ] Serve HTML files
- [ ] Serve CSS files with correct MIME type
- [ ] Serve JavaScript files
- [ ] Serve images (PNG, JPG, GIF, etc.)
- [ ] Serve other static content
- [ ] Correct Content-Type headers for all file types

### 4. Configuration File Features
- [ ] Multiple servers on different ports
- [ ] Multiple servers on same port (virtual hosts)
- [ ] Server names (Host header matching)
- [ ] Root directory configuration
- [ ] Index file specification
- [ ] Custom error pages (400, 403, 404, 405, 500)
- [ ] Client max body size enforcement
- [ ] Listen on multiple interfaces (0.0.0.0, localhost, specific IPs)

### 5. Location/Route Configuration
- [ ] Prefix matching (e.g., `/upload`, `/api`)
- [ ] Extension matching (e.g., `*.php`, `*.py`)
- [ ] Method restrictions per location
- [ ] Custom root per location
- [ ] Directory listing (autoindex on/off)
- [ ] Default files for directories
- [ ] HTTP redirections (return directive)

### 6. CGI Support
- [ ] Execute PHP scripts (php-cgi)
- [ ] Execute Python scripts
- [ ] Pass environment variables correctly
- [ ] Handle CGI input (POST data)
- [ ] Handle CGI output (headers + body)
- [ ] Handle chunked request bodies (un-chunk before CGI)
- [ ] Handle CGI responses without Content-Length
- [ ] Run CGI in correct directory for relative paths
- [ ] CGI timeouts handled

### 7. File Upload
- [ ] Handle multipart/form-data
- [ ] Save uploaded files to configured location
- [ ] Respect client_max_body_size
- [ ] Return 413 Payload Too Large when exceeded
- [ ] Handle multiple file uploads
- [ ] Handle large file uploads

### 8. Error Handling
- [ ] Custom error pages from config
- [ ] Default error pages when custom not available
- [ ] 400 Bad Request (malformed requests)
- [ ] 403 Forbidden (no permissions, no autoindex)
- [ ] 404 Not Found (missing resources)
- [ ] 405 Method Not Allowed (wrong method)
- [ ] 413 Payload Too Large (body size limit)
- [ ] 500 Internal Server Error (server errors)
- [ ] 501 Not Implemented (unknown methods)

### 9. HTTP Protocol Compliance
- [ ] HTTP/1.0 support
- [ ] HTTP/1.1 support
- [ ] Correct status codes
- [ ] Correct response headers
- [ ] Handle keep-alive connections
- [ ] Handle Connection: close
- [ ] Request timeout (no hanging forever)
- [ ] Parse headers correctly
- [ ] Handle header edge cases

### 10. Browser Compatibility
- [ ] Works with Chrome/Chromium
- [ ] Works with Firefox
- [ ] Works with Safari
- [ ] Can load complete websites with resources
- [ ] Handles concurrent requests (CSS, JS, images)

### 11. Stress Testing
- [ ] Handle 100+ simultaneous connections
- [ ] Handle rapid connect/disconnect
- [ ] Handle slow clients (slow send/recv)
- [ ] Memory doesn't leak under load
- [ ] Server remains available under stress
- [ ] No segfaults or crashes

### 12. Edge Cases
- [ ] Empty requests
- [ ] Malformed requests
- [ ] Very long URLs
- [ ] Very long headers
- [ ] Missing required headers
- [ ] Invalid HTTP versions
- [ ] Incomplete requests
- [ ] Client disconnects mid-request
- [ ] Client disconnects mid-response

## 🎁 Bonus Features (Optional)
- [ ] Cookies support
- [ ] Session management
- [ ] Multiple CGI types
- [ ] Virtual hosts (Host header routing)

## 🧪 Testing Tools Required
1. **Manual Browser Testing**: Chrome, Firefox
2. **Command-line**: curl, telnet, netcat (nc)
3. **Load Testing**: siege, ab (Apache Bench), or custom script
4. **Automated Testing**: Python/Shell scripts
5. **Comparison**: Test same scenarios with NGINX

## 📝 Test Documentation
- Record all test results
- Document any deviations from NGINX behavior
- Note any known limitations
- Prepare demo scenarios for evaluation
