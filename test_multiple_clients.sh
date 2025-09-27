#!/bin/bash

# Test script for multiple simultaneous clients

echo "Starting multi-client test..."
echo "This will test if multiple clients can be served simultaneously"
echo ""

# Function to test a single client
test_client() {
    local client_id=$1
    local url=$2
    local output_file="/tmp/client_${client_id}.log"
    
    echo "Client $client_id: Starting request to $url"
    start_time=$(date +%s.%N)
    
    curl -s -o "$output_file" -w "%{time_total}\n" "http://localhost:8080$url" > "/tmp/client_${client_id}_time.log" 2>&1
    
    end_time=$(date +%s.%N)
    duration=$(echo "$end_time - $start_time" | bc)
    
    echo "Client $client_id: Completed in $duration seconds"
    
    # Check if response was successful
    if grep -q "Slow Python CGI Script" "$output_file"; then
        echo "Client $client_id: SUCCESS - Got expected CGI output"
    elif grep -q "Hello from Python CGI" "$output_file"; then
        echo "Client $client_id: SUCCESS - Got fast CGI output"
    elif grep -q "PNG" "$output_file"; then
        echo "Client $client_id: SUCCESS - Got static file"
    else
        echo "Client $client_id: ERROR - Unexpected response"
        head -n 5 "$output_file"
    fi
}

# Start server in background
echo "Starting server..."
cd /home/Dev/Desktop/Webserv
./webserv configs/default.conf > server_test.log 2>&1 &
SERVER_PID=$!

# Wait for server to start
sleep 2

echo "Server started with PID $SERVER_PID"
echo ""

# Test 1: Multiple slow CGI requests simultaneously
echo "=== TEST 1: Multiple slow CGI requests (should complete in ~3 seconds total, not 9) ==="
start_total=$(date +%s.%N)

# Start 3 slow CGI requests in parallel
test_client 1 "/slow_python.py" &
test_client 2 "/slow_python.py" &
test_client 3 "/slow_python.py" &

# Wait for all background jobs to complete
wait

end_total=$(date +%s.%N)
total_duration=$(echo "$end_total - $start_total" | bc)

echo ""
echo "Total time for 3 simultaneous slow CGI requests: $total_duration seconds"

if (( $(echo "$total_duration < 6" | bc -l) )); then
    echo "✅ SUCCESS: Non-blocking CGI working! (< 6 seconds for 3x3-second scripts)"
else
    echo "❌ FAILURE: Blocking behavior detected! (took too long)"
fi

echo ""
echo "=== TEST 2: Mix of fast CGI, slow CGI, and static files ==="
start_mixed=$(date +%s.%N)

# Start mixed requests in parallel
test_client 4 "/hello.py" &
test_client 5 "/slow_python.py" &
test_client 6 "/zarafa.png" &
test_client 7 "/hello.py" &

wait

end_mixed=$(date +%s.%N)
mixed_duration=$(echo "$end_mixed - $start_mixed" | bc)

echo ""
echo "Total time for mixed requests: $mixed_duration seconds"

if (( $(echo "$mixed_duration < 5" | bc -l) )); then
    echo "✅ SUCCESS: Mixed request handling working!"
else
    echo "❌ FAILURE: Mixed request handling too slow!"
fi

echo ""
echo "=== Stopping server ==="
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo "Test completed!"
echo ""
echo "Server log (last 20 lines):"
tail -n 20 server_test.log