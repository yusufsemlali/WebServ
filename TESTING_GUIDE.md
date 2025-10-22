# Complete WebServ Testing Guide - 42 Project

## 📚 Quick Start

### 1. Run Automated Test Suite
```bash
# Start your server first
./webserv configs/default.conf

# In another terminal, run tests
./comprehensive_test_suite.sh
```

### 2. Run Stress Tests
```bash
./stress_test.sh
```

### 3. Manual Browser Testing
Open: `http://localhost:1024/` and `http://localhost:1025/`

---

## 🎯 Critical Subject Requirements

### **Non-Blocking I/O** (MANDATORY)
**Requirement**: Must use poll/select/epoll/kqueue, never block

**Test**:
```bash
# While server is handling a request, send another
curl http://localhost:1024/ &
curl http://localhost:1024/files/test.txt &
wait
# Both should complete without blocking each other
```

**Failure Signs**:
- Server hangs on one request while processing another
- Can't handle concurrent connections
- Uses blocking read/write without poll

---

### **Configuration File** (MANDATORY)
**Requirement**: Parse and use configuration file

**Current Config Check**:
```bash
cat configs/default.conf
```

**Must Support**:
- ✅ Multiple servers (ports 1024 and 1025)
- ✅ Error pages (400, 403, 404, 405, 500)
- ✅ Client max body size
- ✅ Root directory
- ✅ Index files
- ✅ Location blocks
- ✅ CGI configuration

---

### **GET, POST, DELETE Methods** (MANDATORY)

**Test GET**:
```bash
curl http://localhost:1024/
curl http://localhost:1024/files/test.txt
```

**Test POST**:
```bash
# Form data
curl -X POST http://localhost:1024/ -d "name=test&value=123"

# File upload
curl -X POST http://localhost:1024/upload -F "file=@test.txt"

# JSON
curl -X POST http://localhost:1024/ \
  -H "Content-Type: application/json" \
  -d '{"key":"value"}'
```

**Test DELETE**:
```bash
# Create a file first
echo "test" > www/bin/deleteme.txt

# Delete it
curl -X DELETE http://localhost:1024/bin/deleteme.txt
```

---

### **Static Website** (MANDATORY)
**Requirement**: Serve fully static websites

**Test**:
```bash
# HTML
curl http://localhost:1024/

# CSS
curl -I http://localhost:1024/style.css
# Should return: Content-Type: text/css

# JavaScript
curl -I http://localhost:1024/script.js
# Should return: Content-Type: application/javascript

# Images
curl -I http://localhost:1024/logo.png
# Should return: Content-Type: image/png
```

**Browser Test**: Open in browser and check Developer Tools:
- All resources load (200 OK)
- Correct MIME types
- No CORS errors (for same-origin)

---

### **File Upload** (MANDATORY)
**Requirement**: Clients must be able to upload files

**Config Check**:
```nginx
location /upload {
    methods POST;
    client_size 10000000;  # 10MB limit
}
```

**Test**:
```bash
# Simple upload
curl -X POST http://localhost:1024/upload \
  -F "file=@test.txt" \
  -F "name=testfile"

# Check if file was saved
ls -la www/uploads/
```

**Browser Test**: Create HTML form:
```html
<form action="/upload" method="POST" enctype="multipart/form-data">
    <input type="file" name="file">
    <button>Upload</button>
</form>
```

---

### **CGI Execution** (MANDATORY)
**Requirement**: At least one CGI (PHP, Python, etc.)

**Current CGI Config**:
```nginx
location *.py {
    methods GET POST;
    cgi_pass ./usr/bin/python3;
}

location *.php {
    methods GET POST;
    cgi_pass ./usr/bin/php-cgi;
}
```

**Test Python CGI**:
```bash
# GET request
curl http://localhost:1025/hello.py

# POST request
curl -X POST http://localhost:1025/form_handler.py \
  -d "name=John&age=30"
```

**Test PHP CGI** (if available):
```bash
curl http://localhost:1025/info.php
```

**CGI Environment Variables** (must be passed):
- `REQUEST_METHOD`
- `QUERY_STRING`
- `CONTENT_TYPE`
- `CONTENT_LENGTH`
- `PATH_INFO`
- `SCRIPT_NAME`
- `SERVER_PROTOCOL`

---

### **Error Pages** (MANDATORY)
**Requirement**: Custom error pages or default ones

**Test Each Error**:
```bash
# 400 Bad Request
echo -e "INVALID REQUEST\r\n\r\n" | nc localhost 1025

# 403 Forbidden (directory without index, no autoindex)
curl http://localhost:1024/uploads/

# 404 Not Found
curl http://localhost:1024/nonexistent

# 405 Method Not Allowed
curl -X PUT http://localhost:1024/  # If PUT not allowed

# 413 Payload Too Large
dd if=/dev/zero bs=1M count=10 | \
curl -X POST http://localhost:1024/upload --data-binary @-

# 500 Internal Server Error (CGI crash)
# Create a broken CGI script that crashes
```

**Verify**: Each should return custom HTML page from `www/errors/`

---

### **Client Body Size Limit** (MANDATORY)
**Requirement**: Enforce max body size, return 413 if exceeded

