#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#define KONWERSJA 97//stala dzieku ktorej latwiej jest zaimplementowac zmienne oznaczone literami na liczby od 0 do 26
#define MAX_KOMORKI 10000//maksymalna wartosc w komorce tablicy reprezentujacej duza liczbe
#define ILOSC_ZMIENNYCH 26

//typ okreslajacy rodzaj instrukcji
enum inst
{
    INC,
    ADD,
    CLR,
    JMP,
    DJZ,
    HLT
};

//struktura zawierajaca dane na temat zmiennej
typedef struct dane_zmiennej
{
    int *wartosc;
    int komorki;
}dane_zmiennej;

//struktura zawierajaca instrukcje do wykonania w kodzie maszynowym
typedef struct pelna_instrukcja
{
    enum inst instrukcja;
    int pierwszy;
    int drugi_lub_adres;
}pelna_instrukcja;

//lista zawierajaca zawartosc kolejnych petli (nawiasy)
typedef struct lista_petli
{
    int adres;
    int iterator;
    struct lista_petli *nast;
}lista_petli;

int rozmiar_kodu = 0;
pelna_instrukcja **kod = NULL; //tablica dynamiczna zawierajaca kolejne instrukcje w kodzie maszynowym
dane_zmiennej *tablica[ILOSC_ZMIENNYCH]; //obslugiwane zmienne

//procedura dostosowujaca pamiec przeznaczona na kod maszynowy funkcja przewaznie zwieksza pamiec o 1 natomiast w przypadku upraszczania kodu moze te pamiec zmniejszyc
void dostosuj_pamiec()
{
    kod = (pelna_instrukcja**)realloc(kod, sizeof(pelna_instrukcja*) * (unsigned)rozmiar_kodu);
    kod[rozmiar_kodu - 1] = (pelna_instrukcja*)malloc(sizeof(pelna_instrukcja));
}

// procedura przyjmujaca kod ascii litery oznaczajacej dana zmienna i wypisujaca jej wartosc
void wypisz_zmienna(int wypisywana_zmienna)
{
    printf("%d", tablica[wypisywana_zmienna]->wartosc[tablica[wypisywana_zmienna]->komorki - 1]);
    for(int i = tablica[wypisywana_zmienna]->komorki - 2; i >= 0; i--)
        printf("%04d", tablica[wypisywana_zmienna]->wartosc[i]);
    printf("\n");
}

//funkcja przyjmujaca liste, adres poczatku nowej petli i zmienna ja iterujaca zwracajaca wskaznik na liste po dodaniu do niej dodatkowego elementu o podanych danych
lista_petli* dodaj_petle(lista_petli *wsk, int adr, int it)
{
    lista_petli *pom;
    pom = (lista_petli*)malloc(sizeof(lista_petli));
    pom->adres = adr;
    pom->iterator = it;
    pom->nast = wsk;
    return pom;
}

//funkcja przyjmujaca liste zwracajaca wskaznik na liste po usunieciu z niej ostatniej petli
lista_petli* usun_petle(lista_petli *wsk)
{
    lista_petli* pom = wsk->nast;
    free(wsk);
    return pom;
}

//procedura upraszczajaca kod maszynowy
void uproszczony_kod(int* tab, int ile, int iter)
{
    for(int i = 0; i < ile - 1; i++)
    {
        rozmiar_kodu++;
        dostosuj_pamiec();
        kod[rozmiar_kodu - 1]->instrukcja = ADD;
        kod[rozmiar_kodu - 1]->pierwszy = tab[i];
        kod[rozmiar_kodu - 1]->drugi_lub_adres = iter;
    }
    rozmiar_kodu++;
    dostosuj_pamiec();
    kod[rozmiar_kodu - 1]->instrukcja = CLR;
    kod[rozmiar_kodu - 1]->pierwszy = iter;
}

//procedura zerujaca wszystkie zmienne i przydzielajaca im poczatkowa pamiec
void zerowanie_i_przydzielenie_pamieci()
{
    for(int i = 0; i < ILOSC_ZMIENNYCH; i++)
    {
        tablica[i] = (dane_zmiennej*)malloc(sizeof(dane_zmiennej));
        tablica[i]->wartosc = (int*)malloc(sizeof(int));
        tablica[i]->wartosc[0] = 0;
        tablica[i]->komorki = 1;
    }
}

