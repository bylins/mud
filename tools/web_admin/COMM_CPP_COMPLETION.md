# comm.cpp Completion Guide

This document contains the exact code snippets that need to be added to `src/engine/core/comm.cpp` to complete the Admin API implementation.

## Overview

The Unix socket infrastructure requires 5 additions to comm.cpp:
1. Unix socket functions (after `init_socket()`)
2. Global socket variable (with other globals)
3. Socket initialization (in `stop_game()`)
4. Event handling (in `game_loop()`)
5. Cleanup (in `close_socket()`)

---

## 1. Add Unix Socket Functions

**Location**: After `init_socket()` function (around line 980)

**Code to add**:

```cpp
#ifdef ENABLE_ADMIN_API

// Global variables for Admin API
static int active_admin_connections = 0;
static const int MAX_ADMIN_CONNECTIONS = 1;

/*
 * init_unix_socket creates Admin API Unix domain socket
 */
socket_t init_unix_socket(const char *path) {
	socket_t s;
	struct sockaddr_un sa;

	// Create Unix Domain Socket
	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		log("SYSERR: Error creating Unix domain socket: %s", strerror(errno));
		exit(1);
	}

	// Configure address
	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, path, sizeof(sa.sun_path) - 1);

	// Remove old socket file
	unlink(path);

	// Bind to path
	if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		log("SYSERR: Cannot bind Unix socket to %s: %s", path, strerror(errno));
		CLOSE_SOCKET(s);
		exit(1);
	}

	// Set permissions (owner only)
	chmod(path, 0600);

	// Non-blocking mode
	nonblock(s);

	// Start listening (backlog=1 for minimal resources)
	if (listen(s, 1) < 0) {
		log("SYSERR: Cannot listen on Unix socket: %s", strerror(errno));
		CLOSE_SOCKET(s);
		exit(1);
	}

	log("Admin API listening on Unix socket: %s", path);
	return s;
}

/*
 * new_admin_descriptor handles new Admin API connections
 */
int new_admin_descriptor(int epoll, socket_t s) {
	socket_t desc;
	DescriptorData *newd;

	// Check connection limit
	if (active_admin_connections >= MAX_ADMIN_CONNECTIONS) {
		// Accept connection
		desc = accept(s, nullptr, nullptr);
		if (desc != kInvalidSocket) {
			// Send error and close
			const char *error_msg = "{\"status\":\"error\",\"error\":\"Max admin connections reached\"}\n";
			write(desc, error_msg, strlen(error_msg));
			CLOSE_SOCKET(desc);
			log("Admin API: rejected connection (limit reached)");
		}
		return -1;
	}

	// Accept connection
	desc = accept(s, nullptr, nullptr);
	if (desc == kInvalidSocket) {
		return -1;
	}

	// Non-blocking mode
	nonblock(desc);

	// Create descriptor
	CREATE(newd, DescriptorData, 1);

	// Initialization
	newd->descriptor = desc;
	newd->state = EConState::kAdminAPI;
	newd->admin_api_mode = true;
	strcpy(newd->host, "unix-socket");
	newd->login_time = time(0);

	// Add to epoll
#ifdef HAS_EPOLL
	struct epoll_event event;
	event.data.ptr = newd;
	event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
	epoll_ctl(epoll, EPOLL_CTL_ADD, desc, &event);
#endif

	// Add to descriptor_list
	newd->next = descriptor_list;
	descriptor_list = newd;

	// Increment counter
	++active_admin_connections;

	log("Admin API: new connection from Unix socket (active: %d)", active_admin_connections);

	// Send greeting (JSON)
	write_to_descriptor(desc, "{\"status\":\"ready\",\"version\":\"1.0\"}\n");

	return 0;
}

/*
 * close_admin_descriptor handles admin connection cleanup
 */
void close_admin_descriptor(DescriptorData *d) {
	if (d->admin_api_mode && active_admin_connections > 0) {
		--active_admin_connections;
		log("Admin API: connection closed (active: %d)", active_admin_connections);
	}
}

#endif // ENABLE_ADMIN_API
```

---

## 2. Add Global Socket Variable

**Location**: Near the top of the file, where `mother_desc` is declared (search for "socket_t mother_desc" or look in `stop_game()` function around line 728)

**Code to add**:

```cpp
#ifdef ENABLE_ADMIN_API
static socket_t admin_socket = kInvalidSocket;
#endif
```

**Note**: This should be at file scope (global), not inside a function.

---

## 3. Initialize Socket in stop_game()

**Location**: In `stop_game()` function, after `mother_desc = init_socket(port);` (around line 743)

**Code to add**:

