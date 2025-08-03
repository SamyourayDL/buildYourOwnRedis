# [Build Your Own Redis with C/C++](https://build-your-own.org/redis/)
## Goals
- Understand the architecture and core concepts of Redis
- Improve skills in network programming
- Gain a low-level understanding of concurrent I/O models
- Build a complete low-level project from the ground up (0 to 100)


| Chapter                     | Status | Notes                                                                                  |
| ------------------------- | ------ | ------------------------------------------------------------------------------------------------- |
| Introduction              | ✅      | What is Redis and its key concepts                                                            |
| Socket Programming        | ✅      | Linux scoket API: `socket()`, `bind()`, `listen()` и `accept()`                |
| TCP Server and Client     | ✅      | Simple client-server on top of sockets(TCP)                           |
| Request-Response Protocol | ✅      | Application-level network protocol. Parse the stream of bytes into data.             |
| Concurrent IO Models      | ✅      | **Event-loop + poll + non-blocking `read()/write()`** vs Thread-based concurrency   |
| Event Loop (Part 1)       | ✅      | Event-loop realisation based on `poll()`.                                               |
| Event Loop (Part 2)       | ✅     | Pipelined-requests optimisation + optimistyc `write() `                   |
| Key-Value Server          | 🔜       | Finalization of client-server model                             |
| Hashtables (Part 1)       | ❌      |                                  |
| Hashtables (Part 2)       | ❌      |                                       |
| Data Serialization        | ❌      |                       |
| Balanced Binary Tree      | ❌      |  |
| Sorted Set                | ❌      |                                          |
| Timer and Timeout         | ❌      |               |
| Cache Expiration with TTL | ❌      |                                      |
| Thread Pool               | ❌      |                          |
| Tests                     | ❌      |                          |

