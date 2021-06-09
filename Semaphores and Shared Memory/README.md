# Unix/Linux Semaphores and Shared Memory; EDF and LLF Scheduling
This program simulates an airline reservations system with different agents connected to the airline’s central computer. The purpose is to implement several programs with Unix/Linux semaphores and shared memory.

## Description
An agent can make reservations, ticketing (selling of seats), and cancellations. Two or more agents may do the same transactions at around the same time. Such transactions must be handled atomically (mutual exclusion). 

An agent cannot reserve a seat that is already taken. And cannot operate on a non-existent flight or seat.

There is an option to 'wait' for a selected seat if this seat is currently unavailable. If the seat later becomes available as a result of a ‘cancel’ transaction by another passenger, then this seat will be sold to the passenger.

The ‘waitany’ transaction is like ‘wait’ except that at the end if no one cancels the requested seat, then the airline reservation system assigns another available seat.

## Implementation
Each C++ file treats an agent as a process and uses process scheduling methods to order the execution of transactions; Earliest Deadline First, First Come First Serve, and Least Laxity First.




