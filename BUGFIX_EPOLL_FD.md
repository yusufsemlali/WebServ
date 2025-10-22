# Bug Fix: "Bad file descriptor" Error During Epoll Cleanup

## Problem Description
The server was logging errors when trying to remove file descriptors from epoll:
```
Closing connection on fd: 7
Failed to remove file descriptor 7 from epoll: Bad file descriptor
Connection closed successfully
```

## Root Cause
**Double-close bug** in connection cleanup sequence:

1. When a read/write error occurred in `ClientConnection::readData()` or `writeData()`, 
   these methods would call `close()` immediately, closing the file descriptor.

2. The methods would then return `false`, signaling HttpServer to cleanup the connection.

3. `HttpServer::closeConnection()` would try to remove the already-closed FD from epoll 
   using `eventLoop.remove()`, which fails with EBADF (Bad file descriptor).

4. Finally, deleting the ClientConnection would trigger the destructor, which calls 
   `close()` again (harmlessly, as socketFd is set to -1 after first close).

## Timeline of Bug
```
ClientConnection::readData()
  └─> recv() returns 0 (client closed)
  └─> close() called ← FD CLOSED HERE
  └─> returns false

HttpServer::handleRead()
  └─> sees false return
  └─> calls closeConnection(clientFd)

HttpServer::closeConnection()
  └─> eventLoop.remove(clientFd) ← FAILS (FD already closed)
  └─> deletes ClientConnection
      └─> ~ClientConnection()
          └─> close() ← harmless (socketFd already -1)
```

## Solution

### 1. Removed Premature close() Calls
**File:** `src/event/ClientConnection.cpp`

Removed `close()` calls from:
- Line 58: Error path in `readData()`
- Line 63: Client disconnection path in `readData()` 
- Line 100: Error path in `writeData()`

**Rationale:** Let `HttpServer::closeConnection()` manage the complete connection lifecycle.
This ensures proper cleanup order: remove from epoll → close FD.

### 2. Made Epoll Removal Robust
**File:** `src/event/EventLoop.cpp`

Added check for EBADF errno in `EventLoop::remove()`:
```cpp
if (errno != EBADF)
{
    std::cerr << "Failed to remove fd..." << std::endl;
}
```

**Rationale:** EBADF during cleanup is acceptable (FD already closed). Only log other errors.

### 3. Code Cleanup
**File:** `src/event/ClientConnection.cpp`

- Removed unused `#include <fstream>`
- Removed duplicate `#include "ClientConnection.hpp"`

## Testing
Compile tested: ✅ Clean compile with `-Wall -Wextra -Werror`

**Expected Behavior:**
- No more "Bad file descriptor" errors in logs
- Clean connection cleanup on client disconnect
- Proper epoll management throughout connection lifecycle

## Impact
- **Severity:** Medium - Error was cosmetic but indicated poor resource management
- **Scope:** All client disconnections and I/O errors
- **Risk:** Low - Fix follows established patterns for connection lifecycle management
