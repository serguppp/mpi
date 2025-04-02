#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>

#define N 5  // Rozmiar tablicy

int main(int argc, char *argv[]) {
    int rank, np;
    //inicjalizacja MPI
    MPI_Init(&argc, &argv);
    //przypisywanie każdemu procesowi rank
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    //pobieranie liczby procesów
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    if (np != N * N) {
        if (rank == 0) printf("Uruchom z %d procesami\n", N * N);
        MPI_Finalize();
        return 1;
    }

    int a[N] = {0,1,3,-421412,2222};  
    int wynik[N] = {0};                

    //Rozsyłanie elementów tablicy a o wymiarach NxN typu INT do każdego procesu w MPI_COMM_WORLD. Początkowo tablica jest w procesie 0.
    MPI_Bcast(a, N, MPI_INT, 0, MPI_COMM_WORLD);

    int i = rank / N;  // indeks wiersza
    int j = rank % N;  // indeks kolumny
    int pos = 0;       // pozycja po sortowaniu

    // Warunek  1)
    // Sprawdzenie, czy a[i] jest większe od a[j]. 
    // LUB
    // Warunek 2)
    // Jeśli a[i] i a[j] są równe, to sprawdzamy indeks i. 
    // Warunek 2a)
    // Najpierw sprawdzane jest, czy wartości a[i] i a[j] są równe (a[i] == a[j]). 
    // Warunek 2b)
    // Następnie sprawdzane jest, czy indeks i jest większy od indeksu j (i > j).
    // Jeżeli jeden z dwóch warunków jest spełniony, to w=1. Jeżeli żaden warunek nie jest spełniony, w=0.

    int w = (a[i] > a[j]) || (a[i] == a[j] && i > j);

    // Tworzymy nowy komunikator, w którym będą procesy podzielone na grupy według INDEKSU WIERSZA (i). 
    // Każdy proces o tym samym indeksie (i) będzie należał do jednej grupy.
    // (j) to klucz, który służy do uporządkowania procesów w obrębie grupy.
    // W każdej grupie procesy są uporządkowane według wartości j (indeksu kolumny)
    // Dla N=3 dla tablicy (2,1,3) mamy 9 procesow, ktore sa uporzadkowane w nastepujacy sposob:
    //      nr procesu                      przypisana do każdego procesu wartość w
    //     j=0 j=1 j=2                        w[i][j] 
    // i=0 0,  1,  2                          0, 1, 0
    // i=1 3,  4,  5                          0, 0, 0
    // i=2 6,  7,  8                          1, 1, 0
    // MPI_Comm_split utworzy 3 grupy procesów przechowywanych w kom_wiersza według indeksu wiersza i (0, 1, 2) 
    // o kluczach j, i będziemy mieć następujące grupy
    //      nr procesu                      przypisana do każdego procesu wartość "w" (np dla tablicy 2,1,3)
    //       i  j=0 j=1 j=2                 w[i][j] 
    // grupa 0: 0,  1,  2                   0, 1, 0
    // grupa 1: 3,  4,  5                   0, 0, 0
    // grupa 2: 6,  7,  8                   1, 1, 0
    MPI_Comm kom_wiersza;
    MPI_Comm_split(MPI_COMM_WORLD, i, j, &kom_wiersza);

    //Dla każdej grupy w zmiennej kom_wiersza sumowane są wartości "w" i suma przypisywana jest do zmiennej pos
    //MPI_Reduce(co_sumujemy,do_czego_sumujemy,ile_liczb_sumujemy,typ_zmiennej,typ_operacji,NUMER PROCESU ROOT,komunikacja)
    //NUMER PROCESU ROOT = wynik trafia TYLKO I WYŁĄCZNIE do procesu "ROOT", czyli tutaj 0.
    //OZNACZA TO, że nasza suma (przechowywana w zmiennej pos) trafi do elementów poszczególnych grup o j==0
    //       i  proces  j=0(pos)
    // grupa 0:    0    1
    // grupa 1:    3    0
    // grupa 2:    6    2
    // przy czym proces nie jest integralną częścią zmiennej kom_wiersza; kolumnę tę podano dla przejrzystości
    // Ta suma (pos) oznacza, że dla elementu a[i] jest pos liczb mniejszych
    // docelowo element a[i] trafi na pozycję numer pos w tablicy posortowanej:
    // dla i=0: proces 0 przechowuje a[0] = 2 i ma trafić (ale jeszcze nie trafia, to później) na pozycję 1
    // dla i=1: proces 3 przechowuje a[1] = 1 i ma trafić (ale jeszcze nie trafia, to później) na pozycję 0
    // dla i=2: proces 6 przechowuje a[2] = 3 i ma trafić (ale jeszcze nie trafia, to później) na pozycję 2

    MPI_Reduce(&w, &pos, 1, MPI_INT, MPI_SUM, 0, kom_wiersza);
    
    //Teraz tworzymy nowy komunikator, który dzielimy według indeksów kolumn 
    //Mamy 3 grupy procesów przechowywanych w zmiennej kom_kolumny, które są uporządkowane według klucza pos
    //Ponieważ root jest dla j==0, będzie interesować nas tylko ta grupa utworzona z kolumny j o indeksie 0
    //Procesy będą uporządkowane w grupie według pos.
    //           grupa 0 (z kolumny j==0)
    //  proces  3   0   6
    //  pos     0   1   2
    // przy czym proces nie jest integralną częścią zmiennej kom_kolumny: wiersz ten podano dla przejrzystości

    MPI_Comm kom_kolumny;
    MPI_Comm_split(MPI_COMM_WORLD, j, pos, &kom_kolumny);

    //Jeżeli j==0 (kolumna j == 0; grupa kolumna == 0), to procesy w kolumnie j==0 zbierają a[i] według
    // kolejności procesów w kom_kolumny. Przypominam, tablica wejściowa to [2,1,3]
    // pos  proces       a[i]    tablica wynik
    // 0     3(i=1 j=0)   1         [1, ?, ?] 
    // 1     0(i=0 j=0)   2         [1, 2, ?]
    // 2     2(i=2 j=0)   3         [1, 2, 3]
    // Wynik zapisze się w procesie "ROOT" o indeksie 0 (pos = 0), w tym przypadku to proces 3.
    // MPI_Gather(wsk_do_buforu_zawierajacego_dane,liczba_elementow,typ_elementow,bufor_odbierajacy_dane,liczba_elementow_do_bufora_odbierajacego,typ_elementow_do_bufora_odbierajacego,numer_procesu_root, komunikator)
    if (j == 0) {
        MPI_Gather(&a[i], 1, MPI_INT, wynik, 1, MPI_INT, 0, kom_kolumny);
    }

    //Ze względu na to, że wynik w zależności od przykładu (różnych liczb w tablicy wejściowej) może być
    //zapisany w różnych procesach, wymagane jest znalezienie procesu "ROOT" wsród komunikatora kom_kolumny
    int col_root_rank;
    MPI_Comm_rank(kom_kolumny, &col_root_rank);

    //Tablica wyjściowa będzie zapisana w którymś z procesów z kolumny j==0 (tutaj procesy 0,3,6),
    // a także który jest rootem (tutaj wypadło na proces 3)
    if (j == 0 && col_root_rank == 0) {   
        printf("Posortowana tablica: ");
        for (int k = 0; k < N; k++) printf("%d ", wynik[k]);
        printf("\n");
        
    }

    //zwalnianie zasobów i opuszczenie środowiska MPI
    MPI_Comm_free(&kom_wiersza);
    MPI_Comm_free(&kom_kolumny);
    MPI_Finalize();
    return 0;
}

