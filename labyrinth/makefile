all: labyrinth

bit.o: bit.c bit.h
	gcc -c -Wall -Wextra -std=c11 -O2 bit.c

insert_data.o: insert_data.c insert_data.h err.h labyrinth_structure.h testers.h bit.h
	gcc -c -Wall -Wextra -std=c11 -O2 insert_data.c

bfs.o: bfs.c bfs.h labyrinth_structure.h queue.h bit.h err.h testers.h
	gcc -c -Wall -Wextra -std=c11 -O2 bfs.c
	
testers.o: testers.c testers.h err.h
	gcc -c -Wall -Wextra -std=c11 -O2 testers.c

queue.o: queue.c queue.h
	gcc -c -Wall -Wextra -std=c11 -O2 queue.c

main.o: main.c insert_data.h err.h labyrinth_structure.h bfs.h queue.h testers.h bit.h
	gcc -c -Wall -Wextra -std=c11 -O2 main.c

labyrinth: main.o insert_data.o bfs.o testers.o queue.o bit.o
	gcc -o labyrinth main.o insert_data.o bfs.o testers.o queue.o bit.o

clean:
	rm -f *.o labyrinth

