#!/bin/bash

# Configuration Validation Test Script
# Tests all error cases to ensure validation is working correctly

echo "=== Configuration Validation Test Suite ==="
echo

WEBSERV="./webserv"
ERROR_TESTS_DIR="configs/error_tests"
PASSED=0
FAILED=0

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to test a config file expecting an error
test_error_config() {
    local config_file="$1"
    local test_name="$2"
    
    echo -n "Testing ${test_name}... "
    
    # Run webserv with the config file and capture output
    if $WEBSERV "$config_file" > /dev/null 2>&1; then
        echo -e "${RED}FAIL${NC} (Expected error but config was accepted)"
        ((FAILED++))
        return 1
    else
        echo -e "${GREEN}PASS${NC} (Config correctly rejected)"
        ((PASSED++))
        return 0
    fi
}

# Function to test valid config
test_valid_config() {
    local config_file="$1"
    local test_name="$2"
    
    echo -n "Testing ${test_name}... "
    
    if $WEBSERV "$config_file" > /dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC} (Valid config accepted)"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}FAIL${NC} (Valid config rejected)"
        ((FAILED++))
        return 1
    fi
}

# Test valid configuration first
echo -e "${YELLOW}Testing Valid Configuration:${NC}"
test_valid_config "configs/test.conf" "Valid Configuration"
echo

# Test all error configurations
echo -e "${YELLOW}Testing Invalid Configurations:${NC}"

test_error_config "$ERROR_TESTS_DIR/01_invalid_methods.conf" "Invalid HTTP Methods"
test_error_config "$ERROR_TESTS_DIR/02_invalid_ports.conf" "Invalid Port Numbers"
test_error_config "$ERROR_TESTS_DIR/03_invalid_paths.conf" "Invalid File Paths"
test_error_config "$ERROR_TESTS_DIR/04_invalid_booleans.conf" "Invalid Boolean Values"
test_error_config "$ERROR_TESTS_DIR/05_invalid_error_codes.conf" "Invalid Error Codes"
test_error_config "$ERROR_TESTS_DIR/06_invalid_client_size.conf" "Invalid Client Size"
test_error_config "$ERROR_TESTS_DIR/07_invalid_urls.conf" "Invalid URLs"
test_error_config "$ERROR_TESTS_DIR/08_invalid_cgi_paths.conf" "Invalid CGI Paths"
test_error_config "$ERROR_TESTS_DIR/09_invalid_value_counts.conf" "Invalid Value Counts"
test_error_config "$ERROR_TESTS_DIR/10_invalid_hostnames.conf" "Invalid Hostnames"
test_error_config "$ERROR_TESTS_DIR/11_invalid_index_files.conf" "Invalid Index Files"
test_error_config "$ERROR_TESTS_DIR/12_invalid_error_page_format.conf" "Invalid Error Page Format"
test_error_config "$ERROR_TESTS_DIR/13_empty_config.conf" "Empty Configuration"
test_error_config "$ERROR_TESTS_DIR/14_invalid_server_names.conf" "Invalid Server Names"
test_error_config "$ERROR_TESTS_DIR/15_multiple_errors.conf" "Multiple Errors"

echo
echo "=== Test Results ==="
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
echo -e "Total:  $((PASSED + FAILED))"

if [ $FAILED -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed! Configuration validation is working correctly.${NC}"
    exit 0
else
    echo -e "\n${RED}Some tests failed. Please check the validation logic.${NC}"
    exit 1
fi
