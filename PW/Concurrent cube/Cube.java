package concurrentcube;

import java.util.concurrent.Semaphore;
import java.util.function.BiConsumer;

public class Cube {
    public int size;
    private BiConsumer<Integer, Integer> beforeRotation;
    private BiConsumer<Integer, Integer> afterRotation;
    private Runnable beforeShowing;
    private Runnable afterShowing;

    public CubeSide[] kostka;
    Semaphore ochrona; //mutex

    //Semafory binarne pozwalające zmieniać tylko jedną warstwę w danej osi.
    Semaphore[] goradol;
    Semaphore[] lewoprawo;
    Semaphore[] przodtyl;

    //Semafory regulujące które wątki mogą wykonywać pracę zapewniając żywotność.
    Semaphore wejscie[];
    Semaphore poczekalnia;
    Semaphore pokazywacz;

    //Liczniki wątków w odpowiednich fragmentach programu
    int ileChcePokazac;
    int ileChceObracac;
    int ileObraca;
    int ilePokazuje;
    int ileZaPoczekalnia;
    int[] ileCzekaNaWejsciu;

    //Wartość oznaczająca oś której wątki właśnie wykonują lub mają pozwolenie na wykonywanie pracy
    //0 - goradol, 1 - lewoprawo, 2 - przodtyl
    int aktualnaOs;

    //konstruktor
    public Cube(int size,
                BiConsumer<Integer, Integer> beforeRotation,
                BiConsumer<Integer, Integer> afterRotation,
                Runnable beforeShowing,
                Runnable afterShowing) {
        this.size = size;
        this.beforeRotation = beforeRotation;
        this.afterRotation = afterRotation;
        this.beforeShowing = beforeShowing;
        this.afterShowing = afterShowing;

        this.kostka = new CubeSide[6];
        for (int i = 0; i < 6; i++)
            kostka[i] = new CubeSide(size, i);
        ochrona = new Semaphore(1);

        goradol = new Semaphore[size];
        przodtyl = new Semaphore[size];
        lewoprawo = new Semaphore[size];
        for (int i = 0; i < size; i ++) {
            goradol[i] = new Semaphore(1);
            przodtyl[i] = new Semaphore(1);
            lewoprawo[i] = new Semaphore(1);
        }

        wejscie = new Semaphore[3];//0 goradol 1 lewoprawo 2 przodtyl
        for (int i = 0; i < 3; i++)
            wejscie[i] = new Semaphore(0);
        poczekalnia = new Semaphore(0);
        pokazywacz = new Semaphore(0);

        ileChcePokazac = 0;
        ileChceObracac = 0;
        ileObraca = 0;
        ilePokazuje = 0;
        ileZaPoczekalnia = 0;
        ileCzekaNaWejsciu = new int[3];

        aktualnaOs = -1;
    }

    //Metody wywołujące odpowiednie metody na ściankach kostki w zależności od aktualnie obracanej warstwy.
    public void rotate0(int layer) throws InterruptedException {
        int[] przechowalnia = new int[size];
        przechowalnia = kostka[2].getWiersz(layer);
        kostka[2].setWiersz(layer, kostka[3].getWiersz(layer));
        kostka[3].setWiersz(layer, kostka[4].getWiersz(layer));
        kostka[4].setWiersz(layer, kostka[1].getWiersz(layer));
        kostka[1].setWiersz(layer, przechowalnia);
    }

    public void rotate1(int layer) throws InterruptedException {
        int[] przechowalnia = new int[size];
        przechowalnia = kostka[0].getKolumna(layer);
        kostka[0].setKolumna(layer, kostka[4].getKolumnaOdTylu(size - 1 - layer));
        kostka[4].setKolumnaOdTylu(size - 1 - layer, kostka[5].getKolumna(layer));
        kostka[5].setKolumna(layer, kostka[2].getKolumna(layer));
        kostka[2].setKolumna(layer, przechowalnia);
    }

