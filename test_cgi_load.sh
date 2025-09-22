#!/bin/bash

# CGI Load Testing Script
# Tests non-blocking CGI with multiple concurrent requests

SERVER_URL="http://localhost:1024"
CGI_SCRIPT="/slow.py"
NUM_REQUESTS=200
CONCURRENT_LIMIT=50  # Max concurrent requests at once
LOG_FILE="cgi_test_results.log"
TEMP_DIR="/tmp/cgi_test_$$"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}===========================================${NC}"
echo -e "${BLUE}      NON-BLOCKING CGI LOAD TESTING       ${NC}"
echo -e "${BLUE}===========================================${NC}"
echo -e "Server URL: ${SERVER_URL}${CGI_SCRIPT}"
echo -e "Total requests: ${NUM_REQUESTS}"
echo -e "Concurrent limit: ${CONCURRENT_LIMIT}"
echo -e "Log file: ${LOG_FILE}"
echo ""

# Create temp directory for results
mkdir -p "$TEMP_DIR"

# Clear previous log
> "$LOG_FILE"

echo -e "${YELLOW}Starting load test...${NC}"
START_TIME=$(date +%s.%N)

# Function to make a single request
make_request() {
    local request_id=$1
    local start_time=$(date +%s.%N)
    
    # Make the request and capture response
    local response=$(curl -s -w "HTTPCODE:%{http_code}|TIME:%{time_total}" \
                          --max-time 30 \
                          "${SERVER_URL}${CGI_SCRIPT}" 2>/dev/null)
    
    local end_time=$(date +%s.%N)
    local duration=$(echo "$end_time - $start_time" | bc -l)
    
    # Parse response
    local http_code=$(echo "$response" | grep -o "HTTPCODE:[0-9]*" | cut -d: -f2)
    local curl_time=$(echo "$response" | grep -o "TIME:[0-9.]*" | cut -d: -f2)
    local body=$(echo "$response" | sed 's/HTTPCODE:[0-9]*|TIME:[0-9.]*//')
    
    # Determine status
    local status="SUCCESS"
    if [[ "$http_code" != "200" ]] || [[ -z "$http_code" ]]; then
        status="FAILED"
    fi
    
    # Log result
    echo "[$request_id] $status HTTP:$http_code Duration:${duration}s CurlTime:${curl_time}s" >> "$TEMP_DIR/request_$request_id.log"
    
    # Progress indicator
    if [[ $(($request_id % 10)) -eq 0 ]]; then
        echo -e "${GREEN}Request $request_id completed ($status)${NC}"
    fi
}

# Export function so it can be used with xargs
export -f make_request
export SERVER_URL CGI_SCRIPT TEMP_DIR

# Generate request IDs and run them with limited concurrency
seq 1 $NUM_REQUESTS | xargs -n 1 -P $CONCURRENT_LIMIT -I {} bash -c 'make_request {}'

END_TIME=$(date +%s.%N)
TOTAL_DURATION=$(echo "$END_TIME - $START_TIME" | bc -l)

echo ""
echo -e "${BLUE}===========================================${NC}"
echo -e "${BLUE}           TEST RESULTS SUMMARY           ${NC}"
echo -e "${BLUE}===========================================${NC}"

# Collect and analyze results
SUCCESS_COUNT=0
FAILED_COUNT=0
TOTAL_TIME=0
MIN_TIME=999999
MAX_TIME=0

echo "Collecting results..." > "$LOG_FILE"
echo "Test started at: $(date -d @${START_TIME%.*})" >> "$LOG_FILE"
echo "----------------------------------------" >> "$LOG_FILE"

for i in $(seq 1 $NUM_REQUESTS); do
    if [[ -f "$TEMP_DIR/request_$i.log" ]]; then
        result=$(cat "$TEMP_DIR/request_$i.log")
        echo "$result" >> "$LOG_FILE"
        
        if echo "$result" | grep -q "SUCCESS"; then
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        else
            FAILED_COUNT=$((FAILED_COUNT + 1))
        fi
        
        # Extract duration for statistics
        duration=$(echo "$result" | grep -o "Duration:[0-9.]*" | cut -d: -f2)
        if [[ -n "$duration" ]]; then
            TOTAL_TIME=$(echo "$TOTAL_TIME + $duration" | bc -l)
            
            if (( $(echo "$duration < $MIN_TIME" | bc -l) )); then
                MIN_TIME=$duration
            fi
            
            if (( $(echo "$duration > $MAX_TIME" | bc -l) )); then
                MAX_TIME=$duration
            fi
        fi
    else
        echo "[ERROR] Request $i: No result file found" >> "$LOG_FILE"
        FAILED_COUNT=$((FAILED_COUNT + 1))
    fi
