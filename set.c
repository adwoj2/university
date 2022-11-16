#include <stdio.h>
#include <stdbool.h>

int licznik;//zmienna opisujaca ilo�� kart znajdujacych sie w danym momencie "na stole"

bool check(int a, int b, int c)
{
    int x = a % 10;
    int y = b % 10;
    int z = c % 10;
    if((x == y && x == z) || (x != y && y != z && x != z))
    {

        if(a / 10 == 0)
            return true;
        else
            return check(a / 10, b / 10, c / 10);
    }
    return false;
}/*funkcja przyjmujaca trzy inty z zakodowanym rodzajem karty (poczatkowo) sprawdza ona jedna ceche czy spelnia warunki
seta i zwraca :
a) w przypadku kiedy rozpatrywana cecha spelnia warunki i zostaly jeszcze inne cechy do sprawdzenia modyfikuje karty i wywoluje
sama siebie aby sprawdzona zostala kolejna cecha usuwajac wlasnie sprawdzona i przyjmuje wartosc logiczna wywolanej funkcji
b) jesli rozpatrywana cecha jest spelniona i nie ma juz wiecej cech do sprawdzenia zwraca prawde
c) jesli rozpatrywana cecha nie spelnia warunkow zwraca falsz
pierwotna funkcja (ta ktora nie zostaal wywolana rekurencyjnie) zwraca wiec wartosc logiczna prawda
jesli 3 przyjete zmienne reprezentowaly seta i falsz jesli nie
*/

void us_poj(int a, int t[])
{
    for(int i = a; i < licznik - 1; i++)
    {
        t[i] = t[i+1];


    }
    licznik--;
}/* procedura przyjmuje pozycje elementu i przesuwa wszystkie elementy na prawo od niego o 1 pole tablicy w lewo nadpisujac wybrany element
 dodatkowo zmniejsza wartosc zmiennej licznik tak wiec efektywnie usuwa dana karte z gry */

void usun (int a, int b, int c, int t[])
{
    printf("- %d %d %d\n", t[a], t[b], t[c]);
    us_poj(a, t);
    us_poj(b-1, t);
    us_poj(c-2, t);
}/*procedura przyjmuje 3 wartosci int reprezentujace indeksy kart i tablice t i wypisuje stosowny komunikat inicjalizujac na kazdej
karcie procedure usuwajaca ja ze stolu poprawka w 2 i 3 wywolaniu wynika z tego ze funkcja przyjmuje wartosc indeksu sprzed usuniecia
poprzedzajacej karty a operacja jej usuniecia juz jest wykonana w momencie inicjalizacji */

bool dodaj(int t[])
{
    int i = 0;
    int n;
    int result;
    do
    {
        result = scanf("%d", &n);
            if(result == 1)
            {
                t[licznik] = n;
                licznik++;
                i++;
            }
    }while(result != EOF && i < 3);
    if (result == EOF)
        return false;
    else
        return true;
}/*funkcja przyjmuje tablice i wczytuje z danych wej�ciowych 3 kolejne karty (jesli wczytana nie zostanie karta funkcja ja zignoruje)
funkcja zwraca falsz je�li dane wej�ciowe sie skoncza oraz prawde w przeciwnym wypadku.
*/
void wypisz(int t[])
{
    printf("=");
    for(int i = 0; i < licznik; i++)
        printf(" %d", t[i]);
    printf("\n");

}//procedura przyjmuje tablic� i wypisuje aktualny stan stolu gry poprzedzony znakiem "="

bool szukaj_setow(int t[])
{
    int a, b, c;
    for(int i = 0; i < licznik; i++)
        for(int j = i + 1; j < licznik; j++)
            for(int k = j + 1; k < licznik; k++)
            {
                a = t[i];
                b = t[j];
                c = t[k];
                if(check(a, b, c))
                {
                    usun(i, j, k, t);
                    return true;
                }
            }
    return false;
}/* funkcja przyjmuje tablice i szuka seta wsr�d kart aktualnie "na stole" wywolujac funkcje check
na kazdej trojce kart na stole w kolejnowci opisanej w tresci problemu
jesli znajdzie seta wsr�d nich wywoluje funkcje usuwajaca je z gry a takze zwraca wartosc prawda
jesli nie znajdzie seta, zwraca wartosc falsz*/
int main()
{
    bool koniec = false;
    int t[30];                      // tablica przyjmujaca karty
                                    //kart na stole z racji specyfiki gry nigdy nie bedzie wiecej niz 30 (ta liczba ma swoja droga duzy zapas dla pewnosci)


    for(int i = 0; i < 4; i++)      //gra rozpoczyna sie dopiero przy wylozeniu 12 kart tak wiec wywoluje 4 razy funkcje dodajaca 3 karty
        if(!dodaj(t))               //jesli dane sie skoncza wypisuje pocz�tkowy stan stolu teraz poniewa� nigdy nie wejde w "while" i koncze petle
        {
            wypisz(t);
            break;
        }
    while(licznik >= 12 && !koniec) //go�wna petla programu
    {
        wypisz(t);
        if(!szukaj_setow(t))        //je�li brak seta na stole wypisuje stosowny znak oraz dodaje karty
        {
            if(dodaj(t))
                printf("+\n");
            else
            {
                koniec = true;
            }
        }
        else if(licznik < 12)       //ten warunek zachodzi gdy zostanie zabrany set przy 12 kartach
        {                           //wtedy nalezy dobra� karty nie wypisujac znaku +
            if(!dodaj(t))
                {
                    koniec = true;
                    wypisz(t);
                }
        }
    }

    while(szukaj_setow(t))//koncowe szukanie setow we wszystkich pozostalych kartach
    {
        wypisz(t);
    }
}
