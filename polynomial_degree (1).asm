global polynomial_degree
extern putchar

;Argumenty do funkcji polynomial_degree:
;rsi - rozmiar tablicy danych liczb
;rdi - wskaźnik na początek tablicy danych
;Funkcja oblicza szukany wielomian zamieniając go na różnice między sąsiadującymi wartościami
;co redukuje stopień wielomianu o 1, aż do uzyskania wielomianu zerowego lub stałego.

polynomial_degree:
        MEM     EQU 95                  ;Stała równa 32 (maksymalny rozmiar początkowej liczby) + 63 (potrzebne do zaokrąglenia w górę).
        Q_SIZE  EQU 64                  ;Rozmiar rejestru.

        PUSH    rbp                     ;Zachowanie rejestru rbp.
        MOV     rbp, rsp                ;Ustawienie wskaznika na początek stosu.
        MOV     r8, -1                  ;Rejestr odpowiedzialny za trzymanie wyniku.
        MOV     ecx, esi                ;Zapamiętanie początkowego rozmiaru tablicy.

;Obliczenie wartości rejestru rax odpowiedzialnego za określenie pamięci alokowanej na stosie

        MOV     rax, rsi            
        ADD     rax, MEM                ;Dodanie pamięci zajmowanej przez początkową liczbę w bitach + 63 (aby dzielenie przez 64 zaokrąglało w górę).
        MOV     r9, Q_SIZE              ;Zapisanie Q_SIZE w rejestrze na potrzeby operacji dzielenia.
        XOR     rdx, rdx                ;Zerowanie rejestru rdx do operacji IDIV.
        IDIV    r9                      ;Zaokrąglane w górę.
        MOV     r10, rax                ;Zapisanie ilości bajtów zajmowanych przez liczbę do licznika.

;Przesłanie liczb z tablicy podanej w argumencie na stos
to_stack:
        MOV     r9, 0                   ;Rejestr odpowiedzialny za wartość, którą ma wypełnić starsze bity w liczbach nie mieszczących się w 64 bitach.
        MOV     rax, r10                ;Pobranie rozmiaru liczby w jednostkach o rozmiarze 64 bitów.
        MOVSXD  rdx, [rdi]              ;Pobranie kolejnej liczby z tablicy.
        TEST    rdx, rdx
        JNS     to_stack.clean          
        MOV     r9, -1                  ;Ustalenie, że starsze bity mają być wypełniane za pomocą 1 dla liczb ujemnych.
    
.clean:                                 ;Ustawienie nowo zaaloowanej pamięci stosu na odpowiednie wartości ustalone w rejestrze r9.
        PUSH    r9                      ;Zaalokowanie nowej pamięci na stosie i uzupełnienie odpowiednią wartością bitu.
        DEC     rax
        JNZ     .clean

        MOV     [rsp], rdx              ;Zapełnienie najmłodszych bitów liczby wartością z tablicy.
        ADD     rdi, 4                  ;Przejście do kolejnego numeru.
        LOOP    to_stack

;Procedura odejmująca kolejne wartości znajdujące się na stosie i nadpisująca je ich różnicami przechodzi do poly_end w przypadku
;wyzerowania wszystkich liczb na stosie (wielomian zerowy), lub pozostaniu tylko jednej liczby (wielomian stały).
iteration:                                             
        MOV     rcx, rsi                ;Ustawienie rcx na ilosc aktualnie rozpatrywanych liczb.
        XOR     r9, r9                  ;Zerowanie bitu r9.

;Sprawdzenie czy wielomian jest wielomianem zerowym
if_zero:                                    
        MOV     r11, r10                ;Ile obrotów na liczbę.
.number:                                ;Sprawdzenie czy pojedyncza liczba jest zerem.
        MOV     rdx, [rsp + 8 * r9]     ;Sprawdzenie czy kolejny fragment 64-bitowy fragment liczby zapisanej w tablicy jest równy.
        TEST    [rsp + 8 * r9], rdx     ;W przypadku wielomianu zerowego wszystkie takie fragmenty byłyby równe 0.
        JNZ     non_zero                ;Więc w przypadku napotkania liczby innej niż 0 można zakonczyć sprawdzanie i zmienić tablicę w tablicę różnic.
        INC     r9                      ;Przejscie do kolejnego 64-bitowego fragmentu.
        DEC     r11                     
        JNZ     if_zero.number
        LOOP    if_zero
        JMP     poly_end
    
;Zamiana n liczb na stosie na n - 1 różnic pomiędzy kolejnymi liczbami wykonywana w przypadku nienapotkania wielomianu zerowego.
non_zero:
        INC     r8         

        CMP     rsi, 1                  ;Sprawdzenie czy w tablicy nie została już tylko jedna liczba co oznacza funkcję stałą
        JZ      poly_end                ;i w tym przypadku opuszczenie procedury.

        DEC     rsi                     ;Zmiana n na liczbę wykonywanych odejmowań (czyli także rozmiar kolejnej tablicy).
        MOV     rcx, rsi                ;Przywrócenie licznika na wartość n po poprzedniej pętli.

        XOR     r9, r9                  ;Wyzerowanie rejestru r9.
        LEA     rdi, [rsp + 8 * r10]    ;Ustawienie w nieużywanym już rejestrze rdi adresu następnej liczby w stosunku do liczby pod adresem w rejestrze rsp.
        
;Pętla wykonująca się ecx (aktualne n) razy odejmując kolejne liczby w tablicy liczb  i nadpisując je otrzymanymi różnicami
loop_reduce:                            
        MOV     r11, r10                ;Ustawienie licznika w rejestrze r11 ponownie na długość liczby liczoną w 64-bitach.
        CLC                             ;Wyzerowanie flagi przeniesienia dla nowej liczby w tablicy.
.number:                                ;Odejmowanie liczby zajmującej więcej niż 64 bity rejestr po rejestrze.
        MOV     rdx, [rdi + 8 * r9]     ;Ustawienie rdx na jedną z liczb na stosie,
        SBB     rdx, [rsp + 8 * r9]     ;a następnie odjęcie jej od poprzedniej w kolejności
        MOV     [rsp + 8 * r9], rdx     ;i zapisanie wyniku w tej wcześniejszej.
        INC     r9                      ;Przejście na kolejny rejestr zapisany na stosie.
        DEC     r11
        JNZ     .number
        LOOP    loop_reduce
        JMP     iteration
poly_end:
        LEAVE                           ;Przywrócenie wskaźnika stosu oraz przywrócenie wartości rejestru rbi.
        MOV rax, r8;                    
        RET                             ;Zwrócenie wyniku.