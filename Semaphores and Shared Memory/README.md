# Unix/Linux Semaphores and Shared Memory; EDF and LLF Scheduling
This program simulates an airline reservations system with different agents connected to the airline’s central computer. The purpose is to implement several programs with Unix/Linux semaphores and shared memory.

## Description
An agent can make reservations, ticketing (selling of seats), and cancellations. Two or more agents may do the same transactions at around the same time. Such transactions must be handled automatically (mutual exclusion). 
An agent cannot reserve a seat that is already taken. And cannot operate on a non-existent flight or seat.

