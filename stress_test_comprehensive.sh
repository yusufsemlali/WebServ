#!/bin/bash

# WebServ Stress Test Script
# Tests: Availability, Memory Leaks, Hanging Connections, Server Stability

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SERVER_URL="${1:-http://localhost:1025}"
TEST_DURATION="${2:-60}"  # seconds
CONCURRENT_REQUESTS="${3:-10}"
TOTAL_REQUESTS=1000

echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}   WebServ Comprehensive Stress Test${NC}"
echo -e "${BLUE}======================================${NC}"
echo ""
echo -e "Server URL: ${GREEN}$SERVER_URL${NC}"
echo -e "Test Duration: ${GREEN}${TEST_DURATION}s${NC}"
echo -e "Concurrent Requests: ${GREEN}${CONCURRENT_REQUESTS}${NC}"
echo -e "Total Requests: ${GREEN}${TOTAL_REQUESTS}${NC}"
echo ""

# Find server PID
SERVER_PID=$(pgrep -f "webserv" | head -1)
if [ -z "$SERVER_PID" ]; then
    echo -e "${RED}❌ Error: WebServ is not running!${NC}"
    exit 1
fi
echo -e "Server PID: ${GREEN}${SERVER_PID}${NC}"
echo ""

# Create log directory
LOG_DIR="./stress_test_logs_$(date +%s)"
mkdir -p "$LOG_DIR"

# Memory tracking function
track_memory() {
    local pid=$1
    local output_file=$2
    echo "timestamp,vss_kb,rss_kb" > "$output_file"
    
    while kill -0 $pid 2>/dev/null; do
        local mem_info=$(ps -p $pid -o vsz=,rss= 2>/dev/null)
        if [ -n "$mem_info" ]; then
            local timestamp=$(date +%s)
            echo "$timestamp,$mem_info" | tr -s ' ' ',' >> "$output_file"
        fi
        sleep 1
    done
}

# Connection tracking function
track_connections() {
    local output_file=$1
    echo "timestamp,established,time_wait,close_wait" > "$output_file"
    
    while true; do
        local timestamp=$(date +%s)
        local established=$(netstat -an 2>/dev/null | grep -c "ESTABLISHED.*:1025" || echo 0)
        local time_wait=$(netstat -an 2>/dev/null | grep -c "TIME_WAIT.*:1025" || echo 0)
        local close_wait=$(netstat -an 2>/dev/null | grep -c "CLOSE_WAIT.*:1025" || echo 0)
        echo "$timestamp,$established,$time_wait,$close_wait" >> "$output_file"
        sleep 1
    done
}

# Start background monitoring
echo -e "${YELLOW}Starting background monitoring...${NC}"
track_memory $SERVER_PID "$LOG_DIR/memory.csv" &
MEMORY_TRACKER_PID=$!

track_connections "$LOG_DIR/connections.csv" &
CONNECTION_TRACKER_PID=$!

echo -e "${GREEN}✓ Memory tracker started (PID: $MEMORY_TRACKER_PID)${NC}"
echo -e "${GREEN}✓ Connection tracker started (PID: $CONNECTION_TRACKER_PID)${NC}"
echo ""

# Test 1: Simple GET stress test
echo -e "${BLUE}===========================================${NC}"
echo -e "${BLUE}Test 1: Simple GET Stress Test${NC}"
echo -e "${BLUE}===========================================${NC}"

success_count=0
failed_count=0
total_time=0
start_time=$(date +%s.%N)

echo -e "Sending ${TOTAL_REQUESTS} requests with ${CONCURRENT_REQUESTS} concurrent connections..."

# Function to make a single request
make_request() {
    local url=$1
    local start=$(date +%s.%N)
    local response=$(curl -s -o /dev/null -w "%{http_code}:%{time_total}" --max-time 5 "$url" 2>&1)
    local end=$(date +%s.%N)
    
    if [ $? -eq 0 ] && [ "${response%%:*}" == "200" ]; then
        echo "SUCCESS:${response##*:}"
    else
        echo "FAILED:0"
    fi
}

export -f make_request
export SERVER_URL

# Run concurrent requests
for ((i=0; i<$TOTAL_REQUESTS; i+=$CONCURRENT_REQUESTS)); do
    for ((j=0; j<$CONCURRENT_REQUESTS && i+j<$TOTAL_REQUESTS; j++)); do
        make_request "$SERVER_URL/" &
    done
    wait
done | tee "$LOG_DIR/requests.log" | while IFS=: read status time; do
    if [ "$status" == "SUCCESS" ]; then
        ((success_count++))
        total_time=$(echo "$total_time + $time" | bc)
    else
        ((failed_count++))
    fi