    public void rotate2(int layer) throws InterruptedException {
        int[] przechowalnia = new int[size];
        przechowalnia = kostka[0].getWiersz(size - 1 - layer);
        kostka[0].setWiersz(size - 1 - layer, kostka[1].getKolumnaOdTylu(size - 1 - layer));
        kostka[1].setKolumnaOdTylu(size - 1 - layer, kostka[5].getWierszOdTylu(layer));
        kostka[5].setWierszOdTylu(layer, kostka[3].getKolumna(layer));
        kostka[3].setKolumna(layer, przechowalnia);
    }

    public void rotate3(int layer) throws InterruptedException {
        layer = size - 1 - layer;
        int[] przechowalnia = new int[size];
        przechowalnia = kostka[0].getKolumna(layer);
        kostka[0].setKolumna(layer, kostka[2].getKolumna(layer));
        kostka[2].setKolumna(layer, kostka[5].getKolumna(layer));
        kostka[5].setKolumna(layer, kostka[4].getKolumnaOdTylu(size - 1 - layer));
        kostka[4].setKolumnaOdTylu(size - 1 - layer, przechowalnia);
    }

    public void rotate4(int layer) throws InterruptedException {
        layer = size - 1 - layer;
        int[] przechowalnia = new int[size];
        przechowalnia = kostka[0].getWiersz(size - 1 - layer);
        kostka[0].setWiersz(size - 1 - layer, kostka[3].getKolumna(layer));
        kostka[3].setKolumna(layer, kostka[5].getWierszOdTylu(layer));
        kostka[5].setWierszOdTylu(layer, kostka[1].getKolumnaOdTylu(size - 1 - layer));
        kostka[1].setKolumnaOdTylu(size - 1 - layer, przechowalnia);

    }

    public void rotate5(int layer) throws InterruptedException {
        layer = size - 1 - layer;
        int[] przechowalnia = new int[size];
        przechowalnia = kostka[2].getWiersz(layer);
        kostka[2].setWiersz(layer, kostka[1].getWiersz(layer));
        kostka[1].setWiersz(layer, kostka[4].getWiersz(layer));
        kostka[4].setWiersz(layer, kostka[3].getWiersz(layer));
        kostka[3].setWiersz(layer, przechowalnia);
    }


    //Metoda realizująća protokół wstępny metody rotate.
    public void dostepRotate(int os) throws InterruptedException {
        ochrona.acquireUninterruptibly();
        if (ileChcePokazac + ilePokazuje > 0) {
            ileChceObracac++;
            ochrona.release();

            try {
                poczekalnia.acquire();      //przejecie sekcji krytycznej
            } finally {
                ileChceObracac--;
            }
        }
        ileZaPoczekalnia++;

        if (ileChceObracac > 0) {
            poczekalnia.release();
        } else {
            ochrona.release();
        }

        ochrona.acquireUninterruptibly();
        if (aktualnaOs == -1)
            aktualnaOs = os;

        if (aktualnaOs != os || ileCzekaNaWejsciu[0] + ileCzekaNaWejsciu[1] + ileCzekaNaWejsciu[2] > 0) {
            ileCzekaNaWejsciu[os]++;
            ochrona.release();

            try {
                wejscie[os].acquire(); //przekazanie sekcji krytycznej
            } finally {
                ileCzekaNaWejsciu[os]--;
            }

            if (ileCzekaNaWejsciu[os] > 0) {
                ileObraca++;
                wejscie[os].release();
            } else {
                ochrona.release();
            }
        } else {
            ileObraca++;
            ochrona.release();
        }
    }

