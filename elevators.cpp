#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <ctime>
#include <vector>
#include <string>

#include "mpi.h"

using std::cout;
using std::endl;
using std::time;
using std::vector;

enum State { New, Working, Moving, Gone };

const int ITERATIONS = 10;

class Person {
public:
    Person(int new_id)
    {
        id = new_id;
        state = New;
    }
    void start_work()
    {
        t = MPI_Wtime() + rand_range_exclude(1, 4);
        state = Working;
    }
    
    void simulate()
    {
        if (state == Working)
        {
            // hve mikill tími eftir af "working"?
            int now = MPI_Wtime();
            if (t < now)
            {
                state = Moving;
            }
        }
    }

    State state;
    int id;
private:
    double t;
};

// return a value between min and max, but not the number exclude (min <= r < max && r != exclude)
int rand_range_exclude(int exclude, int min, int max)
{
    int random = rand_range(min, max-1) + min;
    if (random >= exclude) {
        random++;
    }
    return random;
}

double rand_range(double min_n, double max_n)
{
    return (double)rand()/RAND_MAX * (max_n - min_n) + min_n;
}


int get_next_floor(int current_floor)
{
    int random_value = rand() % 2;
    if (current_floor == 2)
        return random_value ? 3: 4;
    if (current_floor == 3)
        return random_value ? 2: 4;
    if (current_floor == 4)
        return random_value ? 2: 3;
}

void simulateElevator(int rank) {
    // elevator simulation
    int person;
    // rank 2,3,4 are floors 1,2,3
    vector<int> floors = {2,3,4};
    MPI_Status status;
    bool run_simulation = true;
    int count = 0;
    while(run_simulation) {
        ::MPI_Recv(&person, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        // send the person to either of the other floors
        int source = status.MPI_SOURCE;
        if (person == -1)
            break;
        
        // choose new floor for person
        int destination = get_next_floor(source);
        int movingtime = abs(source - destination);
        
        cout << "Elevator " << rank << " sending person " << person << " from floor " << source << " to floor " << destination<< ", taking " << movingtime << " seconds." << endl;
        
        // elevator is now moving
        sleep(movingtime);
        
        ::MPI_Send(&person, 1, MPI_INT, destination, 0, MPI_COMM_WORLD);
        if (count++ > ITERATIONS)
            run_simulation = false;
    }
}

void simulateFloor(int rank) {
    // floor simulation
    vector<Person> persons;
    vector<int> elevators = {0,1};
    
    // lesa nöfn úr skrá með mpi_io - en fyrst - setja inn einhver gildi
    if (rank == 2) {
        persons.push_back(Person(1));
        persons.push_back(Person(2));
    }
    else if (rank == 3)
    {
        persons.push_back(Person(3));
        persons.push_back(Person(4));
    }
    else if (rank == 4)
    {
        persons.push_back(Person(5));
        persons.push_back(Person(6));
    }
    for (Person& p: persons)
        p.start_work();
    
    int incoming_person_id;
    MPI_Request request;
    MPI_Status status;
    
    bool run_simulation = true;
    // check for new persons on floor
    ::MPI_Irecv(&incoming_person_id, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request);
    int count = 0;

    while (run_simulation)
    {
        for (Person& p: persons)
        {
            p.simulate();
            if (p.state == Moving)
            {
                cout << "Person " << p.id << " done working, waiting for elevator on floor " << rank << endl;
                // send person to elevator
                int elevator = rand_range(0, 2);
                ::MPI_Send(&p.id, 1, MPI_INT, elevator, 0, MPI_COMM_WORLD);
                p.state = Gone;
            }
        }
        
        // remove all persons with state == Moving from the vector
        for (auto it = persons.begin(); it != persons.end();)
        {
            if (it->state == Gone)
            {
                it = persons.erase(it);
            }
            else
            {
                it++;
            }
        }
        
        int something_read=0;
        MPI_Test(&request, &something_read, &status);
        if (something_read)
        {
            cout << "Person " << incoming_person_id << " entered floor " << rank << endl;
            Person p(incoming_person_id);
            p.start_work();
            persons.push_back(p);
            ::MPI_Irecv(&incoming_person_id, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request);
        }
        
        if (count++ > ITERATIONS)
            run_simulation = false;
        
        usleep(500000);
        // optional: sleep until next person is done working
    }
    
    // quit all processes
    MPI_Abort(MPI_COMM_WORLD, 0);
    
    // implement a very basic elevator and a very basic person - basic notion is try out MPI-IO.
}


int main(int argc, char* argv[]) {
    MPI_Status status;
    MPI_Comm comm,scomm;
    int rank, size, color, errs=0;
   
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    srand((unsigned)time(NULL) + rank*100);
    
    if (rank < 2)
    {
        simulateElevator(rank);
    }
    else
    {
        simulateFloor(rank);
    }
    
    cout << "Node " << rank << " stopping." << endl;
    
    MPI_Finalize();
}
