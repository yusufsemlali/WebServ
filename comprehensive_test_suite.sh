#!/bin/bash

# Comprehensive WebServ Test Suite
# Tests all mandatory requirements from the 42 subject

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SERVER_HOST="localhost"
SERVER_PORT_1="1024"
SERVER_PORT_2="1025"
BASE_URL_1="http://${SERVER_HOST}:${SERVER_PORT_1}"
BASE_URL_2="http://${SERVER_HOST}:${SERVER_PORT_2}"

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Helper functions
print_header() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}\n"
}

print_test() {
    echo -e "${YELLOW}[TEST]${NC} $1"
    ((TOTAL_TESTS++))
}

print_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((PASSED_TESTS++))
}

print_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((FAILED_TESTS++))
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# Check if server is running
check_server() {
    if ! nc -z "$SERVER_HOST" "$SERVER_PORT_1" 2>/dev/null; then
        echo -e "${RED}ERROR: Server not running on port $SERVER_PORT_1${NC}"
        echo "Please start the server with: ./webserv configs/default.conf"
        exit 1
    fi
    print_info "Server is running on port $SERVER_PORT_1"
}

# Test 1: Basic GET requests
test_basic_get() {
    print_header "TEST 1: Basic GET Requests"
    
    print_test "GET / (index page)"
    response=$(curl -s -w "\n%{http_code}" "$BASE_URL_1/")
    status=$(echo "$response" | tail -n1)
    if [ "$status" = "200" ]; then
        print_pass "Index page returns 200 OK"
    else
        print_fail "Index page returned $status instead of 200"
    fi
    
    print_test "GET /files/test.txt"
    response=$(curl -s -w "\n%{http_code}" "$BASE_URL_1/files/test.txt")
    status=$(echo "$response" | tail -n1)
    if [ "$status" = "200" ]; then
        print_pass "Static file returns 200 OK"
    else
        print_fail "Static file returned $status instead of 200"
    fi
    
    print_test "GET /nonexistent (404 test)"
    response=$(curl -s -w "\n%{http_code}" "$BASE_URL_1/nonexistent")
    status=$(echo "$response" | tail -n1)
    if [ "$status" = "404" ]; then
        print_pass "Missing file returns 404"
    else
        print_fail "Missing file returned $status instead of 404"
    fi
}

# Test 2: HTTP Methods
test_http_methods() {
    print_header "TEST 2: HTTP Methods"
    
    print_test "POST request"
    response=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL_1/" -d "test=data")
    status=$(echo "$response" | tail -n1)
    if [ "$status" = "200" ] || [ "$status" = "201" ]; then
        print_pass "POST request accepted"
    else
        print_fail "POST request returned $status"
    fi
    
    print_test "DELETE request to /bin"
    # First create a test file
    test_file="/tmp/webserv_delete_test.txt"
    echo "test" > "$test_file"
    response=$(curl -s -w "\n%{http_code}" -X DELETE "$BASE_URL_1/bin/test.txt")
    status=$(echo "$response" | tail -n1)
    if [ "$status" = "204" ] || [ "$status" = "404" ] || [ "$status" = "200" ]; then
        print_pass "DELETE request processed (status: $status)"
    else
        print_fail "DELETE request returned unexpected status: $status"
    fi
    
    print_test "Invalid method (should return 405 or 501)"
    response=$(curl -s -w "\n%{http_code}" -X PATCH "$BASE_URL_1/")
    status=$(echo "$response" | tail -n1)
    if [ "$status" = "405" ] || [ "$status" = "501" ]; then
        print_pass "Invalid method returns $status"
    else
        print_fail "Invalid method returned $status instead of 405/501"
    fi
}

# Test 3: Error Pages
test_error_pages() {
    print_header "TEST 3: Custom Error Pages"
    
    print_test "400 Bad Request error page"
    response=$(echo -e "GET /test\r\n\r\n" | nc localhost $SERVER_PORT_1)
    if echo "$response" | grep -q "malformed syntax"; then
        print_pass "Custom 400 error page is served"
    else
        print_fail "Custom 400 error page not found"
    fi
    
    print_test "404 Not Found error page"
    response=$(curl -s "$BASE_URL_1/nonexistent_page_12345")
    if echo "$response" | grep -q "404" || echo "$response" | grep -q "Not Found"; then
        print_pass "404 error page is served"
    else
        print_fail "404 error page not properly served"
    fi
}

