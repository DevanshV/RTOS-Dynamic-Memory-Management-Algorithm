# Dynamic Memory Management Algorithm
Completed for the "Introduction to Computer Structures &amp; Real-Time Systems" course.

In this project, we implemented a memory allocation strategy called half-fit. The half-fit algorithm runs in constant time, making it suitable for memory management in real-time systems. It was developed by Takeshi Ogasawara at IBM Japan.

## Objectives

* Use array and linked list data structures
* Use bit-wise operators in C
* Track memory allocation and deallocation using an algorithm suitable for a real-time system

## Functions

```void half_init();```
Initializes the system with a single block of size 32768 bits (32 KiB). 

```void *half_alloc( unsigned int n );```
Allocation function that takes an integer as an argument and returns a block of memory of the appropriate size.

```void half_free( void * );```
De-allocation function that takes a block of memory and re-integrates it back into the memory pool. If the freed memory block is adjacent to other currently freed memory blocks, it is merged with them and the combined block is then re-integrated into the memory pool.

## Half-fit Algorithm Overview

* In the half-fit algorithm, a block is a unit of memory which is either allocated or unallocated. Each block has a corresponding header that includes a size as well as additional information needed to manage the memory.

* The algorithm sorts unallocated blocks of memory into buckets, based on the size of the block. Each bucket can hold blocks in the range of (2<sup>i</sup>) to (2<sup>i+1</sup> − 1) for `i = 0, 1, 2,...` with a separate bucket corresponding to each i. 
__As a result, the final bucket will contain blocks in the size range of 2<sup>9</sup> = 512 to 2<sup>9+1</sup> = 1024__

* In order to check if a bucket is empty, a bit-vector is used, making it possible to immediately determine if the request can be satisfied without walking any lists. If the bucket is empty (the corresponding bit in the bit vector is 0), the next largest bucket is checked. The algorithm continues to check the next largest bucket until either a non-empty bucket is found or we run out of buckets to check.

## Files

__half_fit.c__
Skeleton file containing empty functions with signatures expected by the test file. You will need to code these functions, and may add any additional functions required by your implementation.

__half_fit.h__
The associated header file for half_fit.c. Contains the function prototypes for the project. You may define additional stucts here if required by your implementation.

__half_fit_test.c__
The main function is implemented here. This file also contains code to test your half-fit algorithm. You should not make any modifications to this file.

__Retarget.c__
This file is needed to direct the printf outputs to the terminal.

__type.h__
Contains definitions for the types used in this, as well as all future, projects.

__uart.c__
Implements a Universal Asynchronous Receiver Transmiter (UART). 

__uart.h__
The associated header file for uart.c containing function prototypes and definitions.

## References

T. Ogasawara. An Algorithm with Constant Execution Time for Dynamic Storage Allocation [Online]. Available: https://people.kth.se/~roand/half_fit.pdf
