package concurrentcube;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.TimeUnit;

class CubeTest {
    private final int SIDES = 6;
    private final int OSIE = 3;
    private final int CONVERTER = 48;

    //Klasa wątków wykonująca metodę rotate na podanej kostce.
    public static class Rotator implements Runnable {
        int side;
        int layer;
        Cube cube;

        public Rotator(int side,int  layer, Cube cube) {
            this.side = side;
            this.layer = layer;
            this.cube = cube;
        }

        public void run() {
            try {
                cube.rotate(side, layer);
            } catch (InterruptedException e) {System.out.println("interrupted");}
        }
    }
    //Klasa wątków wykonująca metodę rotate
    public static class Shower implements Runnable {
        Cube cube;

        public Shower(Cube cube) {
            this.cube = cube;
        }

        public void run() {
            try {
                cube.show();
            } catch (InterruptedException e) {System.out.println("interrupted");}
        }
    }
    //Test sprawdzający poprawność metody show()
    @Test
    public void SimpleShowTest() throws InterruptedException {
        final String EXPECTED =
                "000"
                        + "000"
                        + "000"

                        + "111"
                        + "111"
                        + "111"

                        + "222"
                        + "222"
                        + "222"

                        + "333"
                        + "333"
                        + "333"

                        + "444"
                        + "444"
                        + "444"

                        + "555"
                        + "555"
                        + "555";


        Cube cube = new Cube(3,
                    (x, y) -> {  },
                    (x, y) -> { },
                    () -> { },
                    () -> { }
        );
        String state = cube.show();
        Assertions.assertEquals(EXPECTED, state);

    }

    //Test sprawdzający poprawność metody Rotate
    @Test
    public void SimpleRotateTest() throws InterruptedException {
        final String EXPECTED =
                "144"
                        + "403"
                        + "223"

                        + "013"
                        + "015"
                        + "555"

                        + "004"
                        + "225"
                        + "415"

                        + "001"
                        + "335"
                        + "225"

                        + "002"
                        + "441"
                        + "332"

                        + "123"
                        + "153"
                        + "144";

        Cube cube = new Cube(3,
                (x, y) -> {  },
                (x, y) -> { },
                () -> { },
                () -> { }
        );
        try {
            for (int side = 0; side < SIDES; side ++)
                cube.rotate(side, 0);
            String state = cube.show();

            Assertions.assertEquals(EXPECTED, state);
        } catch (InterruptedException e) {
            System.out.println("interrupted");
            throw e;
        }
    }

    //Test sprawdzający współbieżność rotacji.
    @Test
    public void ConcurrencyRotateTest() throws InterruptedException {
        boolean blad = false;

        Cube cube = new Cube(3,
                (x, y) -> { try {
                    TimeUnit.SECONDS.sleep(1);
                } catch (InterruptedException e ){System.out.println("ERROR");} },
                (x, y) -> { },
                () -> { },
                () -> { }
        );
        List<Thread> threads = new ArrayList<>();
        for (int side = 0; side < SIDES; side++) {
            for (int layer = 0; layer < cube.size; layer++) {
                Rotator runnable = new Rotator(side % SIDES, layer, cube);
                Thread t = new Thread(runnable);
                threads.add(t);
            }
        }
        Instant start = Instant.now();

        for (Thread thread : threads) {
            thread.start();
        }

        for (Thread thread : threads) {
            thread.join();
        }

        Duration duration = Duration.between(start, Instant.now());
        Duration durationLimit = Duration.ofSeconds(8);
        Duration difference = durationLimit.minus(duration);
        if (difference.isNegative())
            blad = true;
        /*Gdyby jakieś 2 blokujące się obroty przeprowadziłyby operację rotate współbieżnie,
        z dużym prawdopodobieństwem niektóre elementy zostałyby zduplikowane a niektóre nadpisane,
        stąd taka forma sprawdzenia poprawności. Nie udało mi się wymyśleć lepszej w teście tego typu.*/
        String state = cube.show();
        int[] kolory = new int[SIDES];
        for (int i = 0; i < SIDES; i++)
            kolory[i] = 0;
        for (int i = 0; i < SIDES * cube.size * cube.size; i++)
            kolory[state.charAt(i) - CONVERTER]++;
        for (int i = 0; i < SIDES; i++)
            if (kolory[i] != cube.size * cube.size) {
                blad = true;
                break;
            }

            Assertions.assertFalse(blad);
    }

    //Test sprawdzający współbieżność funkcji show.
    @Test
    public void ShowConcurrencyTest(){
        final int ILE_WATKOW = 5;
        boolean blad = false;

        Cube cube = new Cube(200,
                (x, y) -> { },
                (x, y) -> { },
                () -> { try {
                    TimeUnit.SECONDS.sleep(1);
                } catch (InterruptedException e ){} },
                () -> { }
        );
        List<Thread> threads = new ArrayList<>();
        for(int side = 0; side < ILE_WATKOW; side++) {
            Shower runnable = new Shower(cube);
            Thread t = new Thread(runnable);
            threads.add(t);
        }

        Instant start = Instant.now();
        for (Thread thread : threads) {
            thread.start();
        }

        for (Thread thread : threads) {
            try {
                thread.join();
            } catch (InterruptedException e) {
                System.err.println("Main thread interrupted");
            }
        }

        Duration duration = Duration.between(start, Instant.now());
        Duration durationLimit = Duration.ofSeconds(2);
        Duration difference = durationLimit.minus(duration);
        if (difference.isNegative())
            blad = true;

        Assertions.assertFalse(blad);
    }

