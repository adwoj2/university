#include <stdio.h>
#include <stdlib.h>

#include "queue.h"
#include "err.h"

/* Funkcja tworząca jeden element kolejki zawierający podaną wartość
i zwracająca wskaźnik na stworzony element.
*/ 
q_node* create_node(size_t index, bool if_decoy) {
    q_node* q = (q_node*)malloc(sizeof(q_node));
    if (q == NULL)
        return NULL;

    q->index = index;
    q->next = NULL;
    q->previous = NULL;
    q->decoy = if_decoy;

    return q;
}

/* Funkcja pobierająca wskaźnik na koniec kolejki oraz wartość i tworząca nowy 
element kolejki na końcu. Dodatkowo funkcja pobiera wartość logiczną czy nowo
stworzony element ma służyć jako atrapa.
*/
q_node* push_back(q_node* tail, size_t value, bool if_decoy, q_node* head) {
    q_node* q = (q_node*)malloc(sizeof(q_node));
    if (q == NULL) {
        free_queue(head);
        err(0);
    }
        
    q->index = value;
    q->next = tail;
    tail->previous = q;
    q->previous = NULL;
    q->decoy = if_decoy;

    return q;
}

/* Funkcja przyjmująca wskaźnik na początek kolejki i zwracająca pierwszy element
kolejki jednocześnie usuwając go z pamięci drugiego elementu.
*/
q_node pop(q_node* head) {
    (head->previous)->next = NULL;
    return *head;
}

/* Procedura zwalniająca pamięć zajmowaną przez kolejkę pobierając wskaźnik 
na jej początek.
*/
void free_queue(q_node* head) {
    q_node* prev = head->previous;
    free(head);
    if (prev != NULL)
        free_queue(prev);
}

