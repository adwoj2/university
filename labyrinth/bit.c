#include "bit.h"

#define BYTES_IN_BLOCK 8

/* Procedura ustawiająca wskazany bit w tablicy bitów na 1.
*/  
void set_bit(uint8_t* bit_table, int bit_number) {
    int block_number = bit_number / BYTES_IN_BLOCK;
    uint8_t position = bit_number % BYTES_IN_BLOCK;

    bit_table[block_number] |= 1 << position;
}

/* Funkcja sprawdzająca czy na podanym bicie jest ustawiona wartość 1
i zwracająca prawdę w tym przypadku.
*/  
bool bit_is_true(uint8_t* bit_table, int bit_number) {
    int block_number = bit_number / BYTES_IN_BLOCK;
    uint8_t position = bit_number % BYTES_IN_BLOCK;     // pozycja w bloku

    uint8_t flag = 1;
    flag = flag << position;
    return bit_table[block_number] & flag;
}