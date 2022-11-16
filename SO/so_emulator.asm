global so_emul
section .data
;Stałe potrzebne do rotacji
BYTE_LENGTH EQU 8
IDENTIFIER EQU 0x7
NEXT_IDENDTIFIER EQU 0x3

;Stała do wyodrębniania ostatniego bajtu.
LAST_BYTE EQU 0xFF

;Specjalne stałe dla funkcji XCHG
XCHG_CODE EQU 0x1
XCHG_IDENTIFIER EQU 0x8

;Stałe potrzebne do zidentyfikowania klasy instrukcji na podstawie pierwszych 2 bitów.
CLASS_IDENTIFIER EQU 0xC000
ND_CLASS_LIMIT EQU 0x4000
RD_CLASS_LIMIT EQU 0x8000

;Stałe do identyfikacji kodu argumentu
IF_ADDR EQU 0x4
IF_WITH_D EQU 0x2
IF_Y EQU 0x1

;Stałe do odwoływania się do rejestrów emulowanego procesora.
D EQU 0x1
X EQU 0x2
Y EQU 0x3
PC EQU 0x4
C EQU 0x6
Z EQU 0x7

;Deklaracja pamięci z adresami instrukcji emulowanego procesora zgodnie z kodami w treści zadania.
first_instructions: dq execute_mov_and_movi, execute_xchg, execute_or, 0, execute_add_and_addi, execute_sub, execute_adc, execute_sbb
second_instructions: dq execute_mov_and_movi, 0, 0, execute_xori, execute_add_and_addi, execute_cmpi, execute_rcr
third_instructions: dq execute_clc, execute_stc
fourth_instructions: dq execute_jmp, 0, execute_jnc, execute_jc, execute_jnz, execute_jz, 0, execute_brk

section .bss
state: resq CORES
spin_lock resb 1
section .text 
;RDI - pamięć kodu
;RSI - dane
;RDX - wartość steps
so_emul:
        PUSH    RBP
        PUSH    RBX
        PUSH    R12
        PUSH    R13
        PUSH    R14
        PUSH    R15

        LEA     R15, [REL state] 
        ;Ustawienie RCX na 0 w przypadku jednowątkowego procesora
        MOV     R8, 0x1
        SUB     R8, CORES
        CMOVZ   RCX, R8
        ;Ustawienie w rejestrze R15 adresu na pamięć własną danego rdzenia
        LEA     R15, [R15 + RCX * 8]
        MOV     CL, [R15 + PC]
        ;W przypadku 0 kroków do wykonania natychmiastowe przejście do końca
        TEST RDX, RDX
        JZ finish_code

execute_code:
        ;Załadowanie kodu rozkazu do wykonania.
        MOV     AX, [RDI + RCX * 2]
        ;Ustawienie nieistotnego numeru bitu na rejestrze R10
        MOV     R10D, 1;

;Identyfikacja grupy rozkazu na podstawie 2 pierwszych bitów
.identify_class:
        MOV     BX, AX
        AND     BX, CLASS_IDENTIFIER
        JZ      first_class
        CMP     BX, ND_CLASS_LIMIT
        JZ      second_class
        CMP     BX, RD_CLASS_LIMIT
        JZ      third_class
        JMP     fourth_class

.execute_end:
        ;Wyjście z sekcji krytycznej. W przypadku wejścia do niej wyzeruje sie ustawiony 
        ;na 1 na indeksie 0 bit. W przypadku braku wejscia wyzerowany zostanie bit już równy zeru.
        TEST    R10D, R10D
        JNZ      .after_btr
        BTR     dword [rel spin_lock], 0
.after_btr:
        ;Aktualizacja licznika rozkazów i wartości steps
        INC     CL
        DEC     RDX
        JNZ     execute_code

finish_code:
        ;Aktualizacja licznika rozkazów oraz załadowanie struktury do wartości zwracanej
        MOV     [R15 + PC], CL
        MOV     RAX, [R15]
        ;Przywrócenie rejestrów
        POP     R15
        POP     R14
        POP     R13
        POP     R12
        POP     RBX
        POP     RBP
        RET

;Procedura umieszczająca w rejestrze R14 adres argumentu instrukcji emulatora
get_register:
        ;Pobranie kodu argumentu
        MOV     R11, IDENTIFIER
        AND     R11B, AL
;Bezpośrednia wartość rejestrów.
        MOV     R8B, IF_ADDR
        AND     R8B, R11B
        
        JNZ     .adress
        LEA     R14, [R15 + R11]
        JMP     RBP
;Adresy pod rejestrami.
.adress: 
        ;Jeśli program już jest w sekcji krytycznej to nie zakleszcza się.
        TEST    R10D, R10D
        JZ      .after_spinlock
        ;Spinlock zmieniający bit na indeksie 0 w zmiennej spinlock
        MOV     R10D, 0
.spin_lock:
        LOCK    \
        BTS     dword [rel spin_lock], 0
        JC      .spin_lock
.after_spinlock:
        AND     RCX, LAST_BYTE
        ;Sprawdzenie czy adres dotyczy wartości pod X czy pod Y.
        MOV     R8B, IF_Y
        AND     R8B, R11B
        MOV     R14W, [R15 + X]       ;Starsze bity będą ignorowane.
        JZ      .if_add_dval
        MOV     R14W, [R15 + Y]       ;Starsze bity będą ignorowane.
;Sprawdzenie czy należy dodać wartość pod rejestrem D do adresu.
.if_add_dval:
        MOV     R8B, IF_WITH_D
        AND     R8B, R11B
        JZ      .addrdone
        ADD     R14B, [R15 + D]
