#ifndef TESTERS
#define TESTERS

#include <stdbool.h>

#include "labyrinth_structure.h"
#include "err.h"

bool validate_starting_or_finish_point(labyrinth lab, size_t* point, size_t dim);
bool test_if_empty(labyrinth lab, size_t* point);
bool check_eof();
bool check_endline();
void check_last_line();
void* safe_malloc(size_t size);
void* safe_realloc(void* ptr, size_t size);
void free_allocated();

#endif /*TESTERS*/