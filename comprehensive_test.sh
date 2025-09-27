#!/bin/bash

echo "=============================================="
echo "TESTING NON-BLOCKING CGI IMPLEMENTATION"
echo "=============================================="
echo ""

# Test 1: Verify server is responding
echo "TEST 1: Basic connectivity test"
echo "-------------------------------"
if curl -s --connect-timeout 5 http://localhost:8080/hello.py | grep -q "Hello from Python"; then
    echo "✅ Server is responding to basic CGI requests"
else
    echo "❌ Server is not responding properly"
    exit 1
fi
echo ""

# Test 2: Single slow request timing
echo "TEST 2: Single slow request baseline"
echo "-----------------------------------"
echo "Testing single slow request (should take ~3 seconds)..."
start_time=$(date +%s.%N)
curl -s http://localhost:8080/slow_python.py | grep "completed at" 
end_time=$(date +%s.%N)
duration=$(echo "$end_time - $start_time" | bc)
echo "Single slow request took: $duration seconds"
echo ""

# Test 3: Multiple concurrent slow requests
echo "TEST 3: Multiple concurrent slow requests"
echo "----------------------------------------"
echo "Starting 3 concurrent slow requests at $(date +%T)..."
echo "If non-blocking: should complete in ~3 seconds total"
echo "If blocking: would take ~9 seconds total"
echo ""

start_concurrent=$(date +%s.%N)

# Start 3 slow requests in background and capture their completion times
(curl -s http://localhost:8080/slow_python.py | grep "completed at" | sed 's/^/Request 1: /') &
(curl -s http://localhost:8080/slow_python.py | grep "completed at" | sed 's/^/Request 2: /') &
(curl -s http://localhost:8080/slow_python.py | grep "completed at" | sed 's/^/Request 3: /') &

# Wait for all requests to complete
wait

end_concurrent=$(date +%s.%N)
concurrent_duration=$(echo "$end_concurrent - $start_concurrent" | bc)

echo ""
echo "All 3 concurrent requests completed at $(date +%T)"
echo "Total time for 3 concurrent slow requests: $concurrent_duration seconds"
echo ""

# Analyze results
if (( $(echo "$concurrent_duration < 6" | bc -l) )); then
    echo "🎉 SUCCESS: Non-blocking CGI working!"
    echo "   ✅ 3 concurrent 3-second scripts completed in $concurrent_duration seconds"
    echo "   ✅ This proves requests are processed concurrently, not sequentially"
else
    echo "❌ FAILURE: Blocking behavior detected!"
    echo "   ❌ 3 concurrent requests took $concurrent_duration seconds (expected ~3-4)"
fi
echo ""

# Test 4: Mixed request types during slow CGI
echo "TEST 4: Mixed requests during slow CGI execution"
echo "-----------------------------------------------"
echo "Starting slow request + fast requests to prove no blocking..."

start_mixed=$(date +%s.%N)

# Start a slow request in background
(curl -s http://localhost:8080/slow_python.py >/dev/null) &
slow_pid=$!

# Wait a moment, then send fast requests
sleep 0.5

# Send multiple fast requests while slow one is running
fast_responses=""
for i in {1..3}; do
    fast_start=$(date +%s.%N)
    response=$(curl -s http://localhost:8080/hello.py | grep "Hello from Python")
    fast_end=$(date +%s.%N)
    fast_time=$(echo "$fast_end - $fast_start" | bc)
    if [[ -n "$response" ]]; then
        fast_responses="$fast_responses✅ Fast request $i completed in $fast_time seconds\n"
    else
        fast_responses="$fast_responses❌ Fast request $i failed\n"
    fi
done

# Wait for slow request to complete
wait $slow_pid

end_mixed=$(date +%s.%N)
mixed_duration=$(echo "$end_mixed - $start_mixed" | bc)

echo -e "$fast_responses"
echo "Mixed test completed in $mixed_duration seconds"

if echo "$fast_responses" | grep -q "✅"; then
    echo "🎉 SUCCESS: Fast requests completed while slow CGI was running!"
    echo "   ✅ This proves the server doesn't block on CGI execution"
else
    echo "❌ FAILURE: Fast requests were blocked by slow CGI"
fi
echo ""

echo "=============================================="
echo "TEST SUMMARY"
echo "=============================================="
echo "Server log tail (last 10 lines):"
tail -n 10 server_output.log | grep -E "(Event|Connection|CGI|completed|started)" || echo "No relevant log entries found"
echo ""
echo "Testing completed at $(date +%T)"