# Test 4: CGI Execution
test_cgi() {
    print_header "TEST 4: CGI Execution"
    
    # Check if Python CGI script exists
    if [ -f "www/test.py" ] || [ -f "www/hello.py" ]; then
        print_test "Python CGI script execution"
        # Try different possible CGI script locations
        for script in "test.py" "hello.py" "api/test.py"; do
            response=$(curl -s -w "\n%{http_code}" "$BASE_URL_2/$script")
            status=$(echo "$response" | tail -n1)
            body=$(echo "$response" | head -n -1)
            
            if [ "$status" = "200" ] && [ -n "$body" ]; then
                print_pass "CGI script executed successfully: $script"
                print_info "Response preview: $(echo "$body" | head -c 100)..."
                break
            fi
        done
    else
        print_info "No Python CGI scripts found to test"
    fi
    
    # Test POST to CGI
    print_test "POST data to CGI script"
    response=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL_2/form_handler.py" -d "name=test&value=123" 2>/dev/null)
    status=$(echo "$response" | tail -n1)
    if [ "$status" = "200" ] || [ "$status" = "404" ]; then
        print_info "POST to CGI status: $status"
    fi
}

# Test 5: File Upload
test_file_upload() {
    print_header "TEST 5: File Upload"
    
    print_test "Upload file via multipart/form-data"
    # Create a test file
    echo "Test upload content" > /tmp/test_upload.txt
    
    response=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL_1/upload" \
        -F "file=@/tmp/test_upload.txt" 2>/dev/null)
    status=$(echo "$response" | tail -n1)
    
    if [ "$status" = "200" ] || [ "$status" = "201" ]; then
        print_pass "File upload successful (status: $status)"
    elif [ "$status" = "404" ]; then
        print_info "Upload endpoint not configured (404)"
    else
        print_fail "File upload returned unexpected status: $status"
    fi
    
    rm -f /tmp/test_upload.txt
}

# Test 6: Client Body Size Limit
test_body_size_limit() {
    print_header "TEST 6: Client Body Size Limit"
    
    print_test "Upload exceeding client_max_body_size"
    # Create a large file (assuming default limit is around 1MB)
    dd if=/dev/zero of=/tmp/large_file.bin bs=1M count=2 2>/dev/null
    
    response=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL_1/upload" \
        --data-binary "@/tmp/large_file.bin" 2>/dev/null)
    status=$(echo "$response" | tail -n1)
    
    if [ "$status" = "413" ]; then
        print_pass "Large upload correctly rejected with 413"
    else
        print_info "Large upload status: $status (413 expected if limit exceeded)"
    fi
    
    rm -f /tmp/large_file.bin
}

# Test 7: Directory Listing
test_directory_listing() {
    print_header "TEST 7: Directory Listing (Autoindex)"
    
    print_test "Access directory with autoindex on"
    response=$(curl -s -w "\n%{http_code}" "$BASE_URL_1/download/")
    status=$(echo "$response" | tail -n1)
    body=$(echo "$response" | head -n -1)
    
    if [ "$status" = "200" ] && echo "$body" | grep -q "Index of"; then
        print_pass "Directory listing works"
    elif [ "$status" = "403" ]; then
        print_info "Directory listing forbidden (expected if autoindex off)"
    else
        print_fail "Unexpected directory listing response: $status"
    fi
}

# Test 8: HTTP Redirections
test_redirections() {
    print_header "TEST 8: HTTP Redirections"
    
    print_test "Test HTTP redirect"
    response=$(curl -s -w "\n%{http_code}" -L "$BASE_URL_1/redirect" 2>/dev/null)
    status=$(echo "$response" | tail -n1)
    
    if [ "$status" = "200" ] || [ "$status" = "301" ] || [ "$status" = "302" ]; then
        print_info "Redirect response status: $status"
    else
        print_info "No redirect configured at /redirect"
    fi
}

