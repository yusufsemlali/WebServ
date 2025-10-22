# Fix: 400 Error Page Now Serves Configured HTML File

## Problem Identified
When request parsing failed in `ClientConnection::processReadBuffer()`, the server was returning a **hardcoded plain-text 400 error** instead of serving the custom `400.html` error page configured in the server config file.

### Original Behavior (INCORRECT)
- **Location**: `src/event/ClientConnection.cpp`, lines 254-268
- **Issue**: Hardcoded response:
  ```cpp
  context.response.setStatus(400, "Bad Request");
  context.response.setBody("Invalid HTTP request format");  // Plain text!
  context.response.setHeader("Content-Type", "text/plain");
  ```
- **Result**: Users saw plain text "Invalid HTTP request format" instead of the styled HTML error page

## Solution Implemented

### 1. Added Public Method to RequestHandler
**File**: `includes/RequestHandler.hpp`
- Added: `void generateErrorPage(int errorCode, HttpResponse &response, int serverFd);`
- This method generates error pages when we only have a serverFd (before full request parsing)

### 2. Implemented Error Page Generation Logic
**File**: `src/http/RequestHandler.cpp`
- The `generateErrorPage()` method:
  1. Gets the server config for the given serverFd
  2. Calls `serveErrorPage()` which checks for custom error pages in config
  3. Falls back to default HTML error page if custom page not found

### 3. Updated ClientConnection to Use New Method
**File**: `src/event/ClientConnection.cpp`
- Replaced hardcoded error response with:
  ```cpp
  handleRequest.generateErrorPage(400, context.response, serverFd);
  ```

## Expected Behavior (CORRECT)
Now when request parsing fails:
1. The server finds the correct server config for the connection
2. Checks for custom `error_page 400` directive in config
3. Serves the configured HTML file (e.g., `./www/errors/400.html`)
4. Falls back to default styled HTML if custom page not found

## Config Example
```nginx
server {
    listen *:1025;
    error_page 400 ./www/errors/400.html;  # Now properly served!
    error_page 404 ./errors/404.html;
    error_page 500 ./www/errors/500.html;
    ...
}
```

## Testing
Run the test script to verify:
```bash
./test_400_error.sh
```

Or manually test with malformed request:
```bash
echo -e "GET /test\r\n\r\n" | nc localhost 1024
```

You should now see the styled HTML from `www/errors/400.html` with proper formatting, styling, and helpful error information.

## Files Modified
1. `includes/RequestHandler.hpp` - Added public method declaration
2. `src/http/RequestHandler.cpp` - Implemented generateErrorPage()
3. `src/event/ClientConnection.cpp` - Updated processReadBuffer() to use new method

## Result
✅ All 400 errors now properly serve the configured custom error page from the config file
✅ Consistent error handling across all error codes
✅ Users see styled, helpful error pages instead of plain text messages
