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

### For testing smallTest.txt file:

``` 
gcc -Wall -g -o sendFile client.c
./sendFile smallTest.txt localhost:8080 4096  
```
#### For testing wonderland.txt file:
``` 
gcc -Wall -g -o sendFile client.c
./sendFile wonderland.txt localhost:8080 4096  
```