.addrdone:
        ;Wyzerowanie starszych bitów.
        AND     R14, LAST_BYTE
        ADD     R14, RSI
.end:
        JMP     RBP

first_class:
        ;Ustawienie kolejnych argumentów rotując słowo z instrukcją.
        ;Sprawdzenie kodu instrukcji.
        MOV     R8, XCHG_IDENTIFIER
        ;Osobny test na to czy instrukcją jest xchg ze względu na jej specyficzny kod.
        AND     R8B, AL
        JZ      .not_xchg
        MOV     R9, XCHG_CODE      
        JMP     .R9set
.not_xchg:
        MOV     R9, IDENTIFIER
        AND     R9B, AL
.R9set:
        ;Pobranie 1 argumentu.
        ROR     AX, BYTE_LENGTH
        LEA     RBP, [rel first_class.done1]
        JMP     get_register
.done1:
        MOV     R12, R14
        ;Pobranie 2 argumentu.
        ROR     AX, NEXT_IDENDTIFIER
        LEA     RBP, [rel first_class.done2]
        JMP     get_register
.done2:             
        MOV     R13, [R14]
        LEA     RBX, [REL first_instructions]
        JMP     [RBX + R9 * 8]

second_class:
        MOV     R13B, AL
        ROR     AX, BYTE_LENGTH

        LEA     RBP, [rel second_class.done]
        JMP     get_register
.done:  
        MOV     R12, R14

        ROR     AX, NEXT_IDENDTIFIER
        MOV     R9, IDENTIFIER
        AND     R9B, AL
        LEA     RBX, [REL second_instructions]
        JMP     [RBX + R9 * 8]

third_class:
        ROR     AX, BYTE_LENGTH
        MOV     R9, IDENTIFIER
        AND     R9B, AL
        LEA     RBX, [REL third_instructions]
        JMP     [RBX + R9 * 8]

fourth_class:
        MOV     R13B, AL
        ROR     AX, BYTE_LENGTH
        MOV     R9, IDENTIFIER
        AND     R9B, AL
        LEA     RBX, [REL fourth_instructions]
        JMP     [RBX + R9 * 8]

;Fukcje pierwszej klasy
;R12 - adres arg1
;R13B - wartość arg2
;R14 - adres arg2
execute_mov_and_movi:
        MOV     [R12], R13B
        JMP     execute_code.execute_end

execute_or:
        OR      [R12], R13B
        JMP     set_zflag

execute_add_and_addi:
        ADD     [R12], R13B
        JMP     set_zflag

execute_sub:
        SUB     [R12], R13B
        JMP     set_zflag

execute_adc:
        MOV     R8B, -1
        ADD     R8B, [R15 + C]
        ADC     [R12], R13B 
        JMP     set_bothflags

execute_sbb:
        MOV     R8B, -1
        ADD     R8B, [R15 + C]
        SBB     [R12], R13B 
        JMP     set_bothflags

execute_xchg:
        MOV     R8B, [R12]
        MOV     [R12], R13B
        MOV     [R14], R8B
        JMP     execute_code.execute_end

;Funkcje drugiej klasy.
;R12 - adres arg1
;R13B - imm8
;execute_mov_and_movi oraz execute_add_and_addi są zadeklarowane wcześniej
execute_xori:
        XOR     [R12], R13B
        JMP     set_zflag

execute_cmpi:
        CMP     [R12], R13B 
        JMP     set_bothflags

execute_rcr:
        MOV     R14B, [R15 + C]
        ROR     R14B, 1
        MOV     R8B, [R12]

        RCR     R8B, 1
        MOV     byte [R15 + C], 0
        JNC     .skip_carry
        MOV     byte [R15 + C], 1
.skip_carry:
        OR      R8B, R14B
        MOV     [R12], R8B
        JMP     execute_code.execute_end


;Funkcje trzeciej klasy.
execute_clc:
        MOV     byte [R15 + C], 0
        JMP     execute_code.execute_end
execute_stc:
        MOV     byte [R15 + C], 1
        JMP     execute_code.execute_end

;Funkcje czwartej klasy.
;r13 - imm8
execute_jmp:
        ADD     CL, R13B
        JMP     execute_code.execute_end

execute_jnc:
        MOV     R14B, [R15 + C]
        TEST    R14B, R14B
        JNZ     execute_code.execute_end   
        ADD     CL, R13B
        JMP     execute_code.execute_end

execute_jc:
        MOV     R14B, [R15 + C]
        TEST    R14B, R14B
        JZ      execute_code.execute_end   
        ADD     CL, R13B
        JMP     execute_code.execute_end

execute_jnz:
        MOV     R14B, [R15 + Z]
        TEST    R14B, R14B
        JNZ     execute_code.execute_end   
        ADD     CL, R13B
        JMP     execute_code.execute_end

execute_jz:
        MOV     R14B, [R15 + Z]
        TEST    R14B, R14B
        JZ      execute_code.execute_end   
        ADD     CL, R13B
        JMP     execute_code.execute_end

execute_brk:
        MOV     RDX, 1
        JMP     execute_code.execute_end

;Procedury zmieniające flagi
set_zflag:
        MOV     byte [R15 + Z], 0
        JNZ     execute_code.execute_end
        MOV     byte [R15 + Z], 1
        JMP     execute_code.execute_end

set_bothflags:
        MOV     byte [R15 + Z], 0
        JNZ     .cflag
        MOV     byte [R15 + Z], 1
.cflag: 
        MOV     byte [R15 + C], 0
        JNC     execute_code.execute_end
        MOV     byte [R15 + C], 1
        JMP     execute_code.execute_end
