#!/bin/bash

# Siege-like Benchmark Test for WebServ
# Simulates: siege -b -t30s -c10 http://localhost:1025/

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SERVER_URL="${1:-http://localhost:1025/}"
DURATION="${2:-30}"  # seconds
CONCURRENT="${3:-10}"

echo -e "${BLUE}╔══════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   Siege-like Benchmark Mode (-b)    ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════╝${NC}"
echo ""
echo "URL: $SERVER_URL"
echo "Duration: ${DURATION}s"
echo "Concurrent users: $CONCURRENT"
echo ""

# Find server PID
SERVER_PID=$(pgrep -f "webserv" | head -1)
if [ -z "$SERVER_PID" ]; then
    echo -e "${RED}Error: Server not running${NC}"
    exit 1
fi

INITIAL_MEM=$(ps -p $SERVER_PID -o rss= 2>/dev/null | tr -d ' ')
echo "Preparing for siege..."
echo "Initial memory: ${INITIAL_MEM} KB"
echo ""

# Create temp file for results
TEMP_RESULTS=$(mktemp)
trap "rm -f $TEMP_RESULTS" EXIT

# Worker function
worker() {
    local url=$1
    local duration=$2
    local worker_id=$3
    local end_time=$(($(date +%s) + duration))
    local success=0
    local failed=0
    
    while [ $(date +%s) -lt $end_time ]; do
        if curl -s -o /dev/null -w "%{http_code}" --max-time 3 "$url" 2>&1 | grep -q "200"; then
            ((success++))
        else
            ((failed++))
        fi
    done
    
    echo "$success $failed" >> "$TEMP_RESULTS"
}

# Start workers
echo "** Lifting the server siege..."
START_TIME=$(date +%s)

for ((i=1; i<=$CONCURRENT; i++)); do
    worker "$SERVER_URL" "$DURATION" $i &
done

# Wait for all workers
wait

END_TIME=$(date +%s)
ACTUAL_DURATION=$((END_TIME - START_TIME))

# Collect results
TOTAL_SUCCESS=0
TOTAL_FAILED=0

while read success failed; do
    TOTAL_SUCCESS=$((TOTAL_SUCCESS + success))
    TOTAL_FAILED=$((TOTAL_FAILED + failed))
done < "$TEMP_RESULTS"

TOTAL_TRANS=$((TOTAL_SUCCESS + TOTAL_FAILED))
TRANS_RATE=$(awk "BEGIN {printf \"%.2f\", $TOTAL_TRANS / $ACTUAL_DURATION}")
AVAILABILITY=$(awk "BEGIN {printf \"%.2f\", ($TOTAL_SUCCESS / $TOTAL_TRANS) * 100}")
SUCCESSFUL_TRANS=$TOTAL_SUCCESS
FAILED_TRANS=$TOTAL_FAILED

# Get final memory
FINAL_MEM=$(ps -p $SERVER_PID -o rss= 2>/dev/null | tr -d ' ')
MEM_GROWTH=$((FINAL_MEM - INITIAL_MEM))

# Get connection stats
ESTABLISHED=$(netstat -an 2>/dev/null | grep "ESTABLISHED.*:1025" | wc -l | tr -d ' ')
CLOSE_WAIT=$(netstat -an 2>/dev/null | grep "CLOSE_WAIT.*:1025" | wc -l | tr -d ' ')
TIME_WAIT=$(netstat -an 2>/dev/null | grep "TIME_WAIT.*:1025" | wc -l | tr -d ' ')

echo ""
echo -e "${GREEN}Done.${NC}"
echo ""

