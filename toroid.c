/*
Program implementuje komunikację asynchroniczną między procesami w topologii toroidalnej za pomocą MPI 
Toroidalna topologia oznacza, że procesy są uporządkowane w siatce, a procesy z krawędzi mają sąsiadów
 po przeciwnych stronach, co przypomina torusa.
*/
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define N 4 // Liczba procesów w jednym wierszu/kolumnie (Program należy uruchomiić z N*N procesami)


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    //Uzyskuje identyfikator procesu (rank) w komunikatorze.
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // Uzyskuje całkowitą liczbę procesów w komunikatorze (size).
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (size != N*N) {
        if (rank == 0) {
            printf("Program wymaga uruchomienia z %d procesami!\n", N*N);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    
     // Zmienne przechowujące identyfikatory sąsiadów procesu w toroidalnej siatce: górę, prawo, lewo i dół.
     int up, right, left, down;
     // Zmienne przechowujące identyfikatory procesów, od których odbieramy wiadomości (z góry i z prawej).
     int recv_from_up, recv_from_right;
     //Tablica do przechowywania uchwytów do operacji asynchronicznych (operacje wysyłania i odbierania).
     //MPI_Request identyfikuje nieblokujące operacje wysyłania i odbioru, używane później w MPI_Wait
     MPI_Request request[4];
     //Tablica do przechowywania statusów operacji asynchronicznych
     //Przechowuje informacje o zakończonej operacji, np. źródło komunikatu, tag, rozmiar danych.
     MPI_Status status[4];
     
     int row = rank / N; //Liczba określająca wiersz procesu w siatce.
     int col = rank % N; //Liczba określająca kolumnę procesu w siatce.
     
     right = (col+1)%N + row*N; //Sąsiad po prawo
     up =  ((row - 1 + N) % N) * N + col; // Sąsiad powyżej
     left = (col - 1 + N) % N + row * N; // Sąsiad lewo
     down = ((row + 1) % N) * N + col; // Sąsiad niżej
     
    //Asynchroniczna wysyłka identyfikatora procesu
    //Każdy proces wysyła swój rank do procesu po prawej i poniżej 
    //Wyjaśnienie toroid.png
 
    //MPI_Isend(adres_zmiennej_ktora_wysylamy,ilosc_wyslanych_danych,typ_danych,odbiorca,tag_komunikacji,komunikator,adres_do_elementu_tablicy_MPI_REQUEST_pozwalający_śledzić_stan_operacji_wysyłania)
     MPI_Isend(&rank, 1, MPI_INT, right, 0, MPI_COMM_WORLD, &request[0]);
     MPI_Isend(&rank, 1, MPI_INT, up, 1, MPI_COMM_WORLD, &request[1]);
     
     //Asynchroniczny odbiór identyfikatora procesu. Odbieramy od sąsiadów po lewej i dołu
     MPI_Irecv(&recv_from_right, 1, MPI_INT, left, 0, MPI_COMM_WORLD, &request[2]); //odbiór z lewej
     MPI_Irecv(&recv_from_up, 1, MPI_INT, down, 1, MPI_COMM_WORLD, &request[3]); //odbiór z dołu
     
     //czeka na zakończenie wszystkich czterech operacji asynchronicznych (wysyłania i odbierania). 
     // request to tablica uchwytów, a status to tablica statusów operacji.
     MPI_Waitall(4, request, status);
     
     printf("Proces %d wysłał wiadomość do %d (prawa) i do %d (góra)\n", rank, right, up);
     printf("Proces %d odebrał wiadomość od %d (lewa) i od %d (dół)\n", rank, recv_from_right, recv_from_up);
    
    MPI_Finalize();
    return EXIT_SUCCESS;
}

//wlaczenie z wlasnego pc: vpn politechniki-> putty -> mpi.cs.put.poznan.pl -> login i haslo do konta unixlab -->
// -> ssh lab-cad-X -> uruchamianie z lab-cad bo na serwerze mpi cos rozjebane jest i nie da sie uruchomic 
// procesow na kilku pc
//mpirun -np 36 ./toroid
//mpirun -hosts lab-cad-7,lab-cad-12,lab-cad-15,lab-cad-15 -np 16 ./toroid