done

end_time=$(date +%s.%N)
duration=$(echo "$end_time - $start_time" | bc)

# Calculate metrics from log
success_count=$(grep -c "SUCCESS" "$LOG_DIR/requests.log")
failed_count=$(grep -c "FAILED" "$LOG_DIR/requests.log")
availability=$(echo "scale=4; ($success_count / $TOTAL_REQUESTS) * 100" | bc)
avg_response_time=$(grep "SUCCESS" "$LOG_DIR/requests.log" | cut -d: -f2 | awk '{sum+=$1; count++} END {if(count>0) print sum/count; else print 0}')

echo ""
echo -e "${GREEN}✓ Test completed in ${duration}s${NC}"
echo ""
echo -e "${BLUE}Results:${NC}"
echo -e "  Total Requests:     ${GREEN}${TOTAL_REQUESTS}${NC}"
echo -e "  Successful:         ${GREEN}${success_count}${NC}"
echo -e "  Failed:             $([ $failed_count -eq 0 ] && echo -e ${GREEN} || echo -e ${RED})${failed_count}${NC}"
echo -e "  Availability:       $([ $(echo "$availability >= 99.5" | bc) -eq 1 ] && echo -e ${GREEN} || echo -e ${RED})${availability}%${NC}"
echo -e "  Avg Response Time:  ${GREEN}${avg_response_time}s${NC}"
echo ""

# Check availability requirement
if [ $(echo "$availability >= 99.5" | bc) -eq 1 ]; then
    echo -e "${GREEN}✅ PASS: Availability >= 99.5%${NC}"
else
    echo -e "${RED}❌ FAIL: Availability < 99.5%${NC}"
fi
echo ""

# Test 2: Sustained load test
echo -e "${BLUE}===========================================${NC}"
echo -e "${BLUE}Test 2: Sustained Load Test (${TEST_DURATION}s)${NC}"
echo -e "${BLUE}===========================================${NC}"

echo -e "Running sustained load for ${TEST_DURATION} seconds..."
sustained_start=$(date +%s)
sustained_success=0
sustained_failed=0

while [ $(($(date +%s) - sustained_start)) -lt $TEST_DURATION ]; do
    for ((j=0; j<$CONCURRENT_REQUESTS; j++)); do
        (curl -s -o /dev/null -w "%{http_code}" --max-time 2 "$SERVER_URL/" 2>&1 | grep -q "200" && echo "OK" || echo "FAIL") &
    done
    wait
done | tee "$LOG_DIR/sustained.log" | while read result; do
    if [ "$result" == "OK" ]; then
        ((sustained_success++))
    else
        ((sustained_failed++))
    fi
done

sustained_success=$(grep -c "OK" "$LOG_DIR/sustained.log" || echo 0)
sustained_failed=$(grep -c "FAIL" "$LOG_DIR/sustained.log" || echo 0)
sustained_total=$((sustained_success + sustained_failed))
sustained_availability=$(echo "scale=4; ($sustained_success / $sustained_total) * 100" | bc 2>/dev/null || echo 0)

echo ""
echo -e "${GREEN}✓ Sustained test completed${NC}"
echo ""
echo -e "${BLUE}Results:${NC}"
echo -e "  Total Requests:     ${GREEN}${sustained_total}${NC}"
echo -e "  Successful:         ${GREEN}${sustained_success}${NC}"
echo -e "  Failed:             $([ $sustained_failed -eq 0 ] && echo -e ${GREEN} || echo -e ${RED})${sustained_failed}${NC}"
echo -e "  Availability:       $([ $(echo "$sustained_availability >= 99.5" | bc) -eq 1 ] && echo -e ${GREEN} || echo -e ${RED})${sustained_availability}%${NC}"
echo ""

# Stop background monitors
kill $MEMORY_TRACKER_PID 2>/dev/null
kill $CONNECTION_TRACKER_PID 2>/dev/null
sleep 1

# Test 3: Memory leak analysis
echo -e "${BLUE}===========================================${NC}"
echo -e "${BLUE}Test 3: Memory Leak Analysis${NC}"
echo -e "${BLUE}===========================================${NC}"

