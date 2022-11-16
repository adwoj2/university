#pragma once
/* Współbieżność mojego rozwiązania polega na umożliwianiu różnym procesom czytanie z haszmap 
znajdujących się w węzłach, jednak pozwalanie na zmienianie wartości w haszmapach tylko
pojedynczym procesom pod warunkiem, że żaden inny proces nie pracuje na danym węźle ani 
na jego poddrzewie. W tym celu stosuję zmodyfikowaną implementację rwlocków zaprojektowaną
na laboratoriach z wykorzystaniem mutexów i zmiennych typu cond.
Operacja move dopuszcza tylko i wyłącznie jeden procesu wykonujący move oraz 
zapewnia że żadna inna operacja nie wykonuje się w tym czasie w całym drzewie. Jest
to bardzo słaba współbieżność jednak jedynie taką udało mi się napisać
unikając zakleszczeń i zagłodzeń więc zdecydowałem się ją przesłać.
*/

typedef struct Tree Tree; // Let "Tree" mean the same as "struct Tree".

Tree* tree_new();

void tree_free(Tree*);

char* tree_list(Tree* tree, const char* path);

int tree_create(Tree* tree, const char* path);

int tree_remove(Tree* tree, const char* path);

int tree_move(Tree* tree, const char* source, const char* target);