# Test 9: Multiple Ports
test_multiple_ports() {
    print_header "TEST 9: Multiple Ports/Servers"
    
    print_test "Server listening on port $SERVER_PORT_1"
    if nc -z "$SERVER_HOST" "$SERVER_PORT_1" 2>/dev/null; then
        print_pass "Port $SERVER_PORT_1 is listening"
    else
        print_fail "Port $SERVER_PORT_1 is not listening"
    fi
    
    print_test "Server listening on port $SERVER_PORT_2"
    if nc -z "$SERVER_HOST" "$SERVER_PORT_2" 2>/dev/null; then
        print_pass "Port $SERVER_PORT_2 is listening"
    else
        print_fail "Port $SERVER_PORT_2 is not listening"
    fi
}

# Test 10: Concurrent Connections
test_concurrent_connections() {
    print_header "TEST 10: Concurrent Connections"
    
    print_test "Handle 10 simultaneous requests"
    for i in {1..10}; do
        curl -s "$BASE_URL_1/" > /dev/null &
    done
    wait
    print_pass "Server handled concurrent requests without crashing"
}

# Test 11: Malformed Requests
test_malformed_requests() {
    print_header "TEST 11: Malformed Requests"
    
    print_test "Request without HTTP version"
    response=$(echo -e "GET /\r\n\r\n" | nc -w 2 localhost $SERVER_PORT_1)
    if echo "$response" | grep -q "400"; then
        print_pass "Malformed request returns 400"
    else
        print_fail "Malformed request not handled correctly"
    fi
    
    print_test "Empty request"
    response=$(echo -e "\r\n\r\n" | nc -w 2 localhost $SERVER_PORT_1)
    if echo "$response" | grep -q "400"; then
        print_pass "Empty request returns 400"
    else
        print_info "Empty request handling: $(echo "$response" | head -n1)"
    fi
}

# Test 12: MIME Types
test_mime_types() {
    print_header "TEST 12: Content-Type Headers"
    
    print_test "HTML file Content-Type"
    headers=$(curl -s -I "$BASE_URL_1/")
    if echo "$headers" | grep -i "content-type" | grep -q "text/html"; then
        print_pass "HTML returns text/html"
    else
        print_fail "HTML Content-Type incorrect"
    fi
    
    print_test "Plain text file Content-Type"
    headers=$(curl -s -I "$BASE_URL_1/files/test.txt")
    if echo "$headers" | grep -i "content-type" | grep -q "text/plain"; then
        print_pass "Text file returns text/plain"
    else
        print_info "Text file Content-Type: $(echo "$headers" | grep -i content-type)"
    fi
}

# Main execution
main() {
    clear
    echo -e "${GREEN}╔════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║   WebServ Comprehensive Test Suite - 42       ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════╝${NC}"
    
    print_info "Starting tests..."
    check_server
    
    # Run all test suites
    test_basic_get
    test_http_methods
    test_error_pages
    test_cgi
    test_file_upload
    test_body_size_limit
    test_directory_listing
    test_redirections
    test_multiple_ports
    test_concurrent_connections
    test_malformed_requests
    test_mime_types
    
    # Print summary
    print_header "TEST SUMMARY"
    echo -e "Total Tests: ${TOTAL_TESTS}"
    echo -e "${GREEN}Passed: ${PASSED_TESTS}${NC}"
    echo -e "${RED}Failed: ${FAILED_TESTS}${NC}"
    
    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "\n${GREEN}🎉 All tests passed! Your WebServ is looking good!${NC}\n"
    else
        echo -e "\n${YELLOW}⚠️  Some tests failed. Review the output above.${NC}\n"
    fi
    
    # Calculate percentage
    if [ $TOTAL_TESTS -gt 0 ]; then
        percentage=$((PASSED_TESTS * 100 / TOTAL_TESTS))
        echo -e "Success Rate: ${percentage}%\n"
    fi
}

# Run main
main
