#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>

#define ROCKFORD '@'
#define PUSTE_POLE ' '
#define ZIEMIA '+'
#define SKALA '#'
#define KAMIEN 'O'
#define DIAMENT '$'
#define WYJSCIE 'X'
#define GORA 'w'
#define LEWO 'a'
#define DOL 's'
#define PRAWO 'd'
#define KONIEC_LINII '\n'

char **tablica;
bool rock_jest;
typedef struct wsp
    {
        int *x;
        int *y;
    }wsp;

//uzywane w komentarzach pojecie wspolrzedne docelowe oznaczaja wspolrzedne punktu na jaki ma przesunac sie znak ROCKFORD

/*procedura wczytujaca poczatkowy uklad planszy do tablicy dwuwymiarowej tablica
przyjmuje ilosc wierszy w, kolumn k, oraz strukture zawierajaca zmienne wskaznikowe na polozenie znaku ROCKFORD*/
void wczytaj_plansze(int w, int k, wsp rockford)
{
    tablica = (char**)malloc((long unsigned int)w * sizeof(char*));
    for(int i = 0; i < w; i++)
        tablica[i] = (char*)malloc((long unsigned int)k * sizeof(char));
    for(int i = 0; i < w; i++)
        for(int j = 0; j < k; j++)
        {
            tablica[i][j] = (char)getchar();
            if(tablica[i][j] == ROCKFORD)
            {
                *rockford.y = i;
                *rockford.x = j;
                rock_jest = true;//bool rownowazny z rockfordem znajdujacym sie na planszy
            }
            if(tablica[i][j] == KONIEC_LINII)
                j--;
        }
}

//funkcja przyjmujaca ilosc wierszy i kolumn i zwracajaca prawde jesli wszystkie diamenty zostaly zebrane i falsz w przeciwnym wypadku
bool diamenty(int w, int k)
{
    for(int i = 0; i < w; i++)
        for(int j = 0; j < k; j++)
        {
        if(tablica[i][j] == DIAMENT)
            return false;
        }
    return true;
}

/*procedura przyjmujaca strukture ze wskaznikami na wspolrzedne rockforda, ktora "znika" znak ROCKFORD z planszy i
zapobiega wykonywaniu funkcji zmieniajacych jego polozenie, ustawiajac bool rock_jest na falsz*/
void zakoncz(wsp rockford)
{
    rock_jest = false;
    tablica[*rockford.y][*rockford.x] = PUSTE_POLE;
}

//procedura przyjmujaca ilosc wierszy i kolumn wypisujaca aktualny stan planszy
void wypisz_plansze(int w, int k)
{
    printf("%d %d\n", w, k);
    for(int i = 0; i < w; i++)
    {
        for(int j = 0; j < k; j++)
            printf("%c", tablica[i][j]);
        printf("\n");
    }
}

/*procedura przyjmujaca strukture zawierajaca zmienne wskaznikowe wskazujace na aktualne wspolrzedne znaku ROCKFORD
oraz wspolrzedne docelowe i przesuwajaca znak ROCKFORD na wspolrzedne docelowe*/
void ruch(wsp rockford, int x, int y)
{
    tablica[y][x] = ROCKFORD;
    tablica[*rockford.y][*rockford.x] = PUSTE_POLE;
    *rockford.x = x;
    *rockford.y = y;
}

/*procedura przyjmujaca strukture zawierajaca zmienne wskaznikowe wskazujace na aktualne wspolrzedne znaku ROCKFORD i wspolrzedne docelowe
funkcja sprawdza czy to mozliwe i jesli tak to przesuwa kamien znajdujacy sie na miejscu docelowym na pole dalej
oraz za pomoca funkcji ruch znak ROCKFORD*/
void przesun(wsp rockford, int x, int y)
{
    if(*rockford.x - x == 1)
    {
        if(tablica[y][x - 1] == PUSTE_POLE)
        {
            tablica[y][x - 1] = KAMIEN;
            ruch(rockford, x, y);
        }
    }
    else if (*rockford.x - x == -1)
        if(tablica[y][x + 1] == PUSTE_POLE)
        {
            tablica[y][x + 1] = KAMIEN;
            ruch(rockford, x, y);
        }
}

/* procedura przyjmuje strukture zawierajaca zmienne wskaznikowe wskazujace na aktualne polozenie znaku ROCKFORD oraz zmienne docelowe oraz ilosc wierszy i kolumn
nastepnie wywoluje odpowiednie funkcje w zaleznosci od tego co znajduje sie na wspolrzednych docelowych zgodnie z trescia zadania*/
void rozpatrz (wsp rockford, int x, int y, int w, int k)
{
    switch(tablica[y][x])
    {
        case PUSTE_POLE:
            ruch(rockford, x, y);
            break;
        case ZIEMIA:
            ruch(rockford, x, y);
            break;
        case SKALA:
            break;
        case KAMIEN:
            przesun(rockford, x, y);
            break;
        case DIAMENT:
            ruch(rockford, x, y);
            break;
        case WYJSCIE:
            if(diamenty(w, k))
                zakoncz(rockford);
            break;
    }
}

