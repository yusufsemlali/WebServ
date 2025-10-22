#!/bin/bash

# Stress Test for WebServ
# Tests server stability under load

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

SERVER_HOST="localhost"
SERVER_PORT="1024"
BASE_URL="http://${SERVER_HOST}:${SERVER_PORT}"

print_header() {
    echo -e "\n${YELLOW}=== $1 ===${NC}\n"
}

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Test 1: Rapid concurrent connections
test_concurrent_load() {
    print_header "Test 1: Concurrent Connections (100 requests)"
    
    print_info "Sending 100 concurrent GET requests..."
    start_time=$(date +%s)
    
    for i in {1..100}; do
        curl -s "$BASE_URL/" > /dev/null &
    done
    
    wait
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    
    print_info "Completed in ${duration} seconds"
    print_info "Server still responding..."
    
    response=$(curl -s -w "%{http_code}" "$BASE_URL/" -o /dev/null)
    if [ "$response" = "200" ]; then
        echo -e "${GREEN}✓ Server survived concurrent load test${NC}"
    else
        echo -e "${RED}✗ Server may have issues (got status: $response)${NC}"
    fi
}

# Test 2: Rapid connect/disconnect
test_rapid_connections() {
    print_header "Test 2: Rapid Connect/Disconnect (50 times)"
    
    print_info "Rapidly opening and closing connections..."
    
    for i in {1..50}; do
        (echo -e "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc -w 1 localhost $SERVER_PORT) > /dev/null 2>&1 &
    done
    
    wait
    sleep 1
    
    response=$(curl -s -w "%{http_code}" "$BASE_URL/" -o /dev/null)
    if [ "$response" = "200" ]; then
        echo -e "${GREEN}✓ Server survived rapid connections${NC}"
    else
        echo -e "${RED}✗ Server may have crashed${NC}"
    fi
}

# Test 3: Large payload stress
test_large_payloads() {
    print_header "Test 3: Large Payload Stress"
    
    print_info "Sending 10 large POST requests..."
    
    for i in {1..10}; do
        dd if=/dev/zero bs=1K count=500 2>/dev/null | \
        curl -s -X POST "$BASE_URL/upload" \
             --data-binary @- > /dev/null &
    done
    
    wait
    
    response=$(curl -s -w "%{http_code}" "$BASE_URL/" -o /dev/null)
    if [ "$response" = "200" ]; then
        echo -e "${GREEN}✓ Server survived large payload stress${NC}"
    else
        echo -e "${RED}✗ Server may have issues${NC}"
    fi
}

# Test 4: Slow client simulation
test_slow_client() {
    print_header "Test 4: Slow Client Simulation"
    
    print_info "Simulating slow client (partial request)..."
    
    # Send request slowly
    (
        echo -n "GET / HTTP/1.1\r\n"
        sleep 2
        echo -n "Host: localhost\r\n"
        sleep 2
        echo -e "\r\n"
    ) | nc -w 10 localhost $SERVER_PORT > /dev/null 2>&1 &
    
    # Meanwhile, test if server still responds to other clients
    sleep 1
    response=$(curl -s -w "%{http_code}" "$BASE_URL/" -o /dev/null)
    
    wait
    
    if [ "$response" = "200" ]; then
        echo -e "${GREEN}✓ Server handles slow clients without blocking${NC}"
    else
        echo -e "${RED}✗ Server may be blocking on slow clients${NC}"
    fi
}

# Test 5: Mixed load test
test_mixed_load() {
    print_header "Test 5: Mixed Load (GET, POST, DELETE)"
    
    print_info "Sending mixed requests..."
    
    for i in {1..30}; do
        curl -s "$BASE_URL/" > /dev/null &
        curl -s -X POST "$BASE_URL/" -d "data=test" > /dev/null &
        curl -s -X DELETE "$BASE_URL/test" > /dev/null &
    done
    
    wait
    
    response=$(curl -s -w "%{http_code}" "$BASE_URL/" -o /dev/null)
    if [ "$response" = "200" ]; then
        echo -e "${GREEN}✓ Server survived mixed load${NC}"
    else
        echo -e "${RED}✗ Server may have issues${NC}"
    fi
}

# Test 6: Memory leak detection (basic)
test_memory_usage() {
    print_header "Test 6: Memory Usage Check"
    
    if ! command -v pgrep &> /dev/null; then
        print_info "pgrep not available, skipping memory test"
        return
    fi
    
    pid=$(pgrep -f "./webserv" | head -n1)
    
    if [ -z "$pid" ]; then
        print_error "Could not find webserv process"
        return
    fi
    
    print_info "Webserv PID: $pid"
    
    # Get initial memory
    mem_before=$(ps -o rss= -p $pid 2>/dev/null)
    print_info "Memory before: ${mem_before} KB"
    
    # Send many requests
    print_info "Sending 200 requests..."
    for i in {1..200}; do
        curl -s "$BASE_URL/" > /dev/null &
    done
    wait
    
    sleep 2
    
    # Get memory after
    mem_after=$(ps -o rss= -p $pid 2>/dev/null)
    print_info "Memory after: ${mem_after} KB"
    
    if [ -n "$mem_before" ] && [ -n "$mem_after" ]; then
        mem_diff=$((mem_after - mem_before))
        print_info "Memory difference: ${mem_diff} KB"
        
        if [ $mem_diff -lt 50000 ]; then
            echo -e "${GREEN}✓ Memory usage looks reasonable${NC}"
        else
            echo -e "${YELLOW}⚠ Memory increased significantly (possible leak?)${NC}"
        fi
    fi
}

# Main execution
main() {
    clear
    echo -e "${GREEN}╔════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║   WebServ Stress Test Suite           ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════╝${NC}"
    
    # Check if server is running
    if ! nc -z localhost $SERVER_PORT 2>/dev/null; then
        print_error "Server not running on port $SERVER_PORT"
        echo "Start server with: ./webserv configs/default.conf"
        exit 1
    fi
    
    print_info "Server is running, starting stress tests..."
    echo -e "${YELLOW}Note: These tests will stress your server. Monitor for crashes.${NC}\n"
    
    sleep 2
    
    test_concurrent_load
    test_rapid_connections
    test_large_payloads
    test_slow_client
    test_mixed_load
    test_memory_usage
    
    print_header "Stress Test Complete"
    
    # Final check
    response=$(curl -s -w "%{http_code}" "$BASE_URL/" -o /dev/null 2>/dev/null)
    if [ "$response" = "200" ]; then
        echo -e "${GREEN}🎉 Server is still responding normally!${NC}"
        echo -e "${GREEN}Your WebServ appears stable under stress.${NC}\n"
    else
        echo -e "${RED}⚠️  Server may have crashed or is not responding${NC}\n"
    fi
}

main
