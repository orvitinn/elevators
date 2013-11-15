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

const int ITERATIONS = 20;

double rand_range(double min_n, double max_n)
{
    return (double)rand()/RAND_MAX * (max_n - min_n) + min_n;
}




// return a value between min and max, but not the number exclude (min <= r < max && r != exclude)
int rand_range_exclude(int exclude, int min, int max)
{
    int random = rand_range(min, max-1) + min;
    if (random >= exclude) {
        random++;
    }
    return random;
}


class Person {
public:
    Person(int new_id)
    {
        id = new_id;
        state = New;
    }
    void start_work()
    {
        t = MPI_Wtime() + rand_range(1, 4);
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


void simulateElevator(int rank, MPI_Comm& lyftuhopur) {
    cout << "Elevator " << rank << " running." << endl;
    // elevator simulation
    int person;
    // rank 2,3,4 are floors 1,2,3
    vector<int> floors = {2,3,4};
    MPI_Status status;
    bool run_simulation = true;
    int count = 0;
    
    // File io
    MPI_File fh;
    MPI_Info info;
    // char filename[] = "/home/maa33/code/elevators/elevator_dump.bin";
    char file_name[] = "/home/maa33/code/elevators/data.out";
    MPI_Info_create(&info);
    char buffer[4];
    
    int rc = MPI_File_open(lyftuhopur, file_name, MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_SEQUENTIAL, MPI_INFO_NULL, &fh);
    // int rc = MPI_File_open( MPI_COMM_WORLD, file_name, MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_SEQUENTIAL, MPI_INFO_NULL, &out_file);

    cout << "MPI_File_open returned " << rc << " on " << rank << endl;
    
    while(run_simulation) {
        cout << ". " << endl;
        ::MPI_Recv(&person, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        cout << "-" << endl;
        
        if (person == -1)
        {
            cout << "Elevator " << rank << "got a quit message. " << endl;
            break;
        }
        // send the person to either of the other floors
        int source = status.MPI_SOURCE;
        // choose new floor for person
        int destination = get_next_floor(source);
        int movingtime = abs(source - destination);
        
        // write to the binary file when a person enters the elevator
        // IO layout. Each record 32 bits
        // person id, action, elevator, floor
        //    8 bit   8 bit    8 bit    8 bit
        // action
        // 1: entered elevator
        // 2: left elevator
        buffer[0] = static_cast<char>(person);
        buffer[1] = 1;
        buffer[2] = static_cast<char>(rank);
        buffer[3] = static_cast<char>(source);
        MPI_File_write(fh, buffer, 4, MPI_CHAR, &status);
        
        cout << "Elevator " << rank << " sending person " << person << " from floor " << source << " to floor " << destination<< ", taking " << movingtime << " seconds." << endl;
        
        // elevator is now moving
        // usleep(movingtime*10000);
        
        ::MPI_Send(&person, 1, MPI_INT, destination, 0, MPI_COMM_WORLD);
        
        cout << "Elevator " << rank << " empty." << endl;
        
        // write again when person has left the elevator (and entered a floor)
        buffer[1] = 2;
        buffer[3] = static_cast<char>(destination);
        MPI_File_write_ordered(fh, buffer, 4, MPI_CHAR, &status);
        
        if (count++ > ITERATIONS)
            run_simulation = false;
    }
    
    rc = MPI_File_close(&fh);
}

void simulateFloor(int rank) {
    // floor simulation
    vector<Person> persons;
    vector<int> elevators = {0,1};

    
    MPI_File fh;
    MPI_Info info;
    // char filename[] = "/home/maa33/code/elevators/elevator_dump.bin";
    char file_name[] = "/home/maa33/code/elevators/data.out";
    MPI_Info_create(&info);
    char buffer[4];
    
    
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
        for (auto it = persons.begin(); it != persons.end();)
        {
            it->simulate();
            if (it->state == Moving)
            {
                cout << "Person " << it->id << " done working, waiting for elevator on floor " << rank << endl;
                // send person to elevator, this blocks so if any other persons wants an elevator, they will wait
                int elevator = rand_range(0, 2);
                ::MPI_Send(&it->id, 1, MPI_INT, elevator, 0, MPI_COMM_WORLD);
                it->state = Gone;
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
    
    // sendum á báðar lyftur að við séum að fara að hætta... lesum líka ef lyftur eru að reyna að senda okkur
    // allt async þar sem okkur er sama um gildið - við erum hætt!
    int quit_message = -1;
    int answer;
    MPI_Request request2,request3;
    ::MPI_Isend(&quit_message, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);
    ::MPI_Isend(&quit_message, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &request2);
    ::MPI_Irecv(&answer, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request3);
    
    // quit all processes
    sleep(2);
    // MPI_Abort(MPI_COMM_WORLD, 0);
}


int main(int argc, char* argv[]) {
    MPI_Status status;
    // MPI_Comm comm,scomm;
    int rank, size, color, errs=0;
   
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    srand((unsigned)time(NULL) + rank*100);
    
    
    // búum til com fyrir lyfturnar.
    MPI_Group lyftuhopur, allir;
    MPI_Comm lyftucomm;
    vector<int> lyftur = {0, 1};

    MPI_Comm_group(MPI_COMM_WORLD, &allir);
    MPI_Group_incl(allir, 2, &lyftur[0], &lyftuhopur);
    MPI_Comm_create(MPI_COMM_WORLD, lyftuhopur, &lyftucomm);
    
    if (rank < 2)
    {
        simulateElevator(rank, lyftucomm);
    }
    else
    {
        simulateFloor(rank);
    }
    
    cout << "Node " << rank << " stopping." << endl;
    
    MPI_Finalize();
}