# Display siege-like output
echo -e "${BLUE}Transactions:${NC}          ${GREEN}${SUCCESSFUL_TRANS} hits${NC}"
echo -e "${BLUE}Availability:${NC}          $([ $(echo "$AVAILABILITY >= 99.5" | bc) -eq 1 ] && echo -e ${GREEN} || echo -e ${YELLOW})${AVAILABILITY} %${NC}"
echo -e "${BLUE}Elapsed time:${NC}          ${GREEN}${ACTUAL_DURATION} secs${NC}"
echo -e "${BLUE}Data transferred:${NC}      N/A"
echo -e "${BLUE}Response time:${NC}         N/A"
echo -e "${BLUE}Transaction rate:${NC}      ${GREEN}${TRANS_RATE} trans/sec${NC}"
echo -e "${BLUE}Throughput:${NC}            N/A"
echo -e "${BLUE}Concurrency:${NC}           ${GREEN}${CONCURRENT}${NC}"
echo -e "${BLUE}Successful transactions:${NC} ${GREEN}${SUCCESSFUL_TRANS}${NC}"
echo -e "${BLUE}Failed transactions:${NC}    $([ $FAILED_TRANS -eq 0 ] && echo -e ${GREEN} || echo -e ${RED})${FAILED_TRANS}${NC}"
echo ""

# Additional WebServ-specific stats
echo -e "${BLUE}=== Server Health ===${NC}"
echo -e "Memory growth:       $([ $MEM_GROWTH -lt 5000 ] && echo -e ${GREEN} || echo -e ${YELLOW})${MEM_GROWTH} KB${NC}"
echo -e "ESTABLISHED conns:   ${GREEN}${ESTABLISHED}${NC}"
echo -e "TIME_WAIT conns:     ${GREEN}${TIME_WAIT}${NC}"
echo -e "CLOSE_WAIT conns:    $([ ${CLOSE_WAIT:-0} -eq 0 ] && echo -e ${GREEN} || echo -e ${YELLOW})${CLOSE_WAIT}${NC}"
echo ""

# Check if server still running
if kill -0 $SERVER_PID 2>/dev/null; then
    if curl -s -o /dev/null -w "%{http_code}" --max-time 3 "$SERVER_URL" 2>&1 | grep -q "200"; then
        echo -e "${GREEN}✅ Server operational after siege${NC}"
    else
        echo -e "${RED}❌ Server not responding${NC}"
    fi
else
    echo -e "${RED}❌ Server crashed during siege${NC}"
    exit 1
fi

echo ""
echo -e "${BLUE}=== Verdict ===${NC}"

PASS_COUNT=0
TOTAL_CHECKS=4

if [ $(echo "$AVAILABILITY >= 99.5" | bc) -eq 1 ]; then
    echo -e "${GREEN}✓ Availability >= 99.5%${NC}"
    ((PASS_COUNT++))
else
    echo -e "${RED}✗ Availability < 99.5% (${AVAILABILITY}%)${NC}"
fi

if [ $MEM_GROWTH -lt 10000 ]; then
    echo -e "${GREEN}✓ No memory leak detected${NC}"
    ((PASS_COUNT++))
else
    echo -e "${YELLOW}⚠ Memory growth: ${MEM_GROWTH} KB${NC}"
fi

if [ ${CLOSE_WAIT:-0} -eq 0 ]; then
    echo -e "${GREEN}✓ No hanging connections${NC}"
    ((PASS_COUNT++))
else
    echo -e "${YELLOW}⚠ Hanging connections: ${CLOSE_WAIT}${NC}"
fi

if kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "${GREEN}✓ Server stability maintained${NC}"
    ((PASS_COUNT++))
else
    echo -e "${RED}✗ Server crashed${NC}"
fi

echo ""
if [ $PASS_COUNT -eq $TOTAL_CHECKS ]; then
    echo -e "${GREEN}╔════════════════════════════╗${NC}"
    echo -e "${GREEN}║  ALL REQUIREMENTS PASSED!  ║${NC}"
    echo -e "${GREEN}╚════════════════════════════╝${NC}"
else
    echo -e "${YELLOW}Passed ${PASS_COUNT}/${TOTAL_CHECKS} checks${NC}"
fi