//procedura przyjmujaca kod zmiennej powiekszajaca rozmiar tablicy ja reprezentujacej i ustawiajaca wartosc nowej komorki na 1 
void dodaj_komorke_1_na_pocz(dane_zmiennej* zmienna)
{
    zmienna->komorki++;
    zmienna->wartosc = (int*)realloc(zmienna->wartosc, sizeof(int) * (unsigned)zmienna->komorki);
    zmienna->wartosc[zmienna->komorki - 1] = 1;
}

//procedura przyjmujaca kod zmiennej i zwiekszajaca przechowywana w niej wartosc o 1
void inkrementuj(int numer)
{
    dane_zmiennej* zmienna = tablica[numer];
    zmienna->wartosc[0]++;
    int i = 0;
    while(i < zmienna->komorki && zmienna->wartosc[i] >= MAX_KOMORKI)
    {
        zmienna->wartosc[i] = 0;
        if(i + 1 != zmienna->komorki)
            zmienna->wartosc[i + 1]++;
        i++;
    }
    if(i == zmienna->komorki)
        dodaj_komorke_1_na_pocz(zmienna);
}

//procedura przyjmujaca kod 2 zmiennych i dodajacych ich wartosci do siebie
void dodaj(int numer, int numer_dodawanego)
{
    dane_zmiennej* podstawowy = tablica[numer];
    dane_zmiennej* dodawany = tablica[numer_dodawanego];
    if(podstawowy->komorki < dodawany->komorki)
    {
        int roznica = dodawany->komorki - podstawowy->komorki;
        podstawowy->komorki = dodawany->komorki;
        podstawowy->wartosc = (int*)realloc(podstawowy->wartosc, sizeof(int) * (unsigned)podstawowy->komorki);
        podstawowy->wartosc[podstawowy->komorki - 1] = 0;
        for(int i = 1; i <= roznica; i++)
            podstawowy->wartosc[podstawowy->komorki - i] = 0;

    }
    for(int i = 0; i < dodawany->komorki; i++)
    {
        podstawowy->wartosc[i] += dodawany->wartosc[i];
    }
    for(int i = 0; i < podstawowy->komorki - 1; i++)
    {
        if(podstawowy->wartosc[i] >= MAX_KOMORKI)
        {
            podstawowy->wartosc[i] -= MAX_KOMORKI;
            podstawowy->wartosc[i + 1] += 1;
        }
    }
    if(podstawowy->wartosc[podstawowy->komorki - 1] >= MAX_KOMORKI)
    {
        podstawowy->wartosc[podstawowy->komorki - 1] -= MAX_KOMORKI;
        dodaj_komorke_1_na_pocz(podstawowy);
    }
}

//procedura przyjmujaca kod zmiennej i zerujaca ja oraz dostosowujaca pamiec
void wyzeruj(int numer)
{
    dane_zmiennej* zmienna = tablica[numer];
    zmienna->komorki = 1;
    zmienna->wartosc = (int*)realloc(zmienna->wartosc, sizeof(int) * (unsigned)zmienna->komorki);
    zmienna->wartosc[0] = 0;
}

//funkcja rozpatrujaca instrukcje w kodzie maszynowym DJZ
//przyjmuje kod zmiennej, adres do ktorego moze przeskoczyc oraz adres na ktorym jest ona wywolywana
//w zaleznosci od warunkow zwraca adres instrukcji rozpatrywanej nastepnie
int dekrementuj_lub_skacz (int numer, int adres, int adres_wlasny)
{
    dane_zmiennej* zmienna = tablica[numer];
    if(zmienna->komorki == 1 && zmienna->wartosc[0] == 0)
        return adres;
    else
    {
        zmienna->wartosc[0]--;
        int i = 0;
        while(zmienna->wartosc[i] < 0)
        {
            zmienna->wartosc[i + 1]--;
            zmienna->wartosc[i] += MAX_KOMORKI;
            if(zmienna->wartosc[i + 1] == 0)
            {
                zmienna->komorki--;
                zmienna->wartosc = (int*)realloc(zmienna->wartosc, sizeof(int) * (unsigned)zmienna->komorki);
            }
        }
    return adres_wlasny + 1;
    }
}

