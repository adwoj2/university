#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "bfs.h"

/* Funkcja obliczająca jakiemu indeksowi komórki w danym labiryncie odpowiadają
podane współrzędne i zwracająca wartość tego indeksu.
*/
static size_t calculate_index(labyrinth lab, size_t* point_coordinates) {
    size_t index = 0;
    size_t previous_dimensions_size = 1;

    for (size_t i = 0; i < lab.dimension; i++) {
        //-1 wynika z faktu, że indeksy są indeksowane od 0, a współrzędne od 1.
        index += (point_coordinates[i] - 1) * previous_dimensions_size;
        previous_dimensions_size *= lab.size[i];
    }
    return index;
}

/* Procedura dodająca do kolejki sąsiadów komórki o podanym indeksie.
*/
void add_neighbours(labyrinth lab, size_t current_index, 
                    q_node **q_end_ptr, q_node **q_beginning) {
    size_t previous_dimensions_size = 1;
    size_t next_dimensions_size = lab.size[0];
    for (size_t i = 0; i < lab.dimension; i++) {
        // Gdy current_index < previous_dimensions_size ta wartość będzie
        // niezdefiniowana , ale w takim wypadku nie zostanie ona użyta.
        size_t down_index = current_index - previous_dimensions_size; 
        size_t up_index = current_index + previous_dimensions_size;

        // Zapewnienie pozostania w labiryncie oraz 
        // poruszanie się tylko w obrębie 1 wymiaru.
        if (current_index >= previous_dimensions_size && 
           (down_index / next_dimensions_size == current_index / next_dimensions_size))
            if (!bit_is_true(lab.walls, down_index)) {
                *q_end_ptr = push_back(*q_end_ptr, down_index, false, *q_beginning);
                set_bit(lab.walls, down_index);
            }
        // Zapewnienie pozostania w labiryncie oraz 
        // poruszanie się tylko w obrębie 1 wymiaru.
        if (up_index < lab.tiles && 
           (up_index / next_dimensions_size == current_index / next_dimensions_size))
            if (!bit_is_true(lab.walls, up_index)) {
                *q_end_ptr = push_back(*q_end_ptr, up_index, false, *q_beginning);
                set_bit(lab.walls, up_index);
            }

        previous_dimensions_size *= lab.size[i];
        // Ochrona przed odwołaniem się do niezdefiniowanej wartości.
        if (i < lab.dimension - 1)
            next_dimensions_size *= lab.size[i + 1];
        
    }        
}


/* Funkcja wywołująca breadth first search na podanym labiryncie rozpoczynając
od miejsca startowego i kończąc działanie w przypadku dojścia do miejsca 
końcowego. Wykorzystuje w tym celu kolejkę oraz tablicę bitów, na których
wartość 1 oznacza brak możliwości odwiedzenia danej komórki. Tablica ta jest
na bieżąco aktualizowana o odwiedzone elementy
*/
size_t bfs(labyrinth lab, size_t* starting_point, size_t* finish_point) {
    size_t starting_index = calculate_index(lab, starting_point);
    size_t finish_index = calculate_index(lab, finish_point);

    if (bit_is_true(lab.walls, starting_index))
        err(2);
    if (bit_is_true(lab.walls, finish_index))
        err(3);

    q_node* q_beginning = create_node(starting_index, false);
    q_node* q_end = create_node(0, true);       // Atrapa.
    if (q_beginning == NULL || q_end == NULL) {
        free(q_beginning);
        free(q_end);
        err(0);
    }
    q_beginning->previous = q_end;
    q_end->next = q_beginning;

    bool not_end = !(starting_index == finish_index);
    // Zmienna równa odległości od źródła rozpatrywanych komórek w algorytmie bfs.
    int level = 0;               

    // Wartości dla pojedynczego obiegu pętli odpowiadające 1 komórce labiryntu.
    size_t current_index = starting_index;
    q_node current_q;
    while (not_end) {
        if (q_beginning->decoy) {
            if (q_beginning->next == NULL && q_beginning->previous == NULL) {
                not_end = false;
                printf("NO WAY\n");
                free_queue(q_beginning);
                // Koniec działania programu; nie ma wiecej do wypisania.
                exit(0);                                       
            }

            // Umiesczenie atrapy na końcu kolejki oraz zwiększenie zmiennej level.
            current_q = pop(q_beginning);
            free(q_beginning);
            q_beginning = current_q.previous;
            level++;
            q_end = push_back(q_end, 77, true, q_beginning);
            continue;
        }
        current_q = pop(q_beginning);
        free(q_beginning);
        q_beginning = current_q.previous;
        current_index = current_q.index;
        not_end = current_index != finish_index;

        add_neighbours(lab, current_index, &q_end, &q_beginning);
    }
    free_queue(q_beginning);
    return level;
}
