#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "insert_data.h"

#define HEX_SIZE 4
#define NUMCONST 48
#define BIGLETCONST 55
#define SMALLETCONST 87
#define MAX_UINT_LENGTH 21

/* Procedura dodająca do tablicy liczb daną liczbę i czyszcząca pamięć
aktualnie parsowanej liczby. 
*/
static void add_to_table(char* number, size_t* table, size_t size, int line) {
    char* numend = number + sizeof(number);
    table[size - 1] = strtoull(number, &numend, 10);

    //żaden wymiar nie może być zerem a punkty są indeksowane od 1
    if (table[size - 1] == 0) {
        err(line);
    }
    //liczba poza zakresem
    if (table[size - 1] == __SIZE_MAX__ && errno == ERANGE)
        err(line);
}

/* Procedura pobierająca kolejne znaki ze standardowego wejścia i w przypadku 
napotkania znaku innego niż biały lub znaku końca linii odkłada go na 
z powrotem na standardowe wejście i kończy działanie .
*/
static void go_to_non_white() {
    char c;
    do {
    c = getc(stdin);
    } while (isspace(c) && c != '\n');
    ungetc(c, stdin);
}

/* Funkcja zwracająca tablicę liczb wczytanych kolejno z jednej linii wejścia
zgodnie ze specyfikacją zadania. Dodatkowo ustawia zmienną na którą wskazuje k
na liczbę wczytanych liczb.
*/
size_t* read_size_or_coordinates(size_t* k, int line) {
    if (check_endline()) {
        err(line);
    }

    char single;
    char* number = (char*)safe_malloc(sizeof(char) * MAX_UINT_LENGTH);
    size_t table_size = 0;
    size_t* dest = NULL;
    int number_size = 0;
    go_to_non_white();

    do {
        single = getc(stdin);
        if (isspace(single) || single == EOF) {
            if (number_size != 0) {
                number[number_size] = '\0';    
                table_size++;
                dest = (size_t*)safe_realloc(dest, sizeof(size_t) * table_size);

                add_to_table(number, dest, table_size, line);
                go_to_non_white();
                number_size = 0;
            }
        } else if (single < '0' || single > '9') {
            err(line);
        } else {
            number[number_size] = single;
            number_size++;
        }
    } while (single != '\n' && single != EOF);
    //odłożenie końca pliku aby mógł zostać zdiagnozowany w następnej linii
    if (single == EOF)
        ungetc(EOF, stdin);          

    *k = table_size;
    go_to_non_white();
    return dest;
}

/* Procedura wyliczająca i przypisująca do odpowiedniej komórki pamięci 
struktury labirynth ilość komórek pamięci w labiryncie */

void calculate_tiles (labyrinth* lab) {
    lab->tiles = 1;
    for (size_t i = 0; i < lab->dimension; i++)
        lab->tiles *= lab->size[i];
}

/* Procedura ustawiająca bity w tablicy bitów opisującej ściany labiryntu.
*/
static void calculate_walls(labyrinth* lab, size_t* w, size_t w_size) {
    for (size_t i = 1; i <= w_size; i++) 
        set_bit(lab->walls, w[i] % lab->tiles);
}

/* Funkcja czytająca ze standardowego wejścia jedną z 5 liczb, które opisują
ściany labiryntu w formacie z resztami. 
*/ 
void scan_R_value(size_t* x) {
    go_to_non_white();
    if(check_endline())
        err(4);
    if (scanf("%ld", x) == EOF)
        err(4);
    if (*x > UINT32_MAX)
        err(4);
}

/* Procedura wczytująca i przetwarzająca linię opisującą ściany labiryntu w 
przypadku formatu liczby z resztami.
*/
static void parse_R_form(labyrinth* lab) {
    size_t a, b, m, r, s; 

    scan_R_value(&a);
    scan_R_value(&b);
    scan_R_value(&m);
    scan_R_value(&r);
    scan_R_value(&s);
    
    size_t* sw = (size_t*)safe_malloc(sizeof(size_t) * (r + 1));
    sw[0] = s;

    for (size_t i = 1; i <= r; i++)
        sw[i] = ((a * sw[i - 1]) % m + b) % m;
    calculate_walls(lab, sw, r);
}

/* Funkcja konwertująca jeden znak na liczbę zakładając, że owy znak był
cyfrą w rozumieniu systemu szesnastkowego. Gdy takiej konwersji nie można
przeprowadzić zwracany jest odpowiedni błąd.
*/
static int from_hex_to_int (char c) {
    if (isxdigit (c) == 0)
        return -1;
    if (c <= '9' && c >= '0')
        c -= NUMCONST;
    else if (c >= 'a' && c <= 'f')
        c-= SMALLETCONST;
    else if (c >= 'A' && c <= 'F')
        c -= BIGLETCONST;
    else c = -1;
    return c;
}

/* Procedura wczytująca i przetwarzająca linię opisującą ściany labiryntu w 
przypadku formatu liczby szesnastkowej.
*/
static void parse_hexadecimal(labyrinth* lab) {
    char check = getc(stdin);
    if (isspace(check) || check == EOF) {
        err(4);
        return;
    }
    ungetc (check, stdin);
    
    int* number = NULL;
    int index = 0;
    char single = getc(stdin);
    int number_from_hex_digit = 0;
    bool read_zeros = false;
    
    while (!isspace(single) && single != EOF) {
        number_from_hex_digit = from_hex_to_int(single);  
        if ((number_from_hex_digit >= 0 && number_from_hex_digit <= 15)) {
            if (number_from_hex_digit != 0 || read_zeros) {
                number = (int*)safe_realloc(number, sizeof(int) * (index + 1));
                number[index] = number_from_hex_digit;
                index++;
                read_zeros = true;
            }
        } else {
            err(4);
        }
        single = getc(stdin);
    } 
    
    // Odłożenie pobranego białego znaku i dojście do końca linii 
    // na wypadek gdyby pobrano właśnie znak końca linii.
    ungetc(single, stdin);
    go_to_non_white();
    size_t size = index;
    int digit;
    
    for (size_t i = 0; i < size; i++) {
        digit = number[size - i - 1];
        for (size_t j = 0; j < HEX_SIZE; j++) {              
            if (digit % 2 == 1) {
                if (lab->tiles <= HEX_SIZE * i + j)           //Jeżeli ściana miałaby się znaleźć poza labityntem - błąd
                    err(4); 
                set_bit(lab->walls, HEX_SIZE * i + j);
            }
            digit /= 2;
        }
    }
}

/* Procedura przetwarzająca 4-tą linię wejścia opisującą ściany labiryntu w 
podanych w treści formatach.
*/
void read_walls(labyrinth* lab) {
    // Alokacja tablicy bitów
    size_t how_many_blocks = (lab->tiles + 7) / 8;
    lab->walls = (uint8_t*)safe_malloc(sizeof(uint8_t) * how_many_blocks);
    for (size_t i = 0; i < how_many_blocks; i++)
        lab->walls[i] = 0;

    // Sprawdzenie formatu podanej liczby
    char first = getc(stdin);
    if (first == 'R') {
        go_to_non_white();
        parse_R_form(lab);
    } else if (first == '0') {
        char second = getc(stdin);
        if (second == 'x')
            parse_hexadecimal(lab);
        else
            err(4);
    } else {
        err (4);
    }

    go_to_non_white();
    if (!check_endline()) 
        err(4);
}
