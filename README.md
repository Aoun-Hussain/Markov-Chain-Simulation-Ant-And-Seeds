This project consists of the cpp source file which simulates the random walk of an ant on a 5x5 grid
where it picks seeds from the bottom row and drops it on the first row. The ant stops walking when all 
the seeds from the bottom row has been placed in the top row. 

The program uses concurrent threads to run this simulation multiple times until the average number of
ant's moves per simulation converges to a number with 6 decimal places. The program then outputs the number 
of threads created, average number of ant's moves, and the number of simulations/run, in the output
txt file.

The markov chain based simulation in mutil-threading takes approximately 5 seconds to run a total of 1700 
times with an expected 430.xxxxxx moves per simulation/run.
