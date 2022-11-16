#ifndef BFS
#define BFS

#include "labyrinth_structure.h"
#include "queue.h"
#include "bit.h"
#include "err.h"

size_t bfs(labyrinth lab, size_t* starting_index, size_t* finish_index);

#endif /* BFS */