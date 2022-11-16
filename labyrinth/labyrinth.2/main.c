#include <stdio.h>
#include <stdlib.h>

#include "labyrinth_structure.h"
#include "insert_data.h"
#include "bfs.h"
#include "queue.h"
#include "err.h"

int main() {
    // Zapewnienie zwolnienia pamięci przy zakończeniu programu
    atexit(free_allocated);

    // Wczytanie rozmiarów labiryntu i odpowiednich wartości pochodnych.
    labyrinth lab;
    size_t check_dimension = 0, k = 0;
    lab.size = read_size_or_coordinates(&k, 1);
    lab.dimension = k;
    calculate_tiles(&lab);

    // Wczytanie punktu startowego.
    size_t* starting_point = read_size_or_coordinates(&check_dimension, 2);
    if (!validate_starting_or_finish_point(lab, starting_point, check_dimension))
        err(2);

    // Wczytanie punktu końcowego.
    size_t* ending_point = read_size_or_coordinates(&check_dimension, 3);
    if (!validate_starting_or_finish_point(lab, ending_point, check_dimension))
        err(3);

    // Wczytanie linii opisującej ściany i sprawdzenie poprawności danych wejściowych.
    read_walls(&lab);
    check_last_line();

    // Obliczenie i wypisanie wyniku.
    size_t wynik = bfs(lab, starting_point, ending_point);
    printf("%lu\n", wynik);
}