#!/bin/bash

# Quick WebServ Stress Test

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SERVER_URL="${1:-http://localhost:1025}"
REQUESTS="${2:-500}"
CONCURRENT="${3:-5}"

echo -e "${BLUE}=== WebServ Quick Stress Test ===${NC}"
echo "URL: $SERVER_URL"
echo "Total Requests: $REQUESTS"
echo "Concurrent: $CONCURRENT"
echo ""

# Find server PID
SERVER_PID=$(pgrep -f "webserv" | head -1)
if [ -z "$SERVER_PID" ]; then
    echo -e "${RED}❌ Server not running${NC}"
    exit 1
fi

# Get initial memory
INITIAL_MEM=$(ps -p $SERVER_PID -o rss= 2>/dev/null | tr -d ' ')
echo -e "Server PID: ${GREEN}$SERVER_PID${NC}"
echo -e "Initial Memory: ${GREEN}${INITIAL_MEM} KB${NC}"
echo ""

# Run requests
echo "Running stress test..."
SUCCESS=0
FAILED=0
START_TIME=$(date +%s)

for ((i=0; i<$REQUESTS; i+=$CONCURRENT)); do
    for ((j=0; j<$CONCURRENT && i+j<$REQUESTS; j++)); do
        {
            if curl -s -o /dev/null -w "%{http_code}" --max-time 2 "$SERVER_URL/" 2>&1 | grep -q "200"; then
                echo "1"
            else
                echo "0"
            fi
        } &
    done
    wait
    
    # Progress indicator
    if [ $((i % 100)) -eq 0 ]; then
        echo -n "."
    fi
done | {
    while read result; do
        if [ "$result" == "1" ]; then
            ((SUCCESS++))
        else
            ((FAILED++))
        fi
    done
    
    echo ""
    echo ""
    
    END_TIME=$(date +%s)
    DURATION=$((END_TIME - START_TIME))
    
    # Get final memory
    FINAL_MEM=$(ps -p $SERVER_PID -o rss= 2>/dev/null | tr -d ' ')
    MEM_GROWTH=$((FINAL_MEM - INITIAL_MEM))
    
    # Get connection stats
    ESTABLISHED=$(netstat -an 2>/dev/null | grep -c "ESTABLISHED.*:1025" || echo 0)
    CLOSE_WAIT=$(netstat -an 2>/dev/null | grep -c "CLOSE_WAIT.*:1025" || echo 0)
    
    # Calculate availability
    TOTAL=$((SUCCESS + FAILED))
    AVAILABILITY=$(awk "BEGIN {printf \"%.2f\", ($SUCCESS / $TOTAL) * 100}")
    
    # Results
    echo -e "${BLUE}=== Results ===${NC}"
    echo -e "Duration: ${GREEN}${DURATION}s${NC}"
    echo -e "Total: ${GREEN}${TOTAL}${NC}"
    echo -e "Success: ${GREEN}${SUCCESS}${NC}"
    echo -e "Failed: ${RED}${FAILED}${NC}"
    echo -e "Availability: $([ $(echo "$AVAILABILITY >= 99.5" | bc) -eq 1 ] && echo -e ${GREEN} || echo -e ${RED})${AVAILABILITY}%${NC}"
    echo -e "Requests/sec: ${GREEN}$(awk "BEGIN {printf \"%.2f\", $TOTAL / $DURATION}")${NC}"
    echo ""
    echo -e "${BLUE}=== Memory ===${NC}"
    echo -e "Initial: ${GREEN}${INITIAL_MEM} KB${NC}"
    echo -e "Final: ${GREEN}${FINAL_MEM} KB${NC}"
    echo -e "Growth: $([ $MEM_GROWTH -lt 5000 ] && echo -e ${GREEN} || echo -e ${YELLOW})${MEM_GROWTH} KB${NC}"
    echo ""
    echo -e "${BLUE}=== Connections ===${NC}"
    echo -e "ESTABLISHED: ${GREEN}${ESTABLISHED}${NC}"
    echo -e "CLOSE_WAIT: $([ $CLOSE_WAIT -eq 0 ] && echo -e ${GREEN} || echo -e ${YELLOW})${CLOSE_WAIT}${NC}"
    echo ""
    
    # Check if server still running
    if kill -0 $SERVER_PID 2>/dev/null; then
        echo -e "${GREEN}✅ Server still running${NC}"
        
        # Test final responsiveness
        if curl -s -o /dev/null -w "%{http_code}" --max-time 2 "$SERVER_URL/" 2>&1 | grep -q "200"; then
            echo -e "${GREEN}✅ Server responsive${NC}"
        else
            echo -e "${RED}❌ Server not responsive${NC}"
        fi
    else
        echo -e "${RED}❌ Server crashed${NC}"
    fi
    echo ""
    
    # Final verdict
    echo -e "${BLUE}=== Requirements Check ===${NC}"
    if [ $(echo "$AVAILABILITY >= 99.5" | bc) -eq 1 ]; then
        echo -e "${GREEN}✅ Availability >= 99.5%${NC}"
    else
        echo -e "${RED}❌ Availability < 99.5%${NC}"
    fi
    
    if [ $MEM_GROWTH -lt 10000 ]; then
        echo -e "${GREEN}✅ No significant memory leak${NC}"
    else
        echo -e "${YELLOW}⚠️  Memory growth detected${NC}"
    fi
    
    if [ $CLOSE_WAIT -eq 0 ]; then
        echo -e "${GREEN}✅ No hanging connections${NC}"
    else
        echo -e "${YELLOW}⚠️  Hanging connections: $CLOSE_WAIT${NC}"
    fi
}
