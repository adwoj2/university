#define _DEFAULT_SOURCE
#include <endian.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
#include <getopt.h>

#define MAX_DATA_SIZE 65507
#define MIN_RAND_IN_COOKIE 33
#define COOKIE_RANGE 94
#define ASCII_CONVERT_NUMBERS 48
#define ASCII_CONVERT_BIG_LETTERS 55
#define TICKET_VIABLE_CHARACTERS 36
#define MAX_DESCRIPTION_LENGTH 80
#define TICKET_ID_MAX_VALUE 78364164096 //36^7
#define TICKET_ID_JUMP_VALUE 6103515625 //5^14 liczba względnie pierwsza do 36^7 = 6^14
#define FIRST_RESERVATION_ID 1000000
#define MAX_EVENT_LENGTH 150 //szacowane bez dokładnych obliczeń

#define MAX_TICKETS_PER_RESERVATION 9357 /* 9357 = 65500 / 7
i jest to ilość bajtów jakie zmieszczą się w jednym datagramie ponieważ 
7 bajtów jest potrzebne na inne informacje i po 7 bajtów na bilet
*/

#define GET_EVENT 1
#define EVENTS 2
#define GET_RESERVATION 3
#define RESERVATION 4
#define GET_TICKETS 5
#define TICKETS 6
#define BAD_REQUEST 255

#define MESSAGE_ID_SIZE 1
#define DESCRIPTION_LENGTH_SIZE 1
#define TICKET_COUNT_SIZE 2
#define EVENT_ID_SIZE 4
#define RESERVATION_ID_SIZE 4
#define COOKIE_SIZE 48
#define EXPIRATION_TIME_SIZE 8
#define TICKET_SIZE 7