**Config**:
```nginx
client_size 1000000;  # 1MB
```

**Test**:
```bash
# Create 2MB file
dd if=/dev/zero of=big.bin bs=1M count=2

# Should return 413
curl -X POST http://localhost:1024/upload \
  --data-binary @big.bin \
  -w "\n%{http_code}\n"

rm big.bin
```

---

### **Multiple Ports** (MANDATORY)
**Requirement**: Listen on multiple ports

**Test**:
```bash
# Port 1024
curl http://localhost:1024/

# Port 1025
curl http://localhost:1025/

# Both should work independently
```

---

### **Directory Listing (Autoindex)** (MANDATORY)
**Requirement**: Enable/disable directory listing

**With Autoindex ON**:
```bash
curl http://localhost:1024/download/
# Should show file listing HTML
```

**With Autoindex OFF**:
```bash
curl http://localhost:1024/uploads/
# Should return 403 Forbidden
```

---

### **HTTP Redirections** (MANDATORY)
**Requirement**: Support return directive

**Config Example**:
```nginx
location /redirect {
    return https://www.google.com;
}
```

**Test**:
```bash
curl -L http://localhost:1024/redirect
# Should follow redirect
```

---

## 🧪 Advanced Testing

### Test with Telnet (Raw HTTP)
```bash
telnet localhost 1024
```
Then type:
```http
GET / HTTP/1.1
Host: localhost
Connection: close

```
(Press Enter twice after last line)

### Test Keep-Alive
```bash
# Send multiple requests on same connection
(
  echo -e "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
  sleep 1
  echo -e "GET /files/test.txt HTTP/1.1\r\nHost: localhost\r\n\r\n"
) | nc localhost 1024
```

### Test Chunked Transfer Encoding (if implemented)
```bash
curl -X POST http://localhost:1024/upload \
  -H "Transfer-Encoding: chunked" \
  -d "test data"
```

---

## 🔍 Compare with NGINX

**Install NGINX**:
```bash
sudo apt install nginx
```

**Create similar config** and compare:
```bash
# Test with your server
curl -I http://localhost:1024/

# Test with NGINX
curl -I http://localhost:80/

# Compare responses
```

---

## 📊 Browser Developer Tools Testing

1. **Open Developer Tools** (F12)
2. **Network Tab**: Monitor all requests
3. **Check**:
   - Status codes (200, 404, etc.)
   - Response headers
   - Content-Type headers
   - Response times
   - Keep-alive connections

---

## ⚠️ Edge Cases to Test

### Malformed Requests
```bash
# Missing HTTP version
echo -e "GET /\r\n\r\n" | nc localhost 1024

# Invalid method
echo -e "INVALID / HTTP/1.1\r\n\r\n" | nc localhost 1024

# Missing Host header (HTTP/1.1 requires it)
echo -e "GET / HTTP/1.1\r\n\r\n" | nc localhost 1024

# Extremely long URL
curl http://localhost:1024/$(python3 -c "print('a'*10000)")

# Invalid characters in URL
curl http://localhost:1024/%00%01%02
```

### Connection Edge Cases
```bash
# Client disconnects immediately
(echo "GET / HTTP/1.1"; kill $$) | nc localhost 1024

# Slow client (partial send)
(
  echo -n "GET / HTTP/1.1\r\n"
  sleep 5
  echo -e "Host: localhost\r\n\r\n"
) | nc localhost 1024
```

---

## 🎯 Evaluation Preparation

### Before Defense:

1. **Clean compilation**: `make re && ./webserv`
2. **No leaks**: Test with `valgrind`
```bash
valgrind --leak-check=full ./webserv configs/default.conf
```
3. **Test all features**: Run all test scripts
4. **Prepare demos**:
   - Static website
   - File upload
   - CGI script
   - Error pages
   - Multiple servers
5. **Know your config**: Explain every directive
6. **Compare with NGINX**: Show similar behavior

### During Defense:

- Demonstrate non-blocking I/O (concurrent requests)
- Show it works with real browser
- Upload a file through browser
- Execute CGI script
- Trigger error pages
- Handle crashes gracefully (Ctrl+C, kill signal)
- Stress test without crashes

---

## 📝 Common Mistakes to Avoid

❌ **Blocking on read/write** without poll/select
❌ **Memory leaks** under load
❌ **Segfaults** on edge cases
❌ **Hanging forever** on bad requests
❌ **Fork without waitpid** (zombie processes)
❌ **CGI not getting environment variables**
❌ **Wrong MIME types** (serves HTML as plain text)
❌ **Ignoring client body size limit**
❌ **Not handling client disconnection**
❌ **Hardcoded error pages** (not from config)

---

## ✅ Success Criteria

Your WebServ should:
- ✅ Never crash or hang
- ✅ Handle 100+ concurrent connections
- ✅ Work with Chrome/Firefox
- ✅ Pass all automated tests
- ✅ Match NGINX behavior (where applicable)
- ✅ No memory leaks (valgrind clean)
- ✅ Graceful shutdown (Ctrl+C)
- ✅ Parse config correctly
- ✅ Execute CGI properly
- ✅ Serve static sites perfectly

Good luck! 🚀
