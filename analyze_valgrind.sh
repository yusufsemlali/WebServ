#!/bin/bash

# Valgrind log analyzer for webserv
# This script helps analyze valgrind output for common webserv issues

VALGRIND_LOG="valgrind.log"

if [ ! -f "$VALGRIND_LOG" ]; then
    echo "❌ Valgrind log file not found: $VALGRIND_LOG"
    echo "Run 'make valgrind' first"
    exit 1
fi

echo "🔍 Analyzing valgrind output for webserv..."
echo "============================================"

# Check for memory leaks
LEAKS=$(grep -c "definitely lost\|indirectly lost\|possibly lost" "$VALGRIND_LOG")
if [ "$LEAKS" -gt 0 ]; then
    echo "⚠️  Memory leaks detected:"
    grep -A5 -B5 "definitely lost\|indirectly lost\|possibly lost" "$VALGRIND_LOG"
else
    echo "✅ No memory leaks detected"
fi

echo ""

# Check for file descriptor leaks
FD_LEAKS=$(grep -c "Open file descriptor" "$VALGRIND_LOG")
if [ "$FD_LEAKS" -gt 0 ]; then
    echo "⚠️  File descriptor leaks detected:"
    grep -A3 -B1 "Open file descriptor" "$VALGRIND_LOG"
else
    echo "✅ No file descriptor leaks detected"
fi

echo ""

# Check for invalid reads/writes
INVALID_OPS=$(grep -c "Invalid read\|Invalid write" "$VALGRIND_LOG")
if [ "$INVALID_OPS" -gt 0 ]; then
    echo "❌ Invalid memory operations detected:"
    grep -A10 -B2 "Invalid read\|Invalid write" "$VALGRIND_LOG"
else
    echo "✅ No invalid memory operations detected"
fi

echo ""

# Check for use of uninitialized values
UNINIT=$(grep -c "Conditional jump.*uninitialized\|Use of uninitialised" "$VALGRIND_LOG")
if [ "$UNINIT" -gt 0 ]; then
    echo "⚠️  Use of uninitialized values detected:"
    grep -A5 -B2 "Conditional jump.*uninitialized\|Use of uninitialised" "$VALGRIND_LOG"
else
    echo "✅ No uninitialized value usage detected"
fi

echo ""

# Summary
echo "📊 SUMMARY:"
echo "==========="
HEAP_SUMMARY=$(grep -A10 "HEAP SUMMARY" "$VALGRIND_LOG")
if [ ! -z "$HEAP_SUMMARY" ]; then
    echo "$HEAP_SUMMARY"
fi

LEAK_SUMMARY=$(grep -A20 "LEAK SUMMARY" "$VALGRIND_LOG")
if [ ! -z "$LEAK_SUMMARY" ]; then
    echo "$LEAK_SUMMARY"
fi

# Error exit code check
ERROR_EXIT=$(grep "ERROR SUMMARY:" "$VALGRIND_LOG" | tail -1)
if [ ! -z "$ERROR_EXIT" ]; then
    if [[ "$ERROR_EXIT" == *"0 errors"* ]]; then
        echo ""
        echo "🎉 Congratulations! No valgrind errors found!"
        echo "Your webserv should pass memory leak tests."
    else
        echo ""
        echo "❌ Errors found in valgrind output."
        echo "$ERROR_EXIT"
        echo "Fix the issues above before submission."
    fi
else
    echo ""
    echo "⚠️  Valgrind log appears incomplete (no ERROR SUMMARY found)."
    echo "This might happen if the process was interrupted."
    echo "Try running 'make valgrind' again and let it complete."
fi