// Makra pochodzące z udostępnionych na laboratoriach plików upraszczające kod.
#define CHECK(x)                                                          \
    do {                                                                  \
        int err = (x);                                                    \
        if (err != 0) {                                                   \
            fprintf(stderr, "Error: %s returned %d in %s at %s:%d\n%s\n", \
                #x, err, __func__, __FILE__, __LINE__, strerror(err));    \
            exit(EXIT_FAILURE);                                           \
        }                                                                 \
    } while (0)

#define ENSURE(x)                                                         \
    do {                                                                  \
        bool result = (x);                                                \
        if (!result) {                                                    \
            fprintf(stderr, "Error: %s was false in %s at %s:%d\n",       \
                #x, __func__, __FILE__, __LINE__);                        \
            exit(EXIT_FAILURE);                                           \
        }                                                                 \
    } while (0)

#define PRINT_ERRNO()                                                     \
    do {                                                                  \
        if (errno != 0) {                                                 \
            fprintf(stderr, "Error: errno %d in %s at %s:%d\n%s\n",       \
              errno, __func__, __FILE__, __LINE__, strerror(errno));      \
            exit(EXIT_FAILURE);                                           \
        }                                                                 \
    } while (0)


#define CHECK_ERRNO(x)                                                    \
    do {                                                                  \
        errno = 0;                                                        \
        (void) (x);                                                       \
        PRINT_ERRNO();                                                    \
    } while (0)


char shared_buffer[MAX_DATA_SIZE];
long long next_ticket_id = 0;
uint32_t next_reservation = 0, max_reservations_size = 1;

typedef struct reservation {
    uint32_t reservation_id;
    uint8_t cookie[COOKIE_SIZE];
    uint16_t ticket_count;
    uint32_t event_id;
    char* tickets;
    bool claimed;
    uint64_t expiration_time;
} reservation;

typedef struct event {
    uint32_t event_id;
    uint8_t description_length;           
    char* event_description;
    uint16_t tickets_available;
} event;

typedef struct event_list {
    event* events;
    uint32_t how_many_events;
} event_list;

// Funkcje pochodzące z udostępnionych na laboratoriach plików upraszczające kod.
inline static void fatal(const char *fmt, ...) {
    va_list fmt_args;

    fprintf(stderr, "Error: ");
    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end(fmt_args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

uint16_t read_port(char *string) {
    errno = 0;
    unsigned long port = strtoul(string, NULL, 10);
    PRINT_ERRNO();
    if (port > UINT16_MAX) {
        fatal("%ul to nieprawidłowy numer portu", port);
    }

    return (uint16_t) port;
}

int bind_socket(uint16_t port) {
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0); 
    ENSURE(socket_fd > 0);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; 
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_address.sin_port = htons(port);

    CHECK_ERRNO(bind(socket_fd, (struct sockaddr *) &server_address,
                        (socklen_t) sizeof(server_address)));

    return socket_fd;
}

size_t read_message(int socket_fd, struct sockaddr_in *client_address, 
                    char *buffer, size_t max_length) {
    socklen_t address_length = (socklen_t) sizeof(*client_address);
    int flags = 0; 
    errno = 0;
    ssize_t len = recvfrom(socket_fd, buffer, max_length, flags,
                           (struct sockaddr *) client_address, &address_length);
    if (len < 0) {
        PRINT_ERRNO();
    }
    return (size_t) len;
}

void send_message(int socket_fd, const struct sockaddr_in *client_address,
                  const char *message, size_t length) {
    socklen_t address_length = (socklen_t) sizeof(*client_address);
    int flags = 0;
    ssize_t sent_length = sendto(socket_fd, message, length, flags,
                                 (struct sockaddr *) client_address, address_length);
    ENSURE(sent_length == (ssize_t) length);
}

// Funkcja zwracająca strunkturę zawierającą wszystkie wydarzenia zawarte w pliku.
event_list load_events(FILE* f) {
    event_list all_events;

    int it = 0;
    int max_it = 1;
    all_events.events = malloc (sizeof(event) * max_it);
    char bucket_for_endl;

    while (true) {
        event loaded_event;
        loaded_event.event_id = it;
        char* event_description = malloc (sizeof(char) * MAX_DESCRIPTION_LENGTH);
        size_t description_max_length = MAX_DESCRIPTION_LENGTH + 1;

        ssize_t description_length = getline(&event_description, &description_max_length, f);

        if (description_length == -1)
            break;
        
        loaded_event.description_length = description_length - 1;

        // Znak końca linii musi zostać wczytany ręcznie aby nie został 
        // przechwycony jako nowe wydarzenie.
        fscanf(f, "%hu%c", &(loaded_event.tickets_available), &bucket_for_endl);

        loaded_event.event_description = event_description;
        all_events.events[it] = loaded_event;

        it++;
        if (it >= max_it) {
            max_it *= 2;
            all_events.events = realloc(all_events.events, sizeof(event) * max_it);
        }
    }
    all_events.events = realloc(all_events.events, sizeof(event) * (it + 1));
    all_events.how_many_events = it;

    return all_events;
}

// Funkcja zamieniająca pojedyncze wydarzenie na strumień bajtów.
int event_to_bytes_stream(event ev, char* byte_stream) {
    char* byte_stream_pointer = byte_stream;

    *(uint32_t*) byte_stream_pointer = htonl(ev.event_id);
    byte_stream_pointer += EVENT_ID_SIZE;

    *(uint16_t*) byte_stream_pointer = htons(ev.tickets_available);
    byte_stream_pointer += TICKET_COUNT_SIZE;

    *(uint8_t*) byte_stream_pointer = ev.description_length;
    byte_stream_pointer += DESCRIPTION_LENGTH_SIZE;

    char* description_pointer = ev.event_description;
    for(int i = 0; i < ev.description_length; i++) { 
        *byte_stream_pointer = *description_pointer;
        byte_stream_pointer++;
        description_pointer++;
    }

    return byte_stream_pointer - byte_stream;
}

// Procedura wysyłająca wiadomość o message_id = 255.
void send_bad_request(uint32_t value, int socket_fd, struct sockaddr_in* client_address_ptr) {
    *shared_buffer = (char) BAD_REQUEST;
    *(uint32_t*)(shared_buffer + MESSAGE_ID_SIZE) = htonl(value);

    send_message(socket_fd, client_address_ptr, shared_buffer, (MESSAGE_ID_SIZE + EVENT_ID_SIZE));
}

// Funkcja sprawdzająca czy podane ciastka są sobie równe.
bool check_cookie(char* res_cookie, char* cookie) {
    for (int i = 0; i < COOKIE_SIZE; i++) {
        if (cookie[i] != res_cookie[i]) 
            return false;           
        
    }
    return true;
}

// Funkcja zwracająca losowo wygenerowane ciastko.
char* generate_cookie() {
    char* cookie = malloc (sizeof(char) * COOKIE_SIZE);
    char c;
    for (int i = 0; i < COOKIE_SIZE; i++) {
        c = (char) (rand() % COOKIE_RANGE) + MIN_RAND_IN_COOKIE;
        cookie[i] = (char)c;
    }
    return cookie;
}

// Funkcja zwracająca kolejny wygenerowany bilet.
char* generate_ticket() { 
    char* ticket = malloc (sizeof(char) * TICKET_SIZE);
    long long current_ticket = next_ticket_id;
    next_ticket_id = (next_ticket_id + TICKET_ID_JUMP_VALUE) % TICKET_ID_MAX_VALUE;
    int sign;
    for (int i = 0; i < TICKET_SIZE; i++) {
        sign = current_ticket % TICKET_VIABLE_CHARACTERS;
        current_ticket /= TICKET_VIABLE_CHARACTERS;
        if (sign < 10) 
            ticket[i] = (char) sign + ASCII_CONVERT_NUMBERS;
        else
            ticket[i] = (char) sign + ASCII_CONVERT_BIG_LETTERS;
    }

    return ticket;
}

// Funkcja zwracająca do puli wszystkie nieodebrane na czas bilety.
void gather_unused(event* events, reservation* reservations) {
    for (uint32_t i = 0; i < next_reservation; i++)
        if (!reservations[i].claimed && (uint64_t)time(NULL) > reservations[i].expiration_time) {
            reservations[i].claimed = true;                  //bilety zostały zwrócone
            events[reservations[i].event_id].tickets_available += reservations[i].ticket_count;
        }
}

// Funkcja wysyłająca do kliena informacje o wydarzeniach.
void send_event_info(reservation* reservations, event_list all_events, 
                     int socket_fd, struct sockaddr_in* client_address_ptr) {
    char* message_ptr = shared_buffer;

    *(uint8_t*)message_ptr = (uint8_t)EVENTS;
    message_ptr += MESSAGE_ID_SIZE;
    
    char* next_message = malloc(sizeof(char) * MAX_EVENT_LENGTH);
    int next_message_length;

    gather_unused(all_events.events, reservations);            //uzupełnienie dostępnej puli biletów 

    for (uint32_t i = 0; i < all_events.how_many_events; i++) {

        next_message_length = event_to_bytes_stream(all_events.events[i], next_message);

        if (next_message_length + message_ptr - shared_buffer > MAX_DATA_SIZE)
            break;
        for (int i = 0; i < next_message_length; i++) {
            *message_ptr = *(next_message + i) ;
            message_ptr++;
        }
    }
    free(next_message);

    send_message(socket_fd, client_address_ptr, shared_buffer, message_ptr - shared_buffer);
}

// Funkcja zwracająca wskaźnik na nowo zarezerwowany bilet. W przypadku błędnych danych zwraca NULL.
reservation* reserve_ticket(reservation* reservations, event* events, uint32_t event_id, 
                            uint16_t ticket_count, uint64_t expiration_time, uint32_t reservation_id,
                            int socket_fd, struct sockaddr_in* client_address_ptr) {
    event* reserved = events + event_id;

    if (ticket_count == 0) {
        send_bad_request(event_id, socket_fd, client_address_ptr);
        return NULL;
    }
    if (reserved->tickets_available < ticket_count) {
        gather_unused(events, reservations);    
        //Sprawdzenie czy potrzebne bilety nie powróciły do puli.
        if (reserved->tickets_available < ticket_count) { 
            send_bad_request(event_id, socket_fd, client_address_ptr);
            return NULL;
        }
    }

    reservation* res = malloc (sizeof(reservation)); 

    res->reservation_id = reservation_id;
    res->event_id = event_id;
    res->ticket_count = ticket_count;
    res->expiration_time = expiration_time;
    res->claimed= false;
    res->tickets = malloc (sizeof(char*) * res->ticket_count * TICKET_SIZE);

    reserved->tickets_available -= ticket_count;

    char* message_ptr = shared_buffer;
    *(uint8_t*) message_ptr = RESERVATION;
    message_ptr += MESSAGE_ID_SIZE;    

    *(uint32_t*) message_ptr = htonl(reservation_id);
    message_ptr += RESERVATION_ID_SIZE;

    *(uint32_t*) message_ptr = htonl(event_id);
    message_ptr += EVENT_ID_SIZE;

    *(uint16_t*) message_ptr = htons(ticket_count);
    message_ptr += TICKET_COUNT_SIZE;

    char* cookie = generate_cookie();
    char* cookie_curr_pointer = cookie;
    for (int i = 0; i < COOKIE_SIZE; i++) {
        *message_ptr = *cookie_curr_pointer;
        res->cookie[i] = *message_ptr;
        message_ptr++;
        cookie_curr_pointer++;
    }
    free(cookie);

    *(uint64_t*) message_ptr = htobe64(expiration_time);
    message_ptr += EXPIRATION_TIME_SIZE;

    // Generowanie biletów.
    for (int i = 0; i < res->ticket_count; i++) {
        char* new_ticket = generate_ticket();
        for (int j = 0; j < TICKET_SIZE; j++) {
            *(res->tickets + (i * TICKET_SIZE) + j) = *(new_ticket + j);
        }
    }
    send_message(socket_fd, client_address_ptr, shared_buffer, message_ptr - shared_buffer);

    return res;
}

void get_tickets(reservation* res, int socket_fd, struct sockaddr_in* client_address_ptr) {
    if (!res->claimed && (uint64_t)time(NULL) > res->expiration_time) 
        send_bad_request(res->reservation_id, socket_fd, client_address_ptr);
    res->claimed = true;

    char* message_ptr = shared_buffer;

    *(uint8_t*) message_ptr = TICKETS;
    message_ptr += MESSAGE_ID_SIZE;    

    *(uint32_t*) message_ptr = htonl(res->reservation_id);
    message_ptr += RESERVATION_ID_SIZE;
    
    *(uint16_t*) message_ptr = htons(res->ticket_count);
    message_ptr += TICKET_COUNT_SIZE;
    
    for (int i = 0; i < res->ticket_count; i++) {
        for (int j = 0; j < TICKET_SIZE; j++) {
            *message_ptr = *(res->tickets + i * TICKET_SIZE + j);
            message_ptr++;
        }
    }

    send_message(socket_fd, client_address_ptr, shared_buffer, message_ptr - shared_buffer);
}

int main(int argc, char *argv[]) {
    // Potrzebne do losowania ciastek.
    srand(time(NULL));
    char *events_filename = NULL, *timeoutstr = NULL;
    int c;
    int timeout = 5;
    char* portstr = "2865";
    opterr = 0;

    while ((c = getopt (argc, argv, "f:p:t:")) != -1)
        switch (c)
        {
            case 'f':
                events_filename = optarg;
                break;
            case 'p':
                portstr = optarg;
                break;
            case 't':
                timeoutstr = optarg;
                break;
            case '?':
                if (optopt == 'f' || optopt == 'p' || optopt == 't')
                    fprintf (stderr, "Opcja -%c wymaga argumentu.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Nieznana opcja `-%c'.\n", optopt);
                else
                    fprintf (stderr, "Nieznany znak opcji `\\x%x'.\n", optopt);
                return 1;
            default:
            abort();
        }

    for (int i = optind; i < argc; i++) {
        fprintf (stderr, "Nieznany argument %s\n", argv[i]);
        return 1;
    }
    if (events_filename == NULL) {
        fprintf (stderr, "Brak nazwy pliku\n");
        return 1;
    }
    // W przeciwnym przypadku timeout jest domyślny.
    if (timeoutstr != NULL)
        timeout = atoi(timeoutstr);
    if (timeout < 1 || timeout > 86400) {
        fprintf (stderr, "Błędna wartość timeout, musi być między 1 a 86400\n");
        return 1;
    }

    FILE *f = fopen(events_filename, "r");
    if (f == NULL) {
        fprintf (stderr, "Brak pliku o podanej nazwie\n");
        return 1;
    }

    event_list all_events = load_events(f);;
    uint16_t port = read_port(portstr);
    if (port == 0) {
        fprintf (stderr, "Nieprawidłowy port\n");
        return 1;
    }

    memset(shared_buffer, 0, MAX_DATA_SIZE);
    int socket_fd = bind_socket(port);

    struct sockaddr_in client_address;
    size_t read_length;
    uint64_t recieve_time, expiration_time;

    reservation* reservations = malloc (sizeof(reservation) * max_reservations_size);

    do {                    
        read_length = read_message(socket_fd, &client_address, shared_buffer, sizeof(shared_buffer));
        recieve_time = time(NULL);
        expiration_time = recieve_time + timeout;

        //wskaźnik na pierwsze 8 bitów bufora w których powinno być id wiadomości
        char* message_id_ptr = shared_buffer; 

        switch (*message_id_ptr) {
            case GET_EVENT: {
                send_event_info(reservations, all_events, socket_fd, &client_address);
                break;
            }
            case GET_RESERVATION: {
                uint32_t event_id = ntohl(*(uint32_t*) (shared_buffer + MESSAGE_ID_SIZE));  
                uint16_t ticket_count = ntohs(*(uint16_t*) (shared_buffer + MESSAGE_ID_SIZE + EVENT_ID_SIZE));
                if (ticket_count > MAX_TICKETS_PER_RESERVATION) {
                    send_bad_request(event_id, socket_fd, &client_address);
                    break;
                }

                reservation* newres = reserve_ticket(reservations, all_events.events, event_id, 
                                                     ticket_count, expiration_time, 
                                                     next_reservation + FIRST_RESERVATION_ID, 
                                                     socket_fd, &client_address);
                if (newres == NULL)
                    break;
                else 
                    reservations[next_reservation] = *newres;
                
                next_reservation++;
                if (next_reservation >= max_reservations_size) {
                    max_reservations_size *= 2;
                    reservations = realloc (reservations, sizeof(reservation) * max_reservations_size);
                }
                break;
            }
            case GET_TICKETS: {
                uint32_t reservation_id = ntohl(*(uint32_t*) (shared_buffer + MESSAGE_ID_SIZE));  
        
                if (reservation_id < FIRST_RESERVATION_ID ||
                    reservation_id >= next_reservation + FIRST_RESERVATION_ID) {
                    send_bad_request(reservation_id, socket_fd, &client_address);
                    break;
                }

                if (check_cookie((char*)(reservations[reservation_id - FIRST_RESERVATION_ID]).cookie, 
                                shared_buffer + MESSAGE_ID_SIZE + RESERVATION_ID_SIZE))
                    get_tickets(reservations + reservation_id - FIRST_RESERVATION_ID,
                                socket_fd, &client_address);
                else {
                    send_bad_request(reservation_id, socket_fd, &client_address);
                    break;
                }
                break;
            }
        }
    } while (read_length > 0);
    CHECK_ERRNO(close(socket_fd));

    return 0;
}
