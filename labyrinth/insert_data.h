#ifndef INSERT_DATA
#define INSERT_DATA

#include <stdbool.h>

#include "err.h"
#include "labyrinth_structure.h"
#include "testers.h"
#include "bit.h"

size_t* read_size_or_coordinates(size_t* k, int line);
void calculate_tiles (labyrinth* lab);
void read_walls(labyrinth* lab);
void print_lab(labyrinth lab);
void print_structure(labyrinth lab);
void free_lab(labyrinth* lab);
void check_emptiness();
void free_memory_table_insert();

#endif /* INSERT_DATA */