//procedura interpretujaca tablice struktur z poszczegolnymi instrukcjami kodu maszynowego i wykonujaca je wywolujac odpowiednie procedury i funkcje
void wykonaj_kod()
{
    int i = 0;
    bool przetwarzanie_kodu = true;
    while(przetwarzanie_kodu)
    {
        switch(kod[i]->instrukcja)
        {
        case INC:
            inkrementuj(kod[i]->pierwszy - KONWERSJA);
            i++;
            break;
        case ADD:
            dodaj(kod[i]->pierwszy - KONWERSJA, kod[i]->drugi_lub_adres - KONWERSJA);
            i++;
            break;
        case CLR:
            wyzeruj(kod[i]->pierwszy - KONWERSJA);
            i++;
            break;
        case JMP:
            i = kod[i]->drugi_lub_adres;
            break;
        case DJZ:
            i = dekrementuj_lub_skacz(kod[i]->pierwszy - KONWERSJA, kod[i]->drugi_lub_adres, i);
            break;
        case HLT:
            przetwarzanie_kodu = false;
            break;
        }
    }
}

//procedura usuwajaca wszystkie instrukcje z tablicy zawierajacej kod maszynowy
void wyczysc_kod ()
{
    for(int i = 0; i < rozmiar_kodu; i++)
        free(kod[i]);
    rozmiar_kodu = 0;
    kod = (pelna_instrukcja**)realloc(kod, 0);
}

int main()
{
    zerowanie_i_przydzielenie_pamieci();
    int act; //wczytywany znak okreslajacy instrukcje
    lista_petli *petla;
    petla = NULL;
    int ilosc_petli = 0;//zmienna zawierajaca ilosc aktualnie zagniezdzonych petli
    int *w_petli = NULL;//tablica zawierajaca wszystkie instrukcje z najbardziej zewnetrznej petli (wedle wiedzy programu)
    int ile_w_petli = 0;//ilosc znakow w najbardziej zewnetrznej petli (wedle wiedzy programu)
    bool uproszczenie = false;//zmienna bool okreslajaca czy aktualna petle mozna uproscic czy tez warunki do tego nie sa spelnione
    do//glowna petla programu
    {
        act = getchar();
        if(ilosc_petli > 0 && act == petla->iterator)
            uproszczenie = false;
        //zapisywanie aktualnego stanu petli gdyby miala zostac uproszczona
        if(uproszczenie)
        {
            ile_w_petli++;
            w_petli = (int*)realloc(w_petli, sizeof(int) * (unsigned)ile_w_petli);
            w_petli[ile_w_petli - 1] = act;
        }
        if  (act >= 'a' && act <= 'z')
        {
            rozmiar_kodu++;
            dostosuj_pamiec();
            kod[rozmiar_kodu - 1]->instrukcja = INC;
            kod[rozmiar_kodu - 1]->pierwszy = act;
        }
        if(act == '(')
        {
            int znak = getchar();
            uproszczenie = true;
            rozmiar_kodu++;
            dostosuj_pamiec();
            petla = dodaj_petle(petla, rozmiar_kodu, znak);
            ilosc_petli++;
            ile_w_petli = 0;
        }
        if(act == ')')
        {
            //w przypadku gdy petla nadaje sie do uproszczenia program usuwa niezoptymalizowany kod i za pomoca odpowiednich danych tworzy zoptymalizowany
            //w przeciwnym przypadku ustawia odpowiednie instrukcje JMP i DJZ
            if(uproszczenie)
            {
                for(int i = rozmiar_kodu; i >= petla->adres; i--)
                    free(kod[i - 1]);
                rozmiar_kodu = petla->adres - 1;
                uproszczony_kod(w_petli, ile_w_petli, petla->iterator);
                ile_w_petli = 0;
            }
            else
            {
                rozmiar_kodu++;
                dostosuj_pamiec();
                kod[petla->adres - 1]->instrukcja = DJZ;
                kod[petla->adres - 1]->pierwszy = petla->iterator;
                kod[petla->adres - 1]->drugi_lub_adres = rozmiar_kodu;
                //---------------------------------------------------
                kod[rozmiar_kodu - 1]->instrukcja = JMP;
                kod[rozmiar_kodu - 1]->drugi_lub_adres = petla->adres - 1;
            }
            ilosc_petli--;
            petla = usun_petle(petla);
            uproszczenie = false;
        }
        if(act == '=')
        {
            rozmiar_kodu++;
            dostosuj_pamiec();
            kod[rozmiar_kodu - 1]->instrukcja = HLT;
            act = getchar();
            int wypisywana_zmienna = act - KONWERSJA;
            if(kod != NULL)
            {
                wykonaj_kod();
                wyczysc_kod();
            }
            wypisz_zmienna(wypisywana_zmienna);
            petla = NULL;
            ilosc_petli = 0;
            w_petli = (int*)realloc(w_petli, 0);
            uproszczenie = false;
        }
    }while(act != EOF);
    wyczysc_kod();
    free(w_petli);
    free(kod);
    return 0;
}
