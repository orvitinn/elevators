elevators
=========

Elevator project in the course Programming supercomputer B.

Description:
We use two nodes for elevators and three nodes for the floors. When a person stops working, it is sent to a random elevator. 
We use the fact that MPI queues up io requests, so if the elevator is busy, the person just waits.

An elevator transports a person to either of the other floors by sending it to the floor nodes. Only one person can occupy an 
elevator at any moment.


Status:
Model is up and running, persons travel between floors with the elevators
Saves the state correctly to a binary file.
Uses relative time to decide when to quit.
Put elevator loop in to a "game loop" like the floor loop/logic. 
Very simple reading code disabled in this file.
