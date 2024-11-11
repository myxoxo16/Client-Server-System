# CIS\*3210 Assignment 3

A simple client-server system for transferring text files over TCP.

## By

Peter Mangialardi (1227621), Merna Yousif (1198527)

## Dependencies

- GCC compiler
- Standard C libraries
- Unix/Linux environment

## Compilation

```bash
make all    # Builds both server and sendFile
make clean  # Removes compiled files
```

## Running the Programs

### Server

```bash
./server port-number [bufSize]

# Example:
./server 8080         # Uses default buffer size (4096)
./server 8080 1024    # Uses custom buffer size
```

### Client (sendFile)

```bash
./sendFile fileName IP-address:port-number [bufSize]

# Example:
./sendFile test.txt 127.0.0.1:8080         # Local transfer, default buffer
./sendFile test.txt 192.168.1.100:8080     # Remote transfer
./sendFile test.txt 127.0.0.1:8080 1024    # Custom buffer size
```
