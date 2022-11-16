#ifndef BIT
#define BIT

#include <stdint.h>
#include <stdbool.h>

void set_bit(uint8_t* bit_table, int bit_number);
bool bit_is_true(uint8_t* bit_table, int bit_number);

#endif /* BIT */