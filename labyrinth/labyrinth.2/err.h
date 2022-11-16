#ifndef ERR
#define ERR

#include <stdio.h>

/* Procedura wypisująca na standardowe wyjście diagnostyczne podany w 
argumentach numer błędu i kończąca działanie programu z kodem 1.
*/
static inline void err(int errnum) {
    fprintf(stderr, "ERROR %d\n", errnum);
    exit(1);
}
    
#endif /* ERR */