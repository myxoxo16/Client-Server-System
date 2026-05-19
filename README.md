# A Client-Server System

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

#### For testing smallTest.txt file without a buffer size (first line, default size as 4096) and with a buffer size (second line) below:

```
./sendFile smallTest.txt 127.0.0.1:8080 
./sendFile smallTest.txt 127.0.0.1:8080 4000
```
#### For testing wonderland.txt file without a buffer size (first line, default size as 4096) and with a buffer size (second line) below:
``` 
./sendFile wonderland.txt 127.0.0.1:8080
./sendFile wonderland.txt 127.0.0.1:8080 4000
```
