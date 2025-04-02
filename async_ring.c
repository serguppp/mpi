#include <stdio.h>
#include <mpi.h>

int main(int argc, char** argv) {
    int my_rank, np, left_neighbor, right_neighbor, data_received, tag = 0;
    MPI_Status statSend, statRecv;
    MPI_Request reqSend, reqRecv;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    left_neighbor = (my_rank - 1 + np) % np;
    right_neighbor = (my_rank + 1) % np;

    MPI_Irecv(&data_received, 1, MPI_INT, right_neighbor, tag, MPI_COMM_WORLD, &reqRecv);
    MPI_Isend(&my_rank, 1, MPI_INT, left_neighbor, tag, MPI_COMM_WORLD, &reqSend);


    printf("Process %d received from right neighbor: %d\n", my_rank, data_received);

    MPI_Finalize();
    return 0;
}