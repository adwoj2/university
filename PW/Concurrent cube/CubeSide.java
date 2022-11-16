package concurrentcube;

public class CubeSide {
    int[][] scianka;
    int side;
    int size;
    int obrot; //Zmienna przyjmujaca wartosci od 0 do 3 okreslajaca ilosc obrotow sciane zgodnie ze wskazówkami zegara.

    //konstruktor
    CubeSide(int size, int side) {
        scianka = new int[size][size];
        for (int i = 0; i < size; i++)
            for (int j = 0; j < size; j++)
                scianka[i][j] = side;
        this.side = side;
        this.size = size;
        this.obrot = 0;
    }
    //Metody zmieniajace odpowiednie rzedy z tabeli dwuwymiarowej opisujacej wartosci na sciance kostki.
    private void setWierszWlasciwa(int wiersz, int[] nowyWiersz) {
        for (int i = 0; i < size; i++)
            scianka[i][wiersz] = nowyWiersz[i];
    }
    private void setWierszOdTyluWlasciwa(int wiersz, int[] nowyWiersz) {
        for (int i = 0; i < size; i++)
            scianka[size - 1 - i][wiersz] = nowyWiersz[i];
    }
    private void setKolumnaWlasciwa(int kolumna, int[] nowaKolumna) {
        for (int i = 0; i < size; i++)
            scianka[kolumna][i] = nowaKolumna[i];
    }
    private void setKolumnaOdTyluWlasciwa(int kolumna, int[] nowaKolumna) {
        for (int i = 0; i < size; i++)
            scianka[kolumna][size - 1 -i] = nowaKolumna[i];
    }

    //Metoda wywołująca odpowiednie funkcje działające na tabeli w zależnośći od obrotu kostki
    //oraz metod które ją wywołały (poprzez wartość wzglobrot).
    private void setRzad(int wiersz, int[] nowyWiersz, int wzglobrot) {
        switch (wzglobrot) {
            case 0: setWierszWlasciwa(wiersz, nowyWiersz); break;
            case 1: setKolumnaOdTyluWlasciwa(wiersz, nowyWiersz); break;
            case 2: setWierszOdTyluWlasciwa(size - 1 - wiersz, nowyWiersz); break;
            case 3: setKolumnaWlasciwa(size - 1 - wiersz, nowyWiersz); break;
        }
    }
    //Metody zmieniajacy wartosci na sciance kostki w zaleznosci od tego ile razy zostala obrócona
    //poprzez metodę setRząd().
    public void setWiersz(int wiersz, int[] nowyWiersz) {
        setRzad(wiersz, nowyWiersz, obrot);
    }
    public void setKolumna(int kolumna, int[] nowaKolumna) {
        setRzad(size - 1 - kolumna, nowaKolumna, (obrot + 3) % 4);
    }
    public void setWierszOdTylu(int wiersz, int[] nowyWiersz) {
        setRzad(size - 1 - wiersz, nowyWiersz, (obrot + 2) % 4);
    }
    public void setKolumnaOdTylu(int kolumna, int[] nowaKolumna) {
        setRzad(kolumna, nowaKolumna, (obrot + 1) % 4);
    }
    public void obrotZgodnie(){
        obrot = (obrot + 1) % 4;
    }
    public void obrotPrzeciwnie(){
        obrot = (obrot + 3) % 4;
    }

    //Metody zwracajace odpowiednie rzedy z tabeli dwuwymiarowej opisujacej wartosci na sciance kostki.
    private int[] getWierszWlasciwa(int wiersz) {
        int[] nowyWiersz = new int[size];
        for (int i = 0; i < size; i++)
            nowyWiersz[i] = scianka[i][wiersz];
        return nowyWiersz;
    }
    private int[] getWierszOdTyluWlasciwa(int wiersz) {
        int[] nowyWiersz = new int[size];
        for (int i = 0; i < size; i++)
            nowyWiersz[i] = scianka[size - 1 - i][wiersz];
        return nowyWiersz;
    }
    private int[] getKolumnaWlasciwa(int kolumna) {
        int[] nowaKolumna = new int[size];
        for (int i = 0; i < size; i++)
            nowaKolumna[i] = scianka[kolumna][i];
        return nowaKolumna;
    }
    private int[] getKolumnaOdTyluWlasciwa(int kolumna) {
        int[] nowaKolumna = new int[size];
        for (int i = 0; i < size; i++)
            nowaKolumna[i] = scianka[kolumna][size - 1 - i];
        return nowaKolumna;
    }

    //Metoda wywołująca odpowiednie funkcje działające na tabeli w zależnośći od obrotu kostki.
    //oraz metod które ją wywołały (poprzez wartość wzglobrot)
    private int[] getRzad(int wiersz, int wzglobrot) {
        int[] nowyRzad = new int[size];
        switch (wzglobrot) {
            case 0: nowyRzad = getWierszWlasciwa(wiersz); break;
            case 1: nowyRzad = getKolumnaOdTyluWlasciwa(wiersz); break;
            case 2: nowyRzad = getWierszOdTyluWlasciwa(size - 1 - wiersz); break;
            case 3: nowyRzad = getKolumnaWlasciwa(size - 1 - wiersz); break;
        }
        return nowyRzad;
    }

    //Metody zwracajace wartosci ze sciance kostki w zaleznosci od tego ile razy zostala obrócona
    //poprzez metodę setRząd()
    public int[] getWiersz(int wiersz) {
        return getRzad(wiersz, obrot);
    }
    public int[] getKolumna(int kolumna) {
        return getRzad(size - 1 - kolumna, (obrot + 3) % 4);
    }
    public int[] getWierszOdTylu(int wiersz) {
        return getRzad(size - 1 - wiersz, (obrot + 2) % 4);
    }
    public int[] getKolumnaOdTylu(int kolumna) {
        return getRzad(kolumna,  (obrot + 1) % 4);
    }

    //Funkcja zwracajaca StringBuilder opisujacy scianke kostki wg specyfikacji podanej w zadaniu.
    public StringBuilder pokaz() {
        StringBuilder wynik = new StringBuilder();
        for (int i = 0; i < size; i++)
            for (int j = 0; j < size; j++)
                wynik.append(getWiersz(i)[j]);

        return wynik;
    }
}