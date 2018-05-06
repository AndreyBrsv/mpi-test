//
// Created by Дмитрий Бутилов on 06.05.18.
//
#include <mpi.h>
#include <cstdio>
#include <cstdlib>

int currentRank, size;

void Capacity(int repeats, int lenght) {
    double time = 0;
    auto *message = (unsigned char *) malloc(lenght * sizeof(unsigned char));
    MPI_Status state;
    for (int j = 0; j < repeats; j++) {
        if (currentRank == 0) {
            for (int i = 1; i < size; i++) {
                double startTime = MPI_Wtime();
                MPI_Send(message, lenght, MPI_BYTE, i, 3, MPI_COMM_WORLD);
                MPI_Recv(message, lenght, MPI_BYTE, i, 4, MPI_COMM_WORLD, &state);
                time += MPI_Wtime() - startTime;
            }
        } else {
            MPI_Recv(message, lenght, MPI_BYTE, 0, 3, MPI_COMM_WORLD, &state);
            MPI_Send(message, lenght, MPI_BYTE, 0, 4, MPI_COMM_WORLD);
        }
    }
    free(message);
    if (currentRank == 0) {

        double res = 2.0 * repeats;
        printf("%.0f", res);
        auto resultCapacity = 2.0 * repeats * lenght / time;
        printf("Capasity: %.0f bs, Repeats: %d , Lenght: = %d b\n", resultCapacity, repeats, lenght);
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &currentRank);

    double time = 0;

    int capacityRepeats = 10000;
    int capacitySize = 8 * 1024 * 1024;
    if (currentRank == 0) {
        printf("Start\n");
    }
    time = MPI_Wtime();
    Capacity(capacityRepeats, capacitySize);
    time = MPI_Wtime() - time;
    if (currentRank == 0) {
        printf("Ends in %f seconds\n", time);
    }

    MPI_Finalize();
    return 0;
}