    //Test sprawdzający czy wątki wykonujące blokujące akcje poprawnie się wykluczają.
    @Test
    public void BlockingTest() {
        boolean blad = false;
        final int ILE_WATKOW = 50;
            Cube cube = new Cube(5,
                    (x, y) -> {try {
                        TimeUnit.MILLISECONDS.sleep(100);
                    } catch (InterruptedException e ){System.out.println("ERROR");}},
                    (x, y) -> {},
                    () -> { },
                    () -> { }
                );

            List<Thread> threads = new ArrayList<>();
            for (int side = 0; side < OSIE; side++)
                for (int i = 0; i < ILE_WATKOW; i++) {
                    Rotator runnable = new Rotator(side , 0, cube);
                    Thread t = new Thread(runnable);
                    threads.add(t);
                }
            for (int side = OSIE; side < SIDES; side++)
                for (int i = 0; i < ILE_WATKOW; i++) {
                    Rotator runnable = new Rotator(side, cube.size - 1, cube);
                    Thread t = new Thread(runnable);
                    threads.add(t);
                }

            Instant start = Instant.now();
            for (Thread thread : threads) {
                thread.start();
            }

            for (Thread thread : threads) {
                try {
                    thread.join();
                } catch (InterruptedException e) {
                    System.err.println("Main thread interrupted");
                }
            }

            Duration duration = Duration.between(start, Instant.now());
            Duration durationLimit = Duration.ofSeconds(30);
            Duration difference = duration.minus(durationLimit);
            if (difference.isNegative())
                blad = true;

            Assertions.assertFalse(blad);


    }

    //Test tworzący wątki wykonujące losowo obroty losowych warstw kostki lub metodę show() (Czas rzedu ok 3-4 min)
    @Test
    public void RandomStressTest() {
        final int STRESS_OPERATIONS = 100000;
        final int SHOW_RATIO = 6 * 5;
        boolean blad = false;
        Cube cube = new Cube(200,
                (x, y) -> {
                },
                (x, y) -> {
                },
                () -> {
                },
                () -> {
                }
        );
        List<Thread> threads = new ArrayList<>();
        Random rand = new Random();
        for (int i = 0; i < STRESS_OPERATIONS; i++) {
            int side = rand.nextInt(SHOW_RATIO + 1);
            int layer = rand.nextInt(cube.size);
            if (side == SHOW_RATIO) {
                Shower runnable = new Shower(cube);
                Thread t = new Thread(runnable);
                threads.add(t);
            } else {
                Rotator runnable = new Rotator(side % SIDES, layer, cube);
                Thread t = new Thread(runnable);
                threads.add(t);
            }
        }

        for (Thread thread : threads) {
            thread.start();
        }

        for (Thread thread : threads) {
            try {
                thread.join();
            } catch (InterruptedException e) {
                System.err.println("Tread interrupted");
            }
        }
        String state = "";
        /*Gdyby jakieś 2 blokujące się obroty przeprowadziłyby operację rotate współbieżnie,
        z dużym prawdopodobieństwem niektóre elementy zostałyby zduplikowane a niektóre nadpisane,
        stąd taka forma sprawdzenia poprawności. Nie udało mi się wymyśleć lepszej w teście tego typu.*/
        try {
            state = cube.show();
        } catch (InterruptedException e) {blad = true;}
        int[] kolory = new int[SIDES];
        for (int i = 0; i < SIDES; i++)
            kolory[i] = 0;
        for (int i = 0; i < SIDES * cube.size * cube.size; i++)
            kolory[state.charAt(i) - CONVERTER]++;
        for (int i = 0; i < SIDES; i++)
            if (kolory[i] != cube.size * cube.size) {
                blad = true;
                break;
            }
            Assertions.assertFalse(blad);
    }

    //Test wykonujący dużą ilość losowych obrotów (czas rzędu kiludziesieciu sekund)
    @Test
    public void StressRotateTest() {
        final int STRESSCONST = 100000;
        boolean blad = false;
        Cube cube = new Cube(200,
                (x, y) -> {
                },
                (x, y) -> {
                },
                () -> {
                },
                () -> {
                }
        );
        List<Thread> threads = new ArrayList<>();
        Random rand = new Random();
        for (int i = 0; i < STRESSCONST; i++) {
            int side = rand.nextInt(6);
            int layer = rand.nextInt(cube.size);

            Rotator runnable = new Rotator(side, layer, cube);
            Thread t = new Thread(runnable);
            threads.add(t);
        }

        for (Thread thread : threads) {
            thread.start();
        }

        for (Thread thread : threads) {
            try {
                thread.join();
            } catch (InterruptedException e) {
                System.err.println("Tread interrupted");
            }
        }
        String state = "";
        try {
            state = cube.show();
        } catch (InterruptedException e) {blad = true;}
        /*Gdyby jakieś 2 blokujące się obroty przeprowadziłyby operację rotate współbieżnie,
        z dużym prawdopodobieństwem niektóre elementy zostałyby zduplikowane a niektóre nadpisane,
        stąd taka forma sprawdzenia poprawności. Nie udało mi się wymyśleć lepszej w teście tego typu.*/

        int[] kolory = new int[SIDES];
        for (int i = 0; i < SIDES; i++)
            kolory[i] = 0;
        for (int i = 0; i < SIDES * cube.size * cube.size; i++)
            kolory[state.charAt(i) - CONVERTER]++;
        for (int i = 0; i < SIDES; i++)
            if (kolory[i] != cube.size * cube.size) {
                blad = true;
            }
        Assertions.assertFalse(blad);
    }
}