# WebServ Testing - Quick Reference

## 🚀 Quick Test Commands

### 1. Automated Test Suite (Shell)
```bash
./comprehensive_test_suite.sh
```
**Tests**: GET/POST/DELETE, error pages, CGI, uploads, MIME types

### 2. Python Test Suite (Advanced)
```bash
./python_tester.py
```
**Tests**: Concurrent connections, all HTTP features, detailed reporting

### 3. Stress Test
```bash
./stress_test.sh
```
**Tests**: 100+ concurrent requests, memory usage, stability

### 4. Manual 400 Error Test
```bash
./test_400_error.sh
```
**Tests**: Custom 400 error page serving

---

## 📋 Test Checklist Reference

Use `TESTING_CHECKLIST.md` for complete evaluation preparation.

### Critical Features:
- ✅ Non-blocking I/O (poll/select/epoll)
- ✅ Configuration file parsing
- ✅ GET, POST, DELETE methods
- ✅ Static file serving
- ✅ File uploads
- ✅ CGI execution (Python/PHP)
- ✅ Custom error pages
- ✅ Multiple servers/ports
- ✅ Client body size limits
- ✅ Directory listing (autoindex)
- ✅ Browser compatibility

---

## 🌐 Browser Testing

### Open in Browser:
- **Server 1**: http://localhost:1024/
- **Server 2**: http://localhost:1025/

### What to Check:
1. Page loads completely (200 OK)
2. All resources load (CSS, JS, images)
3. Can upload files via form
4. Error pages display correctly (404, 403)
5. Directory listing works (if autoindex on)

---

## 🔧 Manual Test Commands

### Basic Tests
```bash
# GET request
curl http://localhost:1024/

# POST with data
curl -X POST http://localhost:1024/ -d "name=test"

# DELETE
curl -X DELETE http://localhost:1024/bin/file.txt

# File upload
curl -X POST http://localhost:1024/upload -F "file=@test.txt"
```

### Error Testing
```bash
# 404
curl http://localhost:1024/nonexistent

# 400 (malformed)
echo -e "GET /\r\n\r\n" | nc localhost 1024

# 413 (too large)
dd if=/dev/zero bs=1M count=10 | \
curl -X POST http://localhost:1024/upload --data-binary @-
```

### CGI Testing
```bash
# Python CGI
curl http://localhost:1025/hello.py

# POST to CGI
curl -X POST http://localhost:1025/form.py -d "name=John"
```

---

## 📊 Expected Results

### Successful Server Should:
- ✅ Start without errors
- ✅ Parse configuration file
- ✅ Listen on ports 1024 and 1025
- ✅ Serve static HTML/CSS/JS
- ✅ Handle 100+ concurrent connections
- ✅ Return correct status codes
- ✅ Serve custom error pages
- ✅ Execute CGI scripts
- ✅ Accept file uploads
- ✅ Enforce body size limits
- ✅ Work with Chrome/Firefox
- ✅ Never crash or hang
- ✅ No memory leaks (valgrind)

---

## 🐛 Common Issues to Check

### Server Won't Start
```bash
# Check if ports are already in use
netstat -tuln | grep -E ':(1024|1025)'

# Kill conflicting process
lsof -ti:1024 | xargs kill -9
```

### Server Crashes
```bash
# Run with valgrind
valgrind --leak-check=full ./webserv configs/default.conf

# Check logs for errors
tail -f error.log
```

### CGI Not Working
- Check interpreter path in config (./usr/bin/python3)
- Verify CGI script has execute permissions
- Test script independently: `python3 www/hello.py`
- Check environment variables are passed

### File Upload Fails
- Verify upload directory exists: `mkdir -p www/uploads`
- Check permissions: `chmod 755 www/uploads`
- Verify client_size setting in config

---

## 📚 Documentation Files

1. **TESTING_CHECKLIST.md** - Complete checklist for evaluation
2. **TESTING_GUIDE.md** - Detailed testing instructions
3. **FIX_400_ERROR_PAGE.md** - 400 error page fix documentation

---

## 🎯 Before Evaluation

### Final Checks:
```bash
# 1. Clean build
make re

# 2. Run all tests
./comprehensive_test_suite.sh
./python_tester.py
./stress_test.sh

# 3. Check for leaks
valgrind --leak-check=full ./webserv configs/default.conf
# (Then Ctrl+C after a few requests)

# 4. Browser test
# Open http://localhost:1024 in Chrome/Firefox

# 5. Review config
cat configs/default.conf
```

### Prepare to Demonstrate:
1. Server starts with config
2. Serves static website in browser
3. Upload file through HTML form
4. Execute Python CGI script
5. Show custom error pages (404, 400)
6. Multiple concurrent connections
7. Server handles Ctrl+C gracefully

---

## 💡 Tips for Evaluation

1. **Know your config**: Explain every directive
2. **Show non-blocking**: Concurrent curl requests
3. **Demonstrate CGI**: Working Python script
4. **Error handling**: Trigger 400, 404, 413
5. **Compare with NGINX**: Similar behavior
6. **Handle questions**: Read the RFCs (HTTP/1.1)
7. **Edge cases**: Malformed requests, disconnections
8. **No crashes**: Stress test beforehand

---

## 📞 Quick Help

**Tests not running?**
- Ensure server is running: `ps aux | grep webserv`
- Check ports: `netstat -tuln | grep 102`
- Restart server: `./webserv configs/default.conf`

**All tests failing?**
- Verify config file path
- Check file permissions
- Review server logs
- Test simple curl first: `curl http://localhost:1024/`

Good luck with your evaluation! 🚀