done

# Calculate averages
if [[ $SUCCESS_COUNT -gt 0 ]]; then
    AVERAGE_TIME=$(echo "scale=3; $TOTAL_TIME / $SUCCESS_COUNT" | bc -l)
else
    AVERAGE_TIME=0
fi

REQUESTS_PER_SECOND=$(echo "scale=2; $NUM_REQUESTS / $TOTAL_DURATION" | bc -l)

echo "" >> "$LOG_FILE"
echo "========================================" >> "$LOG_FILE"
echo "FINAL STATISTICS:" >> "$LOG_FILE"
echo "========================================" >> "$LOG_FILE"
echo "Total requests: $NUM_REQUESTS" >> "$LOG_FILE"
echo "Successful: $SUCCESS_COUNT" >> "$LOG_FILE"
echo "Failed: $FAILED_COUNT" >> "$LOG_FILE"
echo "Success rate: $(echo "scale=1; $SUCCESS_COUNT * 100 / $NUM_REQUESTS" | bc -l)%" >> "$LOG_FILE"
echo "Total test duration: ${TOTAL_DURATION}s" >> "$LOG_FILE"
echo "Average request time: ${AVERAGE_TIME}s" >> "$LOG_FILE"
echo "Minimum request time: ${MIN_TIME}s" >> "$LOG_FILE"
echo "Maximum request time: ${MAX_TIME}s" >> "$LOG_FILE"
echo "Requests per second: ${REQUESTS_PER_SECOND}" >> "$LOG_FILE"
echo "Test completed at: $(date)" >> "$LOG_FILE"

# Display results
cat "$LOG_FILE" | tail -15
echo ""

if [[ $SUCCESS_COUNT -eq $NUM_REQUESTS ]]; then
    echo -e "${GREEN}✅ ALL REQUESTS SUCCESSFUL!${NC}"
    echo -e "${GREEN}Your non-blocking CGI implementation is working perfectly!${NC}"
elif [[ $SUCCESS_COUNT -gt $((NUM_REQUESTS * 8 / 10)) ]]; then
    echo -e "${YELLOW}⚠️  MOSTLY SUCCESSFUL (${SUCCESS_COUNT}/${NUM_REQUESTS})${NC}"
    echo -e "${YELLOW}Some requests failed, but the server handled most of them${NC}"
else
    echo -e "${RED}❌ MANY FAILURES (${SUCCESS_COUNT}/${NUM_REQUESTS})${NC}"
    echo -e "${RED}The server may be struggling with the load${NC}"
fi

echo ""
echo -e "${BLUE}Key Performance Indicators:${NC}"
echo -e "📊 Requests/second: ${REQUESTS_PER_SECOND} (higher is better)"
echo -e "⏱️  Average response: ${AVERAGE_TIME}s (should be ~5s + overhead)"
echo -e "🎯 Success rate: $(echo "scale=1; $SUCCESS_COUNT * 100 / $NUM_REQUESTS" | bc -l)%"

echo ""
echo -e "${BLUE}If this was a blocking implementation:${NC}"
echo -e "⏳ Expected duration: $((NUM_REQUESTS * 5))s (${NUM_REQUESTS} × 5s sleep)"
echo -e "🚀 Actual duration: ${TOTAL_DURATION}s"
echo -e "💪 Performance gain: $(echo "scale=1; ($NUM_REQUESTS * 5) / $TOTAL_DURATION" | bc -l)x faster!"

# Cleanup
rm -rf "$TEMP_DIR"

echo ""
echo -e "${GREEN}Full detailed results saved to: ${LOG_FILE}${NC}"
echo -e "${BLUE}===========================================${NC}"