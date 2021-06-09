# Unix/Linux Semaphores and Shared Memory; EDF and LLF Scheduling
This program simulates an airline reservations system with different agents connected to the airline’s central computer. The purpose is to implement several programs with Unix/Linux semaphores and shared memory.

## Description
An agent can make reservations, ticketing (selling of seats), and cancellations. Two or more agents may do the same transactions at around the same time. Such transactions must be handled atomically (mutual exclusion). 

An agent cannot reserve a seat that is already taken. And cannot operate on a non-existent flight or seat.

There is an option to 'wait' for a selected seat if this seat is currently unavailable. If the seat later becomes available as a result of a ‘cancel’ transaction by another passenger, then this seat will be sold to the passenger.

The ‘waitany’ transaction is like ‘wait’ except that at the end if no one cancels the requested seat, then the airline reservation system assigns another available seat.

## Implementation
Each C++ file treats each agent as a process. The fork() method is an accessible way to creat and run multiple processes on the program. 

An agent's commands/transacrion will be scheduled with methods such as: Earliest Deadline First, First Come First Serve, and Least Laxity First.

The program uses semaphores make transactions atomic. If two agent (processes) are issuing the same transactions. The semaphore will let one execute first and the other waits, then allowing the second after the first agent is done. 

And shared memory is implemented to allow communication amongst all the agents (processes). When a reservation is made or a cancellation, the status is updated for the rest of the agents through shared memory. 

## Input
The program inputs one text file which lists the number of flights, the seats, and number of agents followed by their transactions with deadlines. 

The input text is as follows:
```
4
SA113 6 10
AA197 2	8
DEL124 4 6
BA112 5 10
2
agent_1:
ticket 4
reserve 3
wait 2
waitany 2
cancel 3
check_passenger 4
reserve SA113 2A Tom deadline 10
wait AA197 1B Jerry deadline 8
waitany SA113 2A Sam deadline 20
check_passenger Sam deadline 30
end.
agent_2:
ticket 4
reserve 2
wait 4
waitany 2
cancel 4
check_passenger 3
reserve AA197 1B Jane deadline 15
ticket DEL124 3E Jane deadline 40
cancel SA113 2A Tom deadline 15 
end.
```

## Output
The C++ version of the files are on c++11. The output will print to the console. 

The output for EDF.cpp follows:
```
Jerry waits on AA197 1B successfully at time 2, deadline met.
Tom reserves SA113 2A successfully at time 5, deadline met.
Jane reserves AA197 1B successfully at time 7, deadline met.
Sam waitany on SA113 2A at time 9, deadline met.
Tom cancels SA113 2A at time 13, deadline met.
Sam does check at time 17, deadline met.
Jerry fails to reserve AA197 1B because it is occupied.
Sam has reserved SA113 2A at time 13(automatically resulting from waitany.)
agent_1: exits
Jane tickets DEL124 3E successfully at time 17, deadline met.
agent_2: exits
```
Of course, all computers run processes differently and this will not be the same. 
