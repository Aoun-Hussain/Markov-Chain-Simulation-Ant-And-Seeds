/*
 References:
     https://www.cplusplus.com/reference/thread/thread/operator=/
     https://www.geeksforgeeks.org/rounding-floating-point-number-two-decimal-places-c-c/
     https://www.gormanalysis.com/blog/random-numbers-in-cpp/
     https://euler.stephan-brumme.com/280/
*/

#include <algorithm>
#include <iostream>
#include <string>
#include <fstream>
#include <cmath>
#include <thread>
#include <map>
#include <vector>
#include <bitset>
using namespace std;

long double globalAverage = 0;      //global variable to keep track of the average steps
unsigned long long globalRuns = 0;  //global variable to keep track of convergence runs per thread

long double round6(long double var)
{
    /*
    function to round off the result to six decimal places
    takes the float and returns rounded to 6 decimal places
    */
    long double value = (int)((var * 1000000) + .5);  //use 100 for 2 decimal places and respectively
    return (long double)(value / 1000000);
}

int antWalk()
{
    /*
    function to run the complete ant random walk simulation multiple times
    called once by each thread
    takes runsPerThread as an argument and runs the simulation runsPerThread times
    adds the average ant moves aggregated over all runs to the global average variable
    returns the move/direction of ant
    */

    /*
    The idea for logic for this antWalk was taken from: https://euler.stephan-brumme.com/280/
    For optimization, a hash function was used to uniquely identify a state
    The idea is to make a strut named state which identifies the current position of ant, the
    seeds in the top and bottom row, whether the ant carries a seed or not, and the current state
    (using a hash function)
    all positions are zero-indexed, i.e. (0,0) to (4,4)
    top row can be identified by y=0
    */

    /*
    For more optimization: After a certain number of moves the probability rapidly
    approaches zero so the program aborts, to avoid low values
    of probabilities that are possible but extremely unlikely to reach the final state.
    */

    /*
    This code uses a markov chain model where independent states are constructed
    */

    //5x5 grid
    const unsigned int gridSize = 5;

    //unique identifier of a state
    typedef unsigned int Hash;

    struct state
    {
        //ant's position
        unsigned int x;
        unsigned int y;


        bool carrySeed; //true if ant is currently carrying a seed

        unsigned int movedSeeds = 5; //5 seeds need to be transported

        //true if seed at (x, 0)
        std::bitset<gridSize> seedsTopRow;

        //true if seed at (x, 4)
        std::bitset<gridSize> seedsBottomRow;

        //default constructor with initialization list to speed up
        state() : x(0), y(0), carrySeed(false), seedsTopRow(0), seedsBottomRow(0) {}

        //hash function as a unique identifier for each state
        Hash getHash() const
        {
            //17 bits: xxxyyyctttttbbbbb
            Hash hash = carrySeed ? 1 : 0;
            hash = (hash << seedsTopRow.size()) + seedsTopRow.to_ulong();
            hash = (hash << seedsBottomRow.size()) + seedsBottomRow.to_ulong();
            hash = hash * gridSize + x;
            hash = hash * gridSize + y;
            return hash;
        }

        //true if all seeds moved to top row
        bool isFinal() const
        {
            return seedsTopRow.count() == movedSeeds && !carrySeed;
        }

        // return true if state has 5 seeds
        //A total of 10270 valid states remains
        bool isValid() const
        {
            //ant can't carry a seed in top row if current square is empty
            if (y == 0 && carrySeed && !seedsTopRow[x])
            {
                return false;
            }

            //ant can't be without a seed in bottom row if current square has seed
            if (y == gridSize - 1 && !carrySeed && seedsBottomRow[x])
            {
                return false;
            }

            //if all seeds are in top row (final state) then the ant must be in the top row, too
            if (isFinal() && y != 0)
            {
                return false;
            }

            //a total of five seeds
            auto seeds = seedsTopRow.count() + seedsBottomRow.count();
            if (carrySeed)
            {
                seeds++;
            }

            if (seeds != gridSize)
            {
                return false;
            }

            // all tests passed
            return true;
        }
    };


    const auto AllBits = (1 << gridSize) - 1;

    // ant starts in the middle, all seeds in bottom row
    state initial;
    initial.x = 2;
    initial.y = 2;
    initial.carrySeed    = false;
    initial.seedsTopRow    = 0;
    initial.seedsBottomRow = AllBits;

    //generate all states
    std::map<Hash, state> states;
    for (unsigned int x = 0; x < gridSize; x++)
        for (unsigned int y = 0; y < gridSize; y++)
            for (auto carrySeed = 0; carrySeed <= 1; carrySeed++)
                for (auto maskTop = 0; maskTop <= AllBits; maskTop++)
                    for (auto maskBottom = 0; maskBottom <= AllBits; maskBottom++)
                    {
                        //generate state
                        state current;
                        current.x = x;
                        current.y = y;
                        current.carrySeed    = (carrySeed == 1);
                        current.seedsTopRow    = maskTop;
                        current.seedsBottomRow = maskBottom;

                        //only accept valid states
                        if (!current.isValid())
                        {
                            continue;
                        }

                        //add to container
                        states[current.getHash()] = current;
                    }

    //find all transitions and identify final states
    std::map<Hash, std::vector<Hash>> transitions;
    std::vector<Hash> final;

    for (auto i : states)
    {
        auto current = i.second;

        //final state ?
        if (current.isFinal())
        {
            final.push_back(i.first);

            //no need to proceed after reaching a final state (no further moves of the ant)
            continue;
        }

        std::vector<state> candidates;

        //move up
        if (current.y > 0)
        {
            current.y--;
            candidates.push_back(current);
            current.y++; //restore
        }

        //move down
        if (current.y < gridSize - 1)
        {
            current.y++;
            candidates.push_back(current);
            current.y--; //restore
        }

        //move left
        if (current.x > 0)
        {
            current.x--;
            candidates.push_back(current);
            current.x++; //restore
        }

        //move right
        if (current.x < gridSize - 1)
        {
            current.x++;
            candidates.push_back(current);
            current.x--; //restore
        }

        //drop or pick up seed
        for (auto candidate : candidates)
        {
            //drop seed ?
            if (candidate.carrySeed && candidate.y == 0 && !candidate.seedsTopRow[candidate.x])
            {
                candidate.carrySeed = false;
                candidate.seedsTopRow[candidate.x] = true;
            }

            //pick up seed ?
            if (!candidate.carrySeed && candidate.y == gridSize - 1 && candidate.seedsBottomRow[candidate.x])
            {
                candidate.carrySeed = true;
                candidate.seedsBottomRow[candidate.x] = false;
            }

            //found one more state transition
            transitions[current.getHash()].push_back(candidate.getHash());
        }
    }

    //no valid moves
    if (final.empty() || transitions.empty())
    {
        return 1;
    }

    //get highest hash value
    auto maxHash = transitions.rbegin()->first;


    int count = 0;  //to store number of iterations after the solution converges

    //initially the ant is with 100% probability in the middle of the grid
    std::vector<double> last(maxHash + 1, 0);

    for (auto x : states)
    {
        last[x.first] = 0;
    }
    last[initial.getHash()] = 1;

    //E = 1 * p(1) + 2 * p(2) + 3 * p(3) + ... + n * p(n)
    //where p(n) is the probability that a final state was reached after n steps
    double avg = 0;

    for (unsigned int steps = 1; ; steps++)
    {
        std::vector<double> next(last.size(), 0);
        for (const auto& followup : transitions)
        {
            auto from = followup.first;  //a hash
            const auto& to = followup.second; //a container with hashes

            //each step is equally likely - uniform distribution - prob
            double prob = (1.0 / to.size());
            prob *= last[from];

            //add to each allowed follow-up state
            for (auto move : to)
            {
                next[move] += prob;
            }
        }

        //overwrite old probabilities with new values
        last = move(next);

        //add probabilities of reaching a final state
        double add = 0;
        for (auto x : final)
        {
            add += last[x];
        }

        add *= steps;               //E = 1 * p(1) + 2 * p(2) + ...
        add = round6(add);
        avg += add;
        double delta = 0.000001;    //the difference is less than delta

        if ((avg > 1) && (add < delta))
        {
            count ++;
        }

        if (count == 10)                //it converges to 6 decimals when the average
        {                               //steps in 6 decimal doesn't change in the last 10 rounds
            globalRuns += steps;
            globalAverage += avg;  //add final expectation of each thread to the global average
            return 0;              //avoiding warning to return something
        }
    }
}

int main()
{
    /*
    main function to call all functions and perform multi-threading and print output to
    ProblemOne.txt.
    */
    unsigned long totalThreads = thread::hardware_concurrency();

    thread threads[totalThreads];

    for (int i = 0; i < totalThreads; ++i)
    {
        threads[i] = thread(antWalk);      //move-assign threads
    }

    for (int i = 0; i < totalThreads; ++i)
    {
        threads[i].join();
    }

    globalAverage = globalAverage/totalThreads;                  //taking final average of all threads
    globalAverage = round6(globalAverage);                       //rounding off to 6 decimal places
    unsigned long long totalRuns = globalRuns/totalThreads;      //runs needed to converge for each thread

    //writes output to the file
    ofstream myfile ("ProblemOne.txt");
    if (myfile.is_open())
    {
        myfile << "Number of threads created: " << to_string(totalThreads) << "\n" << "\n";
        myfile << "Expected number of steps: " << to_string(globalAverage) << "\n" << "\n";
        myfile << "Total number of runs needed for solution convergence: " << to_string(totalRuns) << "\n";
        myfile.close();
    }
    else cout << "Unable to open file";

    return 0;
}