```cpp
#ifdef ENABLE_ADMIN_API
	// Initialize Admin API Unix socket
	std::string socket_path = std::string(LIB_PREFIX) + "/admin_api.sock";
	admin_socket = init_unix_socket(socket_path.c_str());
#endif
```

---

## 4. Handle Events in game_loop()

**Location**: In `game_loop()` function, inside the epoll event processing loop

**Find this section** (around line 1300-1350):

```cpp
	if (events[i].events & EPOLLIN) {
		d = (DescriptorData *) events[i].data.ptr;

		// Check for new connection
		if (mother_desc == d->descriptor) {
			new_descriptor(epoll, mother_desc);
		}
		// ... existing code ...
```

**Add after the `mother_desc` check**:

```cpp
		// Check for Admin API connection
#ifdef ENABLE_ADMIN_API
		else if (admin_socket != kInvalidSocket && admin_socket == d->descriptor) {
			new_admin_descriptor(epoll, admin_socket);
		}
#endif
```

**Full context** (should look like this):

```cpp
	if (events[i].events & EPOLLIN) {
		d = (DescriptorData *) events[i].data.ptr;

		// Check for new connection
		if (mother_desc == d->descriptor) {
			new_descriptor(epoll, mother_desc);
		}
		// Check for Admin API connection
#ifdef ENABLE_ADMIN_API
		else if (admin_socket != kInvalidSocket && admin_socket == d->descriptor) {
			new_admin_descriptor(epoll, admin_socket);
		}
#endif
		else {
			// Regular data processing
			if (process_input(d) < 0)
				close_socket(d, ...);
		}
	}
```

**Note**: If you don't have epoll, find the equivalent section for your polling mechanism (select, poll).

---

## 5. Add Cleanup in close_socket()

**Location**: In `close_socket()` function, at the very beginning (search for "void close_socket")

**Code to add**:

```cpp
#ifdef ENABLE_ADMIN_API
	// Decrement admin connection counter if needed
	close_admin_descriptor(d);
#endif
```

**Full context** (should look like this):

```cpp
void close_socket(DescriptorData *d, int silent) {
#ifdef ENABLE_ADMIN_API
	// Decrement admin connection counter if needed
	close_admin_descriptor(d);
#endif

	// ... rest of existing close_socket code ...
```

---

## Additional Notes

### Required Includes

The following includes should already be in comm.cpp (added by patch):

```cpp
#ifdef ENABLE_ADMIN_API
#include "engine/network/admin_api.h"
#endif
```

If not present, add at the top of the file after other network includes.

### Unix Socket Header

Make sure `<sys/un.h>` is included. It's usually already included on Unix systems, but if you get compilation errors about `sockaddr_un`, add:

```cpp
#ifdef ENABLE_ADMIN_API
#include <sys/un.h>
#endif
```

---

## Verification

After adding all the code, verify:

1. **Compilation**:
```bash
cd build_admin
cmake -DENABLE_ADMIN_API=ON ..
make -j$(nproc)
```

2. **Socket Creation**:
```bash
./circle -W -d small 4000
# Check logs for: "Admin API listening on Unix socket: ..."
ls -la small/lib/admin_api.sock
# Should show: srw------- (0600 permissions)
```

3. **Connection Test**:
```bash
echo '{"command":"ping"}' | nc -U small/lib/admin_api.sock
# Should return: {"status":"pong"}
```

---

## Troubleshooting

### Compilation Errors

- **`sockaddr_un` undeclared**: Add `#include <sys/un.h>`
- **`admin_api_parse` undeclared**: Check `#include "engine/network/admin_api.h"`
- **`EConState::kAdminAPI` not found**: Rebuild from clean (cmake cache issue)

### Runtime Errors

- **Socket not created**: Check syslog for errors, verify path is writable
- **Permission denied**: Check socket permissions with `ls -la`
- **Connection refused**: Make sure Circle is still running, socket file exists

### Debug Logging

Add debug output to verify socket initialization:

```cpp
log("DEBUG: admin_socket initialized, fd=%d", admin_socket);
```

---

## Summary Checklist

- [ ] Add Unix socket functions after `init_socket()` (lines ~980-1100)
- [ ] Add global `admin_socket` variable (file scope)
- [ ] Initialize socket in `stop_game()` (after line ~743)
- [ ] Add event handling in `game_loop()` (in epoll loop)
- [ ] Add cleanup call in `close_socket()` (at beginning)
- [ ] Verify compilation with `-DENABLE_ADMIN_API=ON`
- [ ] Test socket creation and ping command

**Estimated time**: 15-20 minutes for careful insertion and testing.