    //Metoda realizująća protokół końcowy metody rotate()
    public void wyjscieRotate() throws InterruptedException {
        ochrona.acquireUninterruptibly();
        ileObraca--;
        ileZaPoczekalnia--;

        if (ileChcePokazac > 0 && ileZaPoczekalnia == 0) {
            aktualnaOs = -1;
            pokazywacz.release(); // przekazanie sekcji krytycznej
        } else if (ileObraca == 0) {
            aktualnaOs = (aktualnaOs + 1) % 3;

            if (ileCzekaNaWejsciu[aktualnaOs] + ileCzekaNaWejsciu[(aktualnaOs + 1) % 3] + ileCzekaNaWejsciu[(aktualnaOs + 2) % 3] == 0) {
                aktualnaOs = -1;
            }

            if (aktualnaOs != -1) {
                if (ileCzekaNaWejsciu[aktualnaOs] == 0)
                    aktualnaOs = (aktualnaOs + 1) % 3;
                if (ileCzekaNaWejsciu[aktualnaOs] == 0)
                    aktualnaOs = (aktualnaOs + 1) % 3;

                ileObraca++;
                wejscie[aktualnaOs].release(); // przekazanie sekcji krytycznej
            } else {
                ochrona.release();
            }
        } else
            ochrona.release();
    }

    public void dostepShow() throws InterruptedException {
        ochrona.acquireUninterruptibly();
        if (ileZaPoczekalnia + ileChceObracac > 0) {
            ileChcePokazac++;
            ochrona.release();

            try {
                pokazywacz.acquire(); //przejecie sekcji krytycznej
            } finally {
                ileChcePokazac--;
            }
        }
        ilePokazuje++;
        if (ileChcePokazac> 0) {
            pokazywacz.release();
        } else {
            ochrona.release();
        }
    }

    public void wyjscieShow() throws InterruptedException {
        ochrona.acquireUninterruptibly();
        ilePokazuje--;

        if (ileChceObracac > 0)
            poczekalnia.release();
        else if (ileChcePokazac > 0) {
            pokazywacz.release();
        }
        else
            ochrona.release();
    }


    public void rotate(int side, int layer) throws InterruptedException {
        try {
            if (side == 0 || side == 5)
                dostepRotate(0);
            if (side == 1 || side == 3)
                dostepRotate(1);
            if (side == 2 || side == 4)
                dostepRotate(2);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw e;
        }
        switch (side) {
            case 0: goradol[layer].acquireUninterruptibly(); break;
            case 1: lewoprawo[layer].acquireUninterruptibly(); break;
            case 2: przodtyl[layer].acquireUninterruptibly(); break;
            case 3: lewoprawo[size - 1 - layer].acquireUninterruptibly(); break;
            case 4: przodtyl[size - 1 - layer].acquireUninterruptibly(); break;
            case 5: goradol[size - 1 - layer].acquireUninterruptibly(); break;
        }

        beforeRotation.accept(side, layer);

        if (layer == 0)
            kostka[side].obrotZgodnie();
        if (layer == size - 1)
            kostka[side].obrotPrzeciwnie();

        switch (side) {
            case 0: rotate0(layer); break;
            case 1: rotate1(layer); break;
            case 2: rotate2(layer); break;
            case 3: rotate3(layer); break;
            case 4: rotate4(layer); break;
            case 5: rotate5(layer); break;
        }

        afterRotation.accept(side, layer);

        switch (side) {
            case 0: goradol[layer].release(); break;
            case 1: lewoprawo[layer].release(); break;
            case 2: przodtyl[layer].release(); break;
            case 3: lewoprawo[size - 1 - layer].release(); break;
            case 4: przodtyl[size - 1 - layer].release(); break;
            case 5: goradol[size - 1 - layer].release(); break;
        }

        wyjscieRotate();
    }

    public String show() throws InterruptedException {
        dostepShow();
        beforeShowing.run();
        StringBuilder wynik = new StringBuilder();

        for (int i = 0; i < 6; i++)
            wynik.append(kostka[i].pokaz());

        afterShowing.run();
        wyjscieShow();
        return wynik.toString();
    }
}