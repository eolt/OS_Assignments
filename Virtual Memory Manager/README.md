# Virtual Memory Manager
This program will simulate the operations of a virtual memory manager with paged segments. There will be sperate processors requesting pages from the same disk. 

## Implementation 
There are different C++ files which follow a page replacment algorithm: first in first out, least recently used, most frequently used, and working set. 

LRU-X is like LRU except it replaces at x page from our queue. And OPT is optimum with look ahead of x future page requests. 

The program uses data structures such as queue and arrays to simulate a disk driver, page fault handler, or page frame.


## Input 

The input file will list the total number of page frames, maximum number of pages, page size (in number of bytes), numbr of page frames per processs, lookahead x, min free pool size, max free pool size, and total number of processes.

input.txt follows
```

8
16
256
4
3
0
5
4
101 8
102 9
103 15
104 11
101 0001010010110110B
102 0010010010001100B
102 0011010010001111B
103 0100101111000000B
101 0001010010110100B
102 0010010010011000B
102 -1
101 0001010110110100B
101 -1
104 0110000110000000B
104 -1
103 0100101101001111B
103 -1
```
Note each processor has an id (101, 102, 103) followed by the page address requested. 

## Output
The output is printted to the console after completion. 

```
Number of page faults for each process:
Process 101: 2;
Process 102: 2;
Process 103: 1;
Process 104: 1;

Working set: min size: 0; max size: 6.
```



