#ifndef LAB_STRUCT
#define LAB_STRUCT

#include <stdbool.h>
#include <ctype.h>
#include <inttypes.h>

typedef struct labyrinth {
    size_t dimension;       // wymiar labiryntu
    size_t tiles;           // ilość komórek w labiryncie
    size_t* size;           // tablica ścian z rozmiarami labiryntu w danym wymiarze
    uint8_t* walls;         // Tablica bitów w której 1 oznacza ścianę na komórce 
                            // o danym indeksie w labiryncie, a 0 jej brak.
} labyrinth;

#endif /* LAB_STRUCT */