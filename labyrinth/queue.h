#ifndef QUEUE
#define QUEUE

#include <stdbool.h>

typedef struct q_node {
    size_t index;
    struct q_node* next;
    struct q_node* previous;
    bool decoy;
} q_node;

q_node* push_back(q_node* tail, size_t value, bool if_decoy, q_node* head);
q_node pop (q_node* head);
void free_queue(q_node* head);
q_node* create_node(size_t index, bool if_decoy);

#endif /* QUEUE */