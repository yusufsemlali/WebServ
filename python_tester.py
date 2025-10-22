#!/usr/bin/env python3
"""
Comprehensive Python Testing Script for WebServ
Tests all mandatory features from 42 subject
"""

import requests
import time
import sys
import os
from concurrent.futures import ThreadPoolExecutor, as_completed
from typing import Tuple, List

# ANSI Colors
GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
RESET = '\033[0m'

# Configuration
BASE_URL_1 = "http://localhost:1024"
BASE_URL_2 = "http://localhost:1025"

class WebServTester:
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.total = 0
        
    def print_header(self, text: str):
        print(f"\n{BLUE}{'='*50}{RESET}")
        print(f"{BLUE}{text}{RESET}")
        print(f"{BLUE}{'='*50}{RESET}\n")
    
    def test(self, name: str) -> None:
        """Register a test"""
        print(f"{YELLOW}[TEST]{RESET} {name}")
        self.total += 1
    
    def passed_test(self, msg: str) -> None:
        """Mark test as passed"""
        print(f"{GREEN}[PASS]{RESET} {msg}")
        self.passed += 1
    
    def failed_test(self, msg: str) -> None:
        """Mark test as failed"""
        print(f"{RED}[FAIL]{RESET} {msg}")
        self.failed += 1
    
    def info(self, msg: str) -> None:
        """Print info message"""
        print(f"{BLUE}[INFO]{RESET} {msg}")
    
    def check_server(self) -> bool:
        """Check if server is running"""
        try:
            response = requests.get(BASE_URL_1, timeout=2)
            self.info("Server is running")
            return True
        except requests.exceptions.RequestException:
            print(f"{RED}ERROR: Server not running on port 1024{RESET}")
            print("Start server with: ./webserv configs/default.conf")
            return False
    
    def test_basic_get_requests(self):
        """Test basic GET functionality"""
        self.print_header("TEST 1: Basic GET Requests")
        
        # Test 1.1: Index page
        self.test("GET / (index page)")
        try:
            response = requests.get(f"{BASE_URL_1}/", timeout=5)
            if response.status_code == 200:
                self.passed_test(f"Index page returns 200 OK ({len(response.content)} bytes)")
            else:
                self.failed_test(f"Expected 200, got {response.status_code}")
        except Exception as e:
            self.failed_test(f"Exception: {str(e)}")
        
        # Test 1.2: Static file
        self.test("GET /files/test.txt")
        try:
            response = requests.get(f"{BASE_URL_1}/files/test.txt", timeout=5)
            if response.status_code == 200:
                self.passed_test("Static file returns 200 OK")
            else:
                self.failed_test(f"Expected 200, got {response.status_code}")
        except Exception as e:
            self.failed_test(f"Exception: {str(e)}")
        
        # Test 1.3: 404 Not Found
        self.test("GET /nonexistent (404 test)")
        try:
            response = requests.get(f"{BASE_URL_1}/nonexistent_page_xyz", timeout=5)
            if response.status_code == 404:
                self.passed_test("404 Not Found works correctly")
            else:
                self.failed_test(f"Expected 404, got {response.status_code}")
        except Exception as e:
            self.failed_test(f"Exception: {str(e)}")
    
    def test_http_methods(self):
        """Test different HTTP methods"""
        self.print_header("TEST 2: HTTP Methods")
        
        # Test 2.1: POST
        self.test("POST request with form data")
        try:
            response = requests.post(f"{BASE_URL_1}/", 
                                   data={"name": "test", "value": "123"}, 
                                   timeout=5)
            if response.status_code in [200, 201, 204]:
                self.passed_test(f"POST accepted (status: {response.status_code})")
            else:
                self.failed_test(f"Unexpected status: {response.status_code}")
        except Exception as e:
            self.failed_test(f"Exception: {str(e)}")
        
        # Test 2.2: DELETE
        self.test("DELETE request")
        try:
            response = requests.delete(f"{BASE_URL_1}/bin/testfile.txt", timeout=5)
            if response.status_code in [200, 204, 404]:
                self.passed_test(f"DELETE processed (status: {response.status_code})")
            else:
                self.info(f"DELETE returned: {response.status_code}")
        except Exception as e:
            self.failed_test(f"Exception: {str(e)}")
        
        # Test 2.3: Invalid method
        self.test("Invalid method (should return 405 or 501)")
        try:
            response = requests.request('PATCH', f"{BASE_URL_1}/", timeout=5)
            if response.status_code in [405, 501]:
                self.passed_test(f"Invalid method returns {response.status_code}")
            else:
                self.failed_test(f"Expected 405/501, got {response.status_code}")
        except Exception as e:
            self.failed_test(f"Exception: {str(e)}")
    
    def test_error_pages(self):
        """Test custom error pages"""
        self.print_header("TEST 3: Custom Error Pages")
        
        # Test 3.1: 404 error page
        self.test("404 error page content")
        try:
            response = requests.get(f"{BASE_URL_1}/nonexistent", timeout=5)
            if response.status_code == 404:
                if len(response.text) > 100:  # Should be custom HTML page
                    self.passed_test("Custom 404 page is served")
                else:
                    self.info("404 page seems minimal")
            else:
                self.failed_test(f"Expected 404, got {response.status_code}")
        except Exception as e:
            self.failed_test(f"Exception: {str(e)}")
    
    def test_file_upload(self):
        """Test file upload functionality"""
        self.print_header("TEST 4: File Upload")
        
        self.test("Upload file via multipart/form-data")
        try:
            # Create test file
            test_content = b"Test upload content from Python tester"
            files = {'file': ('test_upload.txt', test_content, 'text/plain')}
            
            response = requests.post(f"{BASE_URL_1}/upload", 
                                   files=files, 
                                   timeout=10)
            
            if response.status_code in [200, 201]:
                self.passed_test(f"File upload successful (status: {response.status_code})")
            elif response.status_code == 404:
                self.info("Upload endpoint not configured (404)")
            else:
                self.info(f"Upload returned status: {response.status_code}")
        except Exception as e:
            self.failed_test(f"Exception: {str(e)}")
    
    def test_body_size_limit(self):
        """Test client max body size enforcement"""
        self.print_header("TEST 5: Client Body Size Limit")
        
        self.test("Upload exceeding size limit")
        try:
            # Create 2MB payload
            large_data = b"x" * (2 * 1024 * 1024)
            
            response = requests.post(f"{BASE_URL_1}/upload", 
                                   data=large_data, 
                                   timeout=10)
            
            if response.status_code == 413:
                self.passed_test("Large upload correctly rejected with 413")
            else:
                self.info(f"Large upload status: {response.status_code} (413 expected if limit set)")
        except Exception as e:
            self.info(f"Upload test: {str(e)}")
    
    def test_mime_types(self):
        """Test Content-Type headers"""
        self.print_header("TEST 6: MIME Types / Content-Type")
        
        # Test HTML
        self.test("HTML Content-Type")
        try:
            response = requests.head(f"{BASE_URL_1}/", timeout=5)
            content_type = response.headers.get('Content-Type', '')
            if 'text/html' in content_type:
                self.passed_test("HTML returns text/html")
            else:
                self.failed_test(f"HTML returned: {content_type}")
        except Exception as e:
            self.failed_test(f"Exception: {str(e)}")
        
        # Test plain text
        self.test("Plain text Content-Type")
        try:
            response = requests.head(f"{BASE_URL_1}/files/test.txt", timeout=5)
            content_type = response.headers.get('Content-Type', '')
            if 'text/plain' in content_type:
                self.passed_test("Text file returns text/plain")
            else:
                self.info(f"Text file returned: {content_type}")
        except Exception as e:
            self.info(f"Text file test: {str(e)}")
    
    def test_concurrent_connections(self):
        """Test concurrent request handling"""
        self.print_header("TEST 7: Concurrent Connections")
        
        self.test("Handle 50 concurrent requests")
        try:
            start_time = time.time()
            
            def make_request(n):
                response = requests.get(f"{BASE_URL_1}/", timeout=10)
                return response.status_code == 200
            
            with ThreadPoolExecutor(max_workers=50) as executor:
                futures = [executor.submit(make_request, i) for i in range(50)]
                results = [future.result() for future in as_completed(futures)]
            
            duration = time.time() - start_time
            success_count = sum(results)
            
            if success_count == 50:
                self.passed_test(f"All 50 requests succeeded in {duration:.2f}s")
            else:
                self.failed_test(f"Only {success_count}/50 requests succeeded")
        except Exception as e:
            self.failed_test(f"Exception: {str(e)}")
    
    def test_multiple_ports(self):
        """Test multiple server ports"""
        self.print_header("TEST 8: Multiple Ports/Servers")
        
        # Test port 1024
        self.test("Server on port 1024")
        try:
            response = requests.get(BASE_URL_1, timeout=5)
            if response.status_code == 200:
                self.passed_test("Port 1024 responds")
            else:
                self.failed_test(f"Port 1024 returned {response.status_code}")
        except Exception as e:
            self.failed_test(f"Port 1024: {str(e)}")
        
        # Test port 1025
        self.test("Server on port 1025")
        try:
            response = requests.get(BASE_URL_2, timeout=5)
            if response.status_code == 200:
                self.passed_test("Port 1025 responds")
            else:
                self.failed_test(f"Port 1025 returned {response.status_code}")
        except Exception as e:
            self.failed_test(f"Port 1025: {str(e)}")
    
    def test_cgi(self):
        """Test CGI execution"""
        self.print_header("TEST 9: CGI Execution")
        
        # Check for Python CGI scripts
        cgi_scripts = [
            f"{BASE_URL_2}/hello.py",
            f"{BASE_URL_2}/test.py",
            f"{BASE_URL_2}/form_handler.py"
        ]
        
        found_working_cgi = False
        
        for script_url in cgi_scripts:
            script_name = script_url.split('/')[-1]
            self.test(f"CGI script: {script_name}")
            try:
                response = requests.get(script_url, timeout=10)
                if response.status_code == 200 and len(response.content) > 0:
                    self.passed_test(f"CGI {script_name} executed successfully")
                    found_working_cgi = True
                    break
                elif response.status_code == 404:
                    self.info(f"{script_name} not found")
                else:
                    self.info(f"{script_name} returned {response.status_code}")
            except Exception as e:
                self.info(f"{script_name}: {str(e)}")
        
        if not found_working_cgi:
            self.info("No working CGI scripts found. Create hello.py in www/")
    
    def print_summary(self):
        """Print test summary"""
        self.print_header("TEST SUMMARY")
        
        print(f"Total Tests: {self.total}")
        print(f"{GREEN}Passed: {self.passed}{RESET}")
        print(f"{RED}Failed: {self.failed}{RESET}")
        
        if self.total > 0:
            percentage = (self.passed / self.total) * 100
            print(f"\nSuccess Rate: {percentage:.1f}%")
        
        if self.failed == 0:
            print(f"\n{GREEN}🎉 All tests passed! Your WebServ looks good!{RESET}\n")
        else:
            print(f"\n{YELLOW}⚠️  Some tests failed. Review the output above.{RESET}\n")
    
    def run_all_tests(self):
        """Run all test suites"""
        if not self.check_server():
            return False
        
        try:
            self.test_basic_get_requests()
            self.test_http_methods()
            self.test_error_pages()
            self.test_file_upload()
            self.test_body_size_limit()
            self.test_mime_types()
            self.test_concurrent_connections()
            self.test_multiple_ports()
            self.test_cgi()
            
            self.print_summary()
            return self.failed == 0
        except KeyboardInterrupt:
            print(f"\n{YELLOW}Tests interrupted by user{RESET}")
            return False

def main():
    print(f"{GREEN}╔{'='*48}╗{RESET}")
    print(f"{GREEN}║  WebServ Python Test Suite - 42 Project       ║{RESET}")
    print(f"{GREEN}╚{'='*48}╝{RESET}\n")
    
    tester = WebServTester()
    success = tester.run_all_tests()
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