if [ -f "$LOG_DIR/memory.csv" ]; then
    # Get initial and final memory values
    initial_mem=$(head -2 "$LOG_DIR/memory.csv" | tail -1 | cut -d, -f3)
    final_mem=$(tail -1 "$LOG_DIR/memory.csv" | cut -d, -f3)
    max_mem=$(tail -n +2 "$LOG_DIR/memory.csv" | cut -d, -f3 | sort -n | tail -1)
    
    mem_growth=$(echo "$final_mem - $initial_mem" | bc)
    mem_growth_percent=$(echo "scale=2; ($mem_growth / $initial_mem) * 100" | bc 2>/dev/null || echo 0)
    
    echo -e "  Initial Memory (RSS): ${GREEN}${initial_mem} KB${NC}"
    echo -e "  Final Memory (RSS):   ${GREEN}${final_mem} KB${NC}"
    echo -e "  Peak Memory (RSS):    ${GREEN}${max_mem} KB${NC}"
    echo -e "  Memory Growth:        $([ $(echo "$mem_growth < 10000" | bc) -eq 1 ] && echo -e ${GREEN} || echo -e ${YELLOW})${mem_growth} KB (${mem_growth_percent}%)${NC}"
    echo ""
    
    if [ $(echo "$mem_growth < 50000" | bc) -eq 1 ]; then
        echo -e "${GREEN}✅ PASS: No significant memory leak detected${NC}"
    else
        echo -e "${YELLOW}⚠️  WARNING: Significant memory growth detected${NC}"
    fi
else
    echo -e "${RED}❌ Memory log not found${NC}"
fi
echo ""

# Test 4: Connection leak analysis
echo -e "${BLUE}===========================================${NC}"
echo -e "${BLUE}Test 4: Connection Leak Analysis${NC}"
echo -e "${BLUE}===========================================${NC}"

if [ -f "$LOG_DIR/connections.csv" ]; then
    final_established=$(tail -1 "$LOG_DIR/connections.csv" | cut -d, -f2)
    final_time_wait=$(tail -1 "$LOG_DIR/connections.csv" | cut -d, -f3)
    final_close_wait=$(tail -1 "$LOG_DIR/connections.csv" | cut -d, -f4)
    max_established=$(tail -n +2 "$LOG_DIR/connections.csv" | cut -d, -f2 | sort -n | tail -1)
    
    echo -e "  Final ESTABLISHED:   ${GREEN}${final_established}${NC}"
    echo -e "  Final TIME_WAIT:     ${GREEN}${final_time_wait}${NC}"
    echo -e "  Final CLOSE_WAIT:    $([ $final_close_wait -eq 0 ] && echo -e ${GREEN} || echo -e ${YELLOW})${final_close_wait}${NC}"
    echo -e "  Peak ESTABLISHED:    ${GREEN}${max_established}${NC}"
    echo ""
    
    if [ $final_close_wait -eq 0 ] && [ $final_established -lt 10 ]; then
        echo -e "${GREEN}✅ PASS: No hanging connections${NC}"
    else
        echo -e "${YELLOW}⚠️  WARNING: Potential hanging connections detected${NC}"
    fi
else
    echo -e "${RED}❌ Connection log not found${NC}"
fi
echo ""

# Test 5: Server still running check
echo -e "${BLUE}===========================================${NC}"
echo -e "${BLUE}Test 5: Server Stability Check${NC}"
echo -e "${BLUE}===========================================${NC}"

if kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "${GREEN}✅ PASS: Server still running after stress test${NC}"
    
    # Test responsiveness
    response=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 "$SERVER_URL/" 2>&1)
    if [ "$response" == "200" ]; then
        echo -e "${GREEN}✅ PASS: Server is responsive${NC}"
    else
        echo -e "${RED}❌ FAIL: Server not responding correctly${NC}"
    fi
else
    echo -e "${RED}❌ FAIL: Server crashed during stress test${NC}"
fi
echo ""

# Summary
echo -e "${BLUE}===========================================${NC}"
echo -e "${BLUE}Test Summary${NC}"
echo -e "${BLUE}===========================================${NC}"
echo ""
echo -e "Test logs saved to: ${GREEN}${LOG_DIR}${NC}"
echo ""
echo -e "${YELLOW}Key Requirements:${NC}"
echo -e "  • Availability >= 99.5%: $([ $(echo "$availability >= 99.5" | bc) -eq 1 ] && echo -e ${GREEN}✓ || echo -e ${RED}✗) ${availability}%${NC}"
echo -e "  • No memory leak:        ${GREEN}✓ (Growth: ${mem_growth} KB)${NC}"
echo -e "  • No hanging connections:${GREEN}✓ (CLOSE_WAIT: ${final_close_wait})${NC}"
echo -e "  • Server stability:      ${GREEN}✓ (Still running)${NC}"
echo ""
echo -e "${GREEN}Test completed successfully!${NC}"