//procedura przyjmujaca kolumne oraz ilosc wierszy na planszy i stabilizujaca ja "od dolu", tak aby nizsze obiekty spadaly pierwsze
void ustabilizuj_kolumne(int kol, int w)
{
    int ilepol = 0;
    int ileobiektow = 0;
    int a = w - 1;
    for(int i = a; i >= 0; i--)
    {
        while(tablica[i - ileobiektow][kol] == KAMIEN || tablica[i - ileobiektow][kol] == DIAMENT)
            ileobiektow++;
        if(tablica[i][kol] == KAMIEN || tablica[i][kol] == DIAMENT)
            while(tablica[i + ilepol + 1][kol] == PUSTE_POLE)
                ilepol++;
        for(int j = 0; j < ilepol; j++)
        {
            for(int trzeci = 0; trzeci < ileobiektow; trzeci++)
                tablica[i + j - trzeci + 1][kol] = tablica[i + j - trzeci][kol];
            tablica[i - ileobiektow + 1 + j][kol] = PUSTE_POLE;
        }
        ilepol = 0;
        ileobiektow = 0;
    }
}

/*procedura przyjmujaca numer kolumny i ilosc wierszy na planszy oraz stabilizujaca za pomoca procedury ustabilizuj_kolumne
dwie sasiadujace z wybrana kolumna kolumny
uzywam jej z powodu spostrzezenia ze przesuniecie rockforda poziomo, moze zdestabilizowac jedynie kolumne z ktorej wlasnie zszedl
oraz kolumne o jeden przed nim, na ktora mogl wtoczyc kamien czyli wlasnie na 2 kolumny sasiadujace z jego aktualnym polozeniem*/
void ustabilizuj_wokol(wsp rockford, int w)
{
    ustabilizuj_kolumne(*rockford.x + 1, w);
    ustabilizuj_kolumne(*rockford.x - 1, w);
}

//procedura przyjmujaca ilosc wierszy i kolumn planszy wywolujaca procedure ustabilizuj_kolumne na kazdej kolumnie planszy efektywnie stabilizujac cala plansze
void ustabilizuj(int w, int k)
{
    for(int i = 0; i < k; i++)
    {
        ustabilizuj_kolumne(i, w);
    }
}

//procedura przyjmujaca ilosc wierszy oraz zmienne wskaznikowe i zwalniajaca uprzednio zajmowana za pomoca funkcji malloc przez nie pamiec
int main()
{
    int w, k; //ilosc wierszy i kolumn
    char act = 'u'; //zmienna przyjmujaca polecenie ustawiona na znak 'u' tak aby przypadkiem nie ustawila sie na EOF co przerwaloby wykonywanie programu
    wsp rockford;//struktura ze wspolrzednymi znaku ROCKFORD
    rockford.x = malloc(sizeof(int));//alokacja pamieci
    rockford.y = malloc(sizeof(int));
    scanf("%d%d", &w, &k);
    wczytaj_plansze(w, k, rockford);
    ustabilizuj(w, k);//stabilizacja kazdej kolumny planszy
    while(act != EOF)//glowna petla progarmu
    {
        act = (char)getchar();
        switch(act)
        {
            case GORA:
                if(rock_jest)
                    rozpatrz(rockford, *rockford.x, *rockford.y - 1, w, k);
                break;
            case DOL:
                if (rock_jest)
                {
                    rozpatrz(rockford, *rockford.x, *rockford.y + 1, w, k);
                    ustabilizuj_kolumne(*rockford.x, w);
                }
                break;
            case LEWO:
                if (rock_jest)
                {
                    rozpatrz(rockford, *rockford.x - 1, *rockford.y, w, k);
                    ustabilizuj_wokol(rockford, w);
                }
                break;
            case PRAWO:
                if (rock_jest)
                {
                    rozpatrz(rockford, *rockford.x + 1, *rockford.y, w, k);
                    ustabilizuj_wokol(rockford, w);
                }
                break;
            case KONIEC_LINII:
                wypisz_plansze(w, k);
                break;
        }
    }
    for(int i = 0; i < w; i ++)
        free(tablica[i]);
    free(tablica);
    free(rockford.x);
    free(rockford.y);//zwalnianie pamieci
    return 0;
}
