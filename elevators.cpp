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

enum State { New, Working, Moving, Gone, Reading, Writing };

const int RUNTIME_SECONDS = 10.0;
double START;

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
    int person;
    MPI_Status status;
    MPI_Request request, iorequest_1, iorequest_2;
    bool run_simulation = true;
    int count = 0;
    
    MPI_File fh;
    char file_name[] = "/home/maa33/code/elevators/data.out";
    char buffer[4];
    
    int rc = MPI_File_open(lyftuhopur, file_name, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fh);
    // int rc = MPI_File_open( MPI_COMM_WORLD, file_name, MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_SEQUENTIAL, MPI_INFO_NULL, &out_file);
    
    int destination;

    cout << "MPI_File_open returned " << rc << " on " << rank << endl;
    State state = Reading;
    ::MPI_Irecv(&person, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request);
    while((MPI_Wtime() - START) < RUNTIME_SECONDS) {
        usleep(100000); // to reduce cpu time - work every 100ms
        if (state == Reading)
        {
            int something_read=0;
            MPI_Test(&request, &something_read, &status);
            if (something_read)
            {
                cout << "Person " << person << " entered elevator " << rank << endl;
                int source = status.MPI_SOURCE;
                destination = get_next_floor(source);
                int movingtime = abs(source - destination);
                cout << "Elevator " << rank << " writing to file." << endl;
                buffer[0] = static_cast<char>(person);
                buffer[1] = 1;
                buffer[2] = static_cast<char>(rank);
                buffer[3] = static_cast<char>(source);
                // MPI_File_write_ordered(fh, buffer, 4, MPI_CHAR, &status);
                MPI_File_write_shared(fh, buffer, 4, MPI_CHAR, &status);
                // MPI_File_write(fh, buffer, 4, MPI_CHAR, &status);
                
                cout << "Elevator " << rank << " sending person " << person << " from floor " << source << " to floor " << destination<< ", taking " << movingtime << " seconds." << endl;
                
                // elevator is now moving
                usleep(movingtime*10000);
                
                ::MPI_Isend(&person, 1, MPI_INT, destination, 0, MPI_COMM_WORLD, &request);
                state = Writing;
            }
        }
        else {
            // state = Writing
            int something_written=0;    
            MPI_Test(&request, &something_written, &status);
            if (something_written)
            {
                cout << "Elevator " << rank << " writing to file." << endl;
                buffer[0] = static_cast<char>(person);
                buffer[1] = 2;
                buffer[2] = static_cast<char>(rank);
                buffer[3] = static_cast<char>(destination);
                // ::MPI_File_write_ordered(fh, buffer, 4, MPI_CHAR, &status);
                ::MPI_File_write_shared(fh, buffer, 4, MPI_CHAR, &status);
                // ::MPI_File_write_ordered(fh, buffer, 4, MPI_CHAR, &status);
                cout << "Person " << person << " left the elevator on floor " << destination << endl;
                ::MPI_Irecv(&person, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request);
                state = Reading;
            }
        }
    }

    cout << "Elevator " << rank << " closing." << endl;
    MPI_Barrier(lyftuhopur);
    cout << "Fin. " << rank << endl;
    rc = MPI_File_close(&fh);
}

void simulateFloor(int rank, MPI_Comm& haedahopur) {
    MPI_Comm_rank(haedahopur, &rank);
    double start = MPI_Wtime();

    // floor simulation
    vector<Person> persons;
    vector<int> elevators = {0,1};

    char buf[14];
    
    MPI_File fh;
    MPI_Info info;
    MPI_Status status;
    
#if 0
    char file_name[] = "/home/maa33/code/elevators/names.txt";
    char native[] = "native";
    int rc = MPI_File_open( haedahopur, file_name, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
    MPI_File_set_view(fh, rank*sizeof(char)*15, MPI_CHAR, MPI_CHAR, native, MPI_INFO_NULL);
    MPI_File_read(fh, buf, 14, MPI_CHAR, &status);

    cout << rank << " read name from file: " << buf << endl;
    
    rc = MPI_File_close(&fh);
#endif
    // lesa nöfn úr skrá með mpi_io - en fyrst - setja inn einhver gildi
    if (rank == 0) {
        persons.push_back(Person(1));
        persons.push_back(Person(2));
    }
    else if (rank == 1)
    {
        persons.push_back(Person(3));
        persons.push_back(Person(4));
    }
    else if (rank == 2)
    {
        persons.push_back(Person(5));
        persons.push_back(Person(6));
    }
    for (Person& p: persons)
        p.start_work();
    
    int incoming_person_id;
    MPI_Request request, send_request;
    int cnt = 0;

    
    bool run_simulation = true;
    
    // check for new persons on floor
    ::MPI_Irecv(&incoming_person_id, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request);
    int count = 0;
    int send_tmp;

    while((MPI_Wtime() - START) < RUNTIME_SECONDS)
    {
        for (auto it = persons.begin(); it != persons.end();)
        {
            it->simulate();
            if (it->state == Moving)
            {
                cout << "Person " << it->id << " done working, waiting for elevator on floor " << rank << endl;
                // send person to elevator, this blocks so if any other persons wants an elevator, they will wait
                int elevator = rand_range(0, 2);
                send_tmp = it->id;
                ::MPI_Isend(&send_tmp, 1, MPI_INT, elevator, 0, MPI_COMM_WORLD, &send_request);
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
        
        usleep(100000);
        // optional: sleep until next person is done working
    }
    
    // sendum á báðar lyftur að við séum að fara að hætta... lesum líka ef lyftur eru að reyna að senda okkur
    cout << "Simulation on floor " << rank << " now finally finished quitting." << endl;
}


int main(int argc, char* argv[]) {
    MPI_Status status;
    // MPI_Comm comm,scomm;
    int rank, size, color, errs=0;
   
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    srand((unsigned)time(NULL) + rank*100);

    START = MPI_Wtime();

    
    // búum til com fyrir lyfturnar.
    MPI_Group lyftuhopur, haedahopur, allir;
    MPI_Comm lyftucomm, haedacomm;
    vector<int> lyftur = {0, 1};
    vector<int> haedir = {2, 3, 4};

    MPI_Comm_group(MPI_COMM_WORLD, &allir);
    
    MPI_Group_incl(allir, 2, &lyftur[0], &lyftuhopur);
    MPI_Comm_create(MPI_COMM_WORLD, lyftuhopur, &lyftucomm);

    MPI_Group_incl(allir, 3, &haedir[0], &haedahopur);
    MPI_Comm_create(MPI_COMM_WORLD, haedahopur, &haedacomm);
    
    if (rank < 2)
    {
        simulateElevator(rank, lyftucomm);
    }
    else
    {
        simulateFloor(rank, haedacomm);
    }
    
    cout << "Node " << rank << " stopping." << endl;
    MPI_Barrier(MPI_COMM_WORLD);
    cout << "Node " << rank << " stopped." << endl;
    
    MPI_Finalize();
}
