# OpenMP sprawozdanie

## Autorzy: Jakub Rękas, Ernest Szlamczyk

## Spis treści

1. [Wstęp](#1-wstęp)
2. [Tworzenie losowych liczb](#2-tworzenie-losowych-liczb)
3. [Poszczególne warianty implementacji algorytmu](#3-poszczególne-warianty-implementacji-algorytmu)
    - [Wariant 2](#wariant-2)
    - [Wariant 3](#wariant-3)
4. [Uruchamianie na Aresie](#4-uruchamianie-na-aresie)
5. [Wyniki](#5-wyniki)


## 1. Wstęp

Celem tego sprawozdania jest przedstawienie wykorzystania `OpenMP` - bazowanego na dyrektywach API stworzonego do tworzenia równoległych programów na współdzielonych architekturach pamięciowych. W tym sprawozdaniu będzie badany wpływ różnych funkcji `OpenMP` na performance poszczególnych części algorytmu **sortowania kubełkowego** (*ang. bucket sort*). 

W całym sprawozdaniu używamy następujących oznaczeń:

- $n$ - liczba elementów tablicy wejściowej,
- $k$ - liczba kubełków (w implementacji `n_buckets`; przy domyślnych parametrach $k = n$),
- $p$ - liczba wątków OpenMP (w implementacji `n_threads = omp_get_max_threads()`),
- $c$ - pojemność startowa pojedynczego prywatnego kubełka (w implementacji $c = 10$).

### Sortowanie kubełkowe

Sortowanie kubełkowe (*ang. bucket sort*) jest algorytmem sortowania działającym w czasie liniowym dla danych o jednostajnym rozkładzie. Dane wejściowe dzielone są na $k$ kubełków, z których każdy odpowiada pewnemu przedziałowi wartości. Algorytm składa się z czterech faz:

1. **(b) Rozdział** - każdy element tablicy trafia do kubełka odpowiadającego jego wartości.
2. **(c) Sortowanie kubełków** - każdy kubełek sortowany jest niezależnie dowolnym algorytmem porównawczym (np. quicksort).
3. **(d) Wyznaczenie pozycji zapisu** - dla każdego kubełka obliczana jest pozycja startowa w tablicy wynikowej (prefix sum rozmiarów kubełków).
4. **(d) Zapis** - posortowane kubełki przepisywane są kolejno do tablicy wyjściowej.

**Złożoność** przy $k$ kubełkach i $n$ elementach:

| Przypadek | Złożoność |
|---|---|
| Oczekiwana (rozkład jednostajny, $k = n$) | $O(n)$ |
| Pesymistyczna (wszystkie elementy w jednym kubełku) | $O(n \log n)$ |

**Potencjalne miejsca zrównoleglenia:**

| Faza | Niezależność iteracji | Możliwość zrównoleglenia | Uwaga |
|---|---|---|---|
| (b) Rozdział | Nie, ponieważ istnieje sytuacja gdzie wiele wątków może pisać do tego samego kubełka | Tak, ale wymaga synchronizacji | Konieczne blokady lub eliminacja współdzielenia |
| (c) Sortowanie kubełków | Tak - kubełki są od siebie niezależne | Tak | Przy $k = n$ kubełki są niemal równe, więc `schedule(static)` wystarczy; dynamiczne byłoby korzystne przy mocno nierównych rozmiarach |
| (d) Prefix sum | Nie - każdy termin zależy od poprzedniego | Zasadniczo sekwencyjne | $O(k)$ - koszt pomijalny wobec pozostałych faz |
| (d) Zapis | Tak - każdy kubełek trafia do rozłącznego fragmentu tablicy | Tak | Brak wyścigu przy zapisie |

Wąskim gardłem jest **faza rozdziału** - jedyna wymagająca synchronizacji przy równoległym wykonaniu. Dalsze sekcje opisują dwie strategie jej zrównoleglenia: ochronę kubełków blokadami (wariant 2) oraz eliminację współdzielenia przez prywatne kubełki per wątek (wariant 3).

## 2. Tworzenie losowych liczb

### Implementacja

Tablica wejściowa wypełniana jest liczbami pseudolosowymi z przedziału $[0, n)$ przy użyciu funkcji `erand48`. Każdy wątek inicjalizuje własny, niezależny stan generatora (`xsubi[3]`) entropią systemową pobraną przez `getentropy` - dzięki temu wątki nie współdzielą żadnego stanu i nie wymagają synchronizacji.

```c
void random_array(double* arr, size_t n_elems, double max) {
    #pragma omp parallel
    {
        unsigned short xsubi[3];
        getentropy(xsubi, sizeof(xsubi));

        #pragma omp for schedule(static)
        for (size_t i = 0; i < n_elems; i++) {
            arr[i] = erand48(xsubi) * max;
        }
    }
}
```

Wybór harmonogramowania (`schedule`) ma tu istotny wpływ na wydajność, ponieważ każda iteracja ma identyczny koszt - losowanie jednej liczby przez `erand48`. Przebadano pięć wariantów:

| Wariant | Opis |
|---|---|
| `static(N/T)` | Statyczne, chunk = $n/p$ (jawne, obliczone poza regionem) |
| `static(default)` | Statyczne, chunk domyślny (kompilator/runtime dzieli równomiernie) |
| `static(1)` | Statyczne, chunk = 1 (round-robin po iteracjach) |
| `dynamic(N/T)` | Dynamiczne, chunk = $n/p$ |
| `dynamic(default)` | Dynamiczne, chunk = 1 (domyślny dla `dynamic`) |

### Wyniki

Poniższy wykres słupkowy pokazuje czas wykonania wszystkich wariantów dla $n = 10\,000\,000$:

![scheduler summary bars](./plots/scheduler_summary_bars.png)

`dynamic(default)` jest znacząco gorszy od pozostałych we wszystkich konfiguracjach wątków - przy chunk = 1 każda z $10^7$ iteracji wymaga oddzielnego pobrania pracy od schedulera, co generuje ogromny narzut synchronizacji. Pozostałe cztery warianty są do siebie zbliżone.

Zależność czasu od liczby wątków dla różnych rozmiarów problemu:

![scheduler time by N](./plots/scheduler_time_by_N.png)

Dla małych $n$ (100–10k) wariant `static(N/T)` zachowuje się gorzej niż pozostałe statyczne - narzut związany z wywołaniem `omp_get_max_threads()` i obliczeniem chunk poza regionem równoległym przewyższa zysk z podziału tak małej pracy. Dla dużych $n$ (100k–10M) wszystkie warianty poza `dynamic(default)` skalują się poprawnie.

Przyśpieszenie w funkcji liczby wątków:

![scheduler speedup by N](./plots/scheduler_speedup_by_N.png)

Dla $n = 10\,000\,000$ warianty `static(default)`, `static(N/T)` i `dynamic(N/T)` osiągają przyspieszenie bliskie idealnemu. Wyniki dla małych $n$ są poniżej $1\times$ - koszt uruchomienia regionu równoległego przewyższa zysk z zrównoleglenia tak małej pracy.

Zależność czasu od rozmiaru problemu na skali logarytmicznej, dla każdej liczby wątków:

![scheduler time by threads](./plots/scheduler_time_by_threads.png)

We wszystkich konfiguracjach wątków czas rośnie liniowo z $n$ (prosta linia na skali log-log), co potwierdza oczekiwaną złożoność $O(n/p)$ na wątek. `dynamic(default)` leży konsekwentnie powyżej pozostałych o stały czynnik narzutu schedulera. Warto zauważyć interesujący `static(N/T)`. Jego duży narzut w przypadku threads 48 nie osiąga momentu, w którym główną składową czasu jest ilość elementów. 

Wykresy te dodatkowo bardzo dobrze pokazują wpływ narzutu na całkowity czas. Jak zostało zauważone wcześniej w momencie kiedy główną składową jest narzut wykres jest bliski funkcji $f(x) = N_z$ ($N_z$ w tym przypadku to wartość narzutu). Dopiero dla większych $N$ osiąga on kształt podobny do funkcji $f(x) = x$

### Wybór harmonogramowania

Na podstawie wyników w finalnej implementacji (`random_array` w `random.c`) zastosowano `schedule(static)` bez jawnego chunk - OpenMP dzieli iteracje równomiernie między wątki, co dla identycznego kosztu każdej iteracji jest optymalnym wyborem: brak narzutu dynamicznego schedulera i brak ryzyka false sharing przy małych chunkach.

### Prawo Amdahla

Prawo Amdahla mówi, że dla programu z sekwencyjną frakcją $f_s$ maksymalne przyspieszenie wynosi:

$$S(p) = \frac{1}{f_s + \frac{1-f_s}{p}} \xrightarrow{p \to \infty} \frac{1}{f_s}$$

W `random_array` jedyną sekwencyjną częścią jest inicjalizacja generatora przez `getentropy` (stały koszt, niezależny od $n$). Dla dużego $n$ frakcja ta zanika i $f_s \approx 0$ - przyspieszenie zbliża się do idealnego. Potwierdza to wariant `static(default)` dla $n = 10\,000\,000$:

| $p$ | przyspieszenie | idealne |
|---|---|---|
| 2 | 1.99× | 2.00× |
| 8 | 7.89× | 8.00× |
| 16 | 15.70× | 16.00× |
| 32 | 31.25× | 32.00× |
| 48 | 46.71× | 48.00× |

Prawo Amdahla jest tu dobrze zachowane. Wyniki poniżej $1\times$ dla małych $n$ nie są naruszeniem prawa - dla bardzo małej pracy narzut uruchomienia regionu równoległego (tworzenie wątków, synchronizacja) staje się dominujący. Amdahl modeluje przyspieszenie dla ustalonego $n$; dla $n \to 0$ przyspieszenie zawsze spada poniżej 1.

## 3. Poszczególne warianty implementacji algorytmu

Oba warianty dzielą wspólną infrastrukturę: strukturę danych kubełka, punkt wejścia programu oraz interfejs funkcji sortującej.

### Struktura `Bucket` i moduł `bucket.c`

Kubełek reprezentowany jest przez prostą strukturę:

```c
typedef struct {
    double* elems;    // wskaźnik na tablicę elementów
    size_t  n_elems;  // aktualna liczba elementów
    size_t  capacity; // maksymalna pojemność
} Bucket;
```

Moduł `bucket.c` udostępnia cztery operacje:

| Funkcja | Opis |
|---|---|
| `bucket_init(capacity)` | Alokuje kubełek o zadanej pojemności |
| `bucket_push(bucket, elem)` | Wstawia element; zwraca `PUSH_FAILED` gdy kubełek pełny |
| `create_buckets(n, capacity)` | Alokuje tablicę `n` kubełków **jednym `malloc`**: ciągły blok `[Bucket[0]…Bucket[n-1] \| elems[0]…elems[n-1]]`; każde `Bucket[i].elems` wskazuje odpowiedni fragment slaba |
| `destroy_buckets(buckets)` | Zwalnia cały blok jednym `free` (metadane i slab `elems` to jedna alokacja) |

Kubełki **nie są dynamicznie rozszerzane** - `bucket_push` po prostu zwraca błąd przy przepełnieniu. Dobór pojemności jest obowiązkiem wywołującego; pojemność startowa musi uwzględniać pesymistyczny rozkład danych.

### `main.c` - punkt wejścia i parametryzacja

Program przyjmuje dwa opcjonalne argumenty: `n_elems` (domyślnie $10^6$) i ścieżkę do pliku CSV z wynikami. Zakres wartości jest ustalony jako $[0, n)$ - każda liczba losowana jest przez `erand48(...) * n`, co przy $k = n$ kubełkach daje jednostajny rozkład z oczekiwaną jedną liczbą na kubełek.

```c
static size_t bucket_idx(double elem) {
    return (size_t)elem;  // obcięcie do części całkowitej
}
```

Globalne `buckets` (wspólne dla obu wariantów) alokowane są z pojemnością 10 - wystarczającą przy jednostajnym rozkładzie, gdzie oczekiwana liczba elementów na kubełek wynosi 1. Pomiar `t_total` obejmuje całe wywołanie `bucket_sort`, a poprawność wyniku weryfikowana jest sekwencyjną pętlą `is_sorted` poza pomiarem czasu. Wyniki (czasy faz, flaga poprawności) dopisywane są do CSV - program zawsze dopisuje wyłącznie wiersz danych, bez nagłówka. Nagłówek w plikach CSV pochodzi z zewnętrznego skryptu benchmarkowego.

### Wariant 2

#### Opis algorytmu

Wariant 2 zrównolegla fazę rozdziału poprzez ochronę każdego kubełka globalnego **oddzielnym lockiem**. Wątki wstawiają elementy bezpośrednio do wspólnych `buckets[]`, zabezpieczając każde wstawienie parą `omp_set_lock` / `omp_unset_lock`. Nie istnieje faza scalania ani prywatne kopie kubełków - narzut pamięciowy wynosi $O(k)$ locków, niezależnie od $p$.

Algorytm przebiega w czterech etapach:

1. **(a) Inicjalizacja locków** - $k$ locków alokowanych i inicjalizowanych równolegle w pętli `parallel for` poza mierzonym czasem.
2. **(b) Rozdział** - pętla `parallel for schedule(static)` po wszystkich $n$ elementach. Każdy element trafia do kubełka globalnego pod ochroną jego locka.
3. **(c) Sortowanie kubełków** - niezależne sortowanie każdego kubełka przez `qsort`. Brak kolizji.
4. **(d) Zapis** - wyznaczenie pozycji zapisu (prefix sum, sekwencyjnie), a następnie równoległe przepisanie posortowanych kubełków do tablicy wyjściowej blokami `memcpy`. Każdy kubełek trafia do rozłącznego fragmentu - brak kolizji.

#### Implementacja

Synchronizacja opiera się na tablicy `omp_lock_t locks[k]`, gdzie `locks[i]` chroni wyłącznie `buckets[i]`. Inicjalizacja i niszczenie locków odbywa się w osobnych pętlach `parallel for` poza mierzonym zakresem.

Faza rozdziału używa `#pragma omp parallel for schedule(static)`: iteracje $[0, n)$ dzielone są statycznie między wątki. Każda iteracja akwiruje lock dla docelowego kubełka, wstawia element, po czym go zwalnia:

```c
omp_set_lock(&locks[idx]);
bucket_push(&buckets[idx], array[i]);
omp_unset_lock(&locks[idx]);
```

Granularność blokady jest **per kubełek** - wątki piszące do różnych kubełków nigdy się nie blokują wzajemnie. Przy $k = n$ i rozkładzie jednostajnym każdy kubełek przyjmuje średnio jeden element, więc kolizje dwóch wątków na tym samym locku są rzadkie.

Flaga `sort_failed` aktualizowana jest przez `#pragma omp atomic write`, co jest wystarczające - zmiana jest wyłącznie jednostronna (`false` → `true`).

Fazy sortowania i zapisu są analogiczne do wariantu 3: `parallel for schedule(static)` po kubełkach oraz `memcpy` do rozłącznych fragmentów tablicy wyjściowej.

#### a) Ochrona danych współdzielonych

Wariant 2 chroni dane przez **blokowanie per kubełek** zamiast eliminacji współdzielenia.

Tablica wejściowa `array[]` jest czytana przez wiele wątków wyłącznie do odczytu - brak konfliktu. Każdy kubełek globalny `buckets[i]` chroniony jest przez `locks[i]`: wątek może pisać do `buckets[i]` tylko po nabyciu locka, co wyklucza równoczesny zapis dwóch wątków do tego samego kubełka. Fazy sortowania i zapisu nie wymagają synchronizacji - `parallel for` przydziela kubełki wątkom rozłącznie, a write-back zapisuje do rozłącznych fragmentów tablicy.

Wariant 2 nie wymaga żadnych jawnych barier między etapami - niejawna bariera kończąca region `parallel for` gwarantuje, że wszystkie elementy zostały wstawione przed rozpoczęciem sortowania.

#### b) Złożoność obliczeniowa i pamięciowa

| Etap | Span (ścieżka krytyczna) | Praca łączna | Pamięć dodatkowa |
|---|---|---|---|
| **(a) Init locków** | $O(k/p)$ | $O(k)$ | $O(k)$ - tablica `locks` |
| **(b) Rozdział** | $O(n/p)$ + contention | $O(n)$ | $O(1)$ |
| **(c) Sortowanie** | $O\!\left(\frac{n}{k}\log\frac{n}{k}\right)$ | $O\!\left(n\log\frac{n}{k}\right)$ | $O(1)$ na kubełek |
| **(d) Prefix sum** | $O(k)$ sekwencyjnie | $O(k)$ | $O(k)$ - tablica `write_at` |
| **(d) Zapis** | $O(n/p)$ | $O(n)$ | $O(1)$ |
| **Całość** | $O\!\left(\frac{k}{p} + \frac{n}{p} + \frac{n}{k}\log\frac{n}{k}\right)$ | $O\!\left(n\log\frac{n}{k}\right)$ | $O(k)$ |

Praca łączna wynosi $O\!\left(n\log\frac{n}{k}\right)$ - identycznie jak w sekwencyjnym sortowaniu kubełkowym. Algorytm jest **sekwencyjnie efektywny**.

Narzut pamięciowy jest stały i niezależny od $p$: dodatkowe alokacje to tablica $k$ locków i tablica `write_at` rozmiaru $k$ - łącznie $O(k)$. Przy $k = n = 10^6$ zajmują one kilkadziesiąt megabajtów, wobec $\approx 6{,}6\,\text{GB}$ dla wariantu 3 przy $p = 64$.

Oczekiwany span fazy rozdziału wynosi $O(n/p)$: przy rozkładzie jednostajnym i $k = n$ prawdopodobieństwo kolizji dwóch wątków na tym samym locku wynosi $1/k$, więc oczekiwany czas oczekiwania na locku jest pomijalny. W pesymistycznym przypadku (wszystkie elementy w jednym kubełku) rozdział degeneruje do sekwencyjnego $O(n)$.

### Wariant 3

#### Opis algorytmu

Wariant 3 eliminuje konflikty przy zapisie do kubełków poprzez przydzielenie każdemu wątkowi **prywatnej kopii wszystkich kubełków**. Wątek czyta wyłącznie swój fragment tablicy wejściowej i zapisuje wyłącznie do własnego wiersza kubełków - żaden inny wątek nie dotyka tego samego adresu pamięci. Kosztem jest dodatkowy etap scalania, w którym prywatne kubełki o tym samym zakresie wartości są łączone w jeden kubełek globalny.

Algorytm przebiega w czterech etapach:

1. **(b) Rozdział** - każdy wątek czyta swój fragment `array[start..end)` i wstawia elementy do własnych prywatnych kubełków (`thread_buckets[tid * n_buckets + idx]`). Brak kolizji - wątek `A` pisze wyłącznie do wiersza `A`.
2. **(b') Scalanie** - pętla równoległa po indeksach kubełków: wątek otrzymuje podzbiór kubełków globalnych i dla każdego z nich konkatenuje odpowiednie wiersze prywatne od wszystkich wątków. Brak kolizji - każdy wątek pisze do innego `buckets[b]`.
3. **(c) Sortowanie kubełków** - niezależne sortowanie każdego kubełka globalnego przez `qsort`. Brak kolizji.
4. **(d) Zapis** - wyznaczenie pozycji zapisu (prefix sum, sekwencyjnie), a następnie równoległe przepisanie posortowanych kubełków do tablicy wyjściowej blokami `memcpy`. Każdy kubełek trafia do rozłącznego fragmentu tablicy - brak kolizji.

#### Implementacja

Kluczową strukturą danych jest `thread_buckets` - tablica $p \cdot k$ obiektów `Bucket` zaalokowana jako ciągły blok pamięci w układzie *row-major*:

$$
\underbrace{[\,b_0\ |\ b_1\ |\ \cdots\ |\ b_{k-1}\,]}_{\text{wątek 0}}\ \underbrace{[\,b_0\ |\ b_1\ |\ \cdots\ |\ b_{k-1}\,]}_{\text{wątek 1}}\ \cdots\ \underbrace{[\,b_0\ |\ b_1\ |\ \cdots\ |\ b_{k-1}\,]}_{\text{wątek } p-1}
$$

Dostęp do prywatnego kubełka $b$ wątku $t$ odbywa się przez indeks `thread_buckets[t * n_buckets + b]`, co zapewnia brak aliasingu między wierszami i pozwala kompilatorowi/procesorowi na skuteczną prefetching.

Pojemność startowa $c = 10$ jest wystarczająca przy rozkładzie jednostajnym: oczekiwana liczba elementów w prywatnym kubełku jednego wątku wynosi $\frac{n}{p \cdot k}$, co dla $k = n$ daje $\frac{1}{p} \leq 1$, czyli prywatny kubełek przyjmuje średnio co najwyżej jeden element - pojemność $c = 10$ jest z dużym zapasem. Kubełki nie są dynamicznie rozszerzane - przepełnienie sygnalizowane jest przez `PUSH_FAILED` i propagowane przez flagę `sort_failed`.

Jedyną zmienną współdzieloną z zapisem jest flaga `sort_failed` sygnalizująca przepełnienie kubełka; jej aktualizacja odbywa się przez `#pragma omp atomic write`, co jest wystarczające (flaga przechodzi tylko z `false` na `true`).

`thread_buckets` alokowane jest przez `create_buckets(n_threads * n_buckets, c)` - jednym wywołaniem `malloc` zamiast wcześniejszych $p \cdot k + 1$ osobnych alokacji. Zwolnienie odbywa się przez `destroy_buckets(thread_buckets)` (bez parametru `n`). Zmiana ta eliminuje sekwencyjny narzut wielu wywołań alokatora przy dużych $p$ i $k$.

Etap rozdziału używa `#pragma omp parallel` (nie `parallel for`), ponieważ granice pętli zależą od `tid` obliczanego wewnątrz regionu. Etap scalania musi być **osobnym** regionem równoległym - niejawna bariera kończąca rozdział gwarantuje, że wszystkie wątki skończyły zapis do `thread_buckets` przed rozpoczęciem odczytu.

#### a) Ochrona danych współdzielonych

Wariant 3 jest zaprojektowany tak, by w żadnym etapie nie dochodziło do wyścigu o dane - zamiast synchronizować dostęp, eliminuje współdzielenie poprzez podział przestrzeni danych między wątki.

Tablica wejściowa `array[]` jest czytana wyłącznie przez właściciela fragmentu, a przy zapisie każdy wątek pisze do rozłącznego segmentu wyznaczonego przez prefix sum - nie ma potrzeby żadnej ochrony. Prywatne kubełki `thread_buckets` są z definicji wyłączną własnością wątku $t$ (wiersze w układzie *row-major*), więc zarówno zapis przy rozdziale, jak i odczyt przy scalaniu są wolne od konfliktów. Globalne `buckets[]` są w fazie scalania rozdzielone między wątki po indeksie kubełka: wątek piszący do `buckets[b]` jest jedynym, który dotyka tego kubełka - analogicznie przy sortowaniu i zapisie z powrotem.

Jedyną zmienną wymagającą synchronizacji jest flaga `sort_failed`, którą wiele wątków może jednocześnie próbować ustawić na `true` w razie przepełnienia kubełka. Zabezpieczona jest przez `#pragma omp atomic write`, co jest wystarczające, bo flaga zmienia się wyłącznie w jednym kierunku (`false` na `true`).

Wariant 3 nie używa żadnych locków. Poza `atomic write` na fladze błędu jedyną synchronizacją są niejawne bariery kończące regiony równoległe, które oddzielają etap rozdziału od scalania.

#### b) Złożoność obliczeniowa i pamięciowa

| Etap | Span (ścieżka krytyczna) | Praca łączna | Pamięć dodatkowa |
|---|---|---|---|
| **(b) Rozdział** | $O(n/p)$ | $O(n)$ | $O(p \cdot k \cdot c)$ - prywatne kubełki |
| **(b') Scalanie** | $O(k)$ | $O(p \cdot k)$ | $O(n)$ - kubełki globalne |
| **(c) Sortowanie** | $O\!\left(\frac{n}{k}\log\frac{n}{k}\right)$ | $O\!\left(n\log\frac{n}{k}\right)$ | $O(1)$ na kubełek (dzięki użyciu qsort *in-place*) |
| **(d) Prefix sum** | $O(k)$ sekwencyjnie | $O(k)$ | $O(k)$ - tablica `write_at` |
| **(d) Zapis** | $O(n/p)$ | $O(n)$ | $O(1)$ |
| **Całość** | $O\!\left(k + \frac{n}{k}\log\frac{n}{k}\right)$ | $O\!\left(p \cdot k + n\log\frac{n}{k}\right)$ | $O(n + p \cdot k \cdot c)$ |

Praca łączna wynosi $O\!\left(n \log \frac{n}{k}\right)$, co odpowiada złożoności sekwencyjnego sortowania kubełkowego - algorytm jest **sekwencyjnie efektywny**.

> **Uwaga dotycząca złożoności pamięciowej.**  
> Alokacja `thread_buckets` tworzy ciągły blok zawierający $p \cdot k$ obiektów `Bucket` oraz ich wspólny slab `elems`. Przy domyślnych parametrach testowych $n = k = 10^6$ i $p = 64$ (typowe dla Aresa) zaalokowana pamięć wynosi:
>
> $$p \cdot k \cdot \underbrace{24\,\text{B}}_{\texttt{sizeof(Bucket)}} + p \cdot k \cdot c \cdot \underbrace{8\,\text{B}}_{\texttt{sizeof(double)}} = 64 \cdot 10^6 \cdot (24 + 80)\,\text{B} \approx 6{,}6\,\text{GB}$$
>
> Dzięki pojedynczej alokacji narzut wywołań alokatora jest pomijalny niezależnie od $p$ i $k$ - wcześniejszy schemat wymagał $p \cdot k + 1$ osobnych wywołań `malloc`/`free`, co przy $p = 48$, $k = 10^6$ dawało $\approx 48 \cdot 10^6$ operacji. Wciąż warto jednak zbadać, czy tak duże jednorazowe zapotrzebowanie na pamięć nie stanie się wąskim gardłem - zarówno ze względu na ograniczenia RAM, jak i koszt inicjalizacji $p \cdot k$ elementów `Bucket`.

## 4. Uruchamianie na Aresie

Pomiary wykonano na klastrze **Ares** przy użyciu skryptów SLURM znajdujących się w katalogu `scripts/`. Każdy skrypt jest jednocześnie skryptem Bash i plikiem zadania SLURM - można go przesłać poleceniem:

```bash
sbatch scripts/run_bs2.sh
```

Wszystkie skrypty rezerwują **1 węzeł wyłącznie** (`--exclusive`), 1 zadanie z 48 rdzeniami (`--cpus-per-task=48`). Przed uruchomieniem pomiarów skrypt ładuje moduł `gcc` (jeśli dostępne jest polecenie `module`) i buduje pliki wykonywalne z pomocą dostarczonego `Makefile`. Wiązanie wątków do rdzeni ustawione jest przez zmienne środowiskowe `OMP_PLACES=cores` i `OMP_PROC_BIND=close`, co minimalizuje koszt migracji wątków między rdzeniami.

Wyniki dopisywane są do pliku CSV w katalogu `results/`. Nazwa pliku jest parametryzowana: gdy skrypt uruchamiany jest przez SLURM, do nazwy dodawany jest identyfikator zadania (np. `bs2_123456.csv`); lokalnie używana jest nazwa domyślna (np. `bs2.csv`).

### Skrypty benchmarkowe

#### `scripts/run_bs2.sh` i `scripts/run_bs3.sh`

Pełne benchmarki odpowiednio dla wariantu 2 i 3. Parametry przestrzeni pomiarowej:

| Parametr | Wartości |
|---|---|
| $n$ | 37 wartości: 100, 300, 500, 1 000, …, 1 000 000, 1 500 000, 2 000 000, 2 500 000 |
| $p$ | 1, 2, 3, 4, 6, 8, 10, 12, 16, 20, 24, 32, 48 |
| Limit czasu | 1 godz. 30 min (`--time=01:30:00`) |
| Partycja | `plgrid` |

Kombinacje z $p > n$ są pomijane (zbędna konfiguracja przy tak małej pracy). Każda poprawna kombinacja uruchamiana jest raz; wielokrotne uruchomienie skryptu dopisuje kolejne wiersze do tego samego CSV, co pozwala uśrednić wyniki po stronie skryptów rysujących.

#### `scripts/run_bs2_short.sh` i `scripts/run_bs3_short.sh`

Skrócone warianty do szybkiej weryfikacji poprawności i wstępnej oceny wydajności na partycji testowej:

| Parametr | Wartości |
|---|---|
| $n$ | 1 000, 10 000, 100 000, 1 000 000 |
| $p$ | 1, 4, 16, 48 |
| Limit czasu | 10 min (`--time=00:10:00`) |
| Partycja | `plgrid-testing` |

#### `scripts/random_tests.sh`

Benchmark do pomiaru wydajności generowania losowych liczb (`random_array`) dla różnych wariantów harmonogramowania. Parametry:

| Parametr | Wartości |
|---|---|
| $n$ | 100, 1 000, 10 000, 100 000, 1 000 000, 10 000 000 |
| $p$ | 1, 2, 8, 16, 32, 48 |
| Limit czasu | 1 godz. (`--time=01:00:00`) |
| Partycja | `plgrid` |

Skrypt buduje i uruchamia binarny plik `scheduler`, który testuje pięć wariantów harmonogramowania opisanych w sekcji 2.

## 5. Wyniki

Sekcja wyniki została podzielona na trzy części:
- skrypty wynikowe
- prezentacja wyników poszczególnych wariantów
- porównanie wariantów między sobą

### Skrypty wynikowe

#### Format CSV

Każde uruchomienie programu dopisuje jeden wiersz do pliku CSV (bez nagłówka):

| Kolumna | Typ | Opis |
|---|---|---|
| `algorithm` | string | nazwa wariantu (`bs2` lub `bs3`) |
| `n_threads` | int | liczba wątków OpenMP |
| `n_elems` | int | rozmiar tablicy wejściowej |
| `t_random` | float | czas losowania tablicy [s] |
| `t_distribute` | float | czas rozdziału elementów do kubełków [s] |
| `t_merge` | float | czas scalania prywatnych kubełków [s] (0 dla bs2) |
| `t_sort` | float | czas sortowania kubełków [s] |
| `t_writeback` | float | czas prefix-sum + przepisania do tablicy [s] |
| `t_total` | float | całkowity czas sortowania (mierzony niezależnie) [s] |
| `sorted` | int | weryfikacja poprawności (`1` = posortowane, `0` = błąd) |

Przykładowy wiersz:
```
bs2,8,1000000,0.012481,0.043210,0.000000,0.031540,0.005320,0.080100,1
```

#### `utils/bs_common.py`

Współdzielony moduł używany przez oba skrypty rysujące. Definiuje nazwy kolumn (`COLS`), listę mierzonych faz (`COMPONENTS`: Distribute, Merge, Sort, Write-back), funkcję `load()` wczytującą CSV do ramki pandas oraz `fmt_n()` formatującą rozmiary czytelnie (`1000000` → `1M`).

#### `utils/plot_heatmap.py`

Generuje mapy cieplne w układzie **liczba wątków × N** dla każdego wariantu:

```bash
python utils/plot_heatmap.py <plik.csv> [katalog_wyjściowy]
```

| Plik wyjściowy | Zawartość |
|---|---|
| `<algo>_total_heatmap.png` | heatmapa `t_total` |
| `<algo>_components_heatmap.png` | siatka 2×2 heatmap dla czterech faz |

#### `utils/plot_slice.py`

Generuje wykresy liniowe dla ustalonego N lub ustalonej liczby wątków:

```bash
python utils/plot_slice.py <plik.csv> <wariant> <N|threads> <wartość>
# np.: python utils/plot_slice.py results/results.csv bs2 N 1000000
```

| Plik wyjściowy | Zawartość |
|---|---|
| `<algo>_<typ><wartość>_total.png` | `t_total` w funkcji wolnej zmiennej |
| `<algo>_<typ><wartość>_components.png` | siatka 2×2 dla czterech faz |

#### `utils/plot_all_slices.sh`

Skrypt uruchamiający `plot_slice.py` dla wszystkich zdefiniowanych kombinacji N i liczby wątków, dla każdego pliku `bs2_*.csv` i `bs3_*.csv` w katalogu `results/`. Wykresy zapisywane są w podkatalogu nazwanym zgodnie z plikiem CSV.

```bash
./utils/plot_all_slices.sh [katalog_wyjściowy]
# domyślnie: results/plots/
```

### Prezentacja wyników poszczególnych wariantów

Pomiary wykonano dla $n \in [100,\ 2\,500\,000]$ (37 wartości) oraz $p \in \{1, 2, 3, 4, 6, 8, 10, 12, 16, 20, 24, 32, 48\}$. Każda kombinacja była uruchamiana wielokrotnie (co najmniej 2); wykresy przedstawiają wartości uśrednione.

#### Wariant 2

Mapa cieplna poniżej przedstawia `t_total` w całej przestrzeni parametrów $(n, p)$. Ciemnoniebieski kolor oznacza krótki czas wykonania.

![bs2 heatmap total](./plots/bs2_static/bs2_total_heatmap.png)

Widoczny jest wyraźny gradient: czas rośnie wzdłuż osi $n$ i maleje wzdłuż osi $p$. Cieplejsze kolory pojawiają się wyłącznie w lewej części (mała liczba wątków przy dużym $n$). Przy 24 i więcej wątkach cała przestrzeń pozostaje niebieska, co wskazuje na dobre wykorzystanie równoległości.

Rozkład czasu na fazy dla największego problemu ($n = 2\,500\,000$):

![bs2 components N=2.5M](./plots/bs2_static/bs2_N2500000_components.png)

Dominującą fazą jest **rozdział** - każdy element wymaga pobrania locka na kubełek. Mimo tego faza skaluje się dobrze: przy $k = n$ kubełki są rzadko odwiedzane przez dwa wątki jednocześnie, więc (jak wspomniane wyżej) lock contention jest ograniczone. Fazy sortowania i zapisu skalują się niemal idealnie, operując na rozłącznych fragmentach danych.

Zależność `t_total` od $n$ przy $p = 48$ jest niemal liniowa:

![bs2 threads=1 total](./plots/bs2_static/bs2_threads1_total.png)
![bs2 threads=48 total](./plots/bs2_static/bs2_threads48_total.png)

Czas przy $p = 24$ i $n = 2{,}5\text{M}$ wynosi $0{,}03\,\text{s}$, a przy $p = 48$ - $0{,}032\,\text{s}$ - algorytm zachowuje oczekiwaną złożoność $O(n)$.

Heatmapa komponentów potwierdza dominację fazy rozdziału oraz zanikanie fazy sortowania przy rosnącym $p$:

![bs2 heatmap components](./plots/bs2_static/bs2_components_heatmap.png)

Przyspieszenie `t_total` względem $p = 1$ dla $n = 2\,500\,000$:

| $p$ | $t_\text{total}$ [s] | przyspieszenie (około) |
|---|---|---|
| 1 | 0.345 | 1.0× |
| 4 | 0.104 | 3.3× |
| 8 | 0.059 | 5.9× |
| 12 | 0.044 | 7.8× |
| 24 | 0.030 | 10.5× |
| 32 | 0.034 | 10.2× |
| 48 | 0.032 | 10.8× |

Przyspieszenie rośnie mocno aż do $p = 24$ ($10{,}5\times$), potem się spłaszcza (w zależności od obłożenia na aresie odnotowano wahania między $0.035$ a $0.03$ dla $p \ge 24$).

**Prawo Amdahla.** Dopasowanie do danych dla $p \leq 24$ daje sekwencyjną frakcję $f_s \approx 5{,}4\%$, co wyznacza teoretyczny sufit $S_{\max} = 1/f_s \approx 18{,}6\times$. Źródłem tej frakcji jest sekwencyjny prefix sum iterujący przez $k = n = 2{,}5 \cdot 10^6$ kubełków oraz stałe narzuty synchronizacji wątków.

Przy $p = 32$ przyspieszenie chwilowo spada ($10{,}20\times$), po czym przy $p = 48$ rośnie do $10{,}84\times$. Nie jest to klasyczne plateau przewidziane przez Amdahla - nieregularność wynika z konkurencji o locki w fazie rozdziału i wahań NUMA; przy $k = n$ kolizje są jednak rzadkie, więc efekt jest umiarkowany.

---

#### Wariant 3

Mapa cieplna `t_total` dla wariantu 3 wygląda zupełnie inaczej niż dla wariantu 2:

![bs3 heatmap total](./plots/bs3_static/bs3_total_heatmap.png)

Gradient jest **odwrócony**: czas rośnie wraz z $p$. Ciepłe kolory pojawiają się w prawej części mapy (wiele wątków), a przy $p = 1$ algorytm działa szybciej niż przy $p = 48$. Jest to jednoznaczny sygnał anty-skalowania.

Widok komponentów dla $n = 2\,500\,000$ pokazuje źródło problemu:

![bs3 components N=2.5M](./plots/bs3_static/bs3_N2500000_components.png)

Fazy **Distribute** i **Sort** skalują się poprawnie - obie maleją z $p$ *(W distribute pojawiły się jakieś szumy, nie jesteśmy w stanie w 100% ustalić o co chodzi, najlepsza nasza teoria to overhead `dynamic`)*. Faza **Write-back** skaluje się dobrze - maleje z $\approx 0{,}036\,\text{s}$ przy $p = 1$ do $\approx 0{,}015\,\text{s}$ przy $p = 48$ (dzięki ciągłemu slab `elems` `memcpy` ma teraz liniowy dostęp do pamięci). Nowym wąskim gardłem jest faza **Merge**: skacze z $0{,}101\,\text{s}$ przy $p = 1$ do $\approx 0{,}21\,\text{s}$ przy $p = 2$ i pozostaje płaska dla wszystkich większych $p$ ($0{,}17$–$0{,}23\,\text{s}$). Najlepszą teorią "dlaczego" jest wystąpienie `cache-missów`. Dla każdego globalnego kubełka $b$ wątek musi odczytać dane z $p$ oddzielnych wierszy `thread_buckets`, co przy dużym $k$ generuje $O(p \cdot k)$ chybień pamięci podręcznej - łączna praca merga rośnie liniowo z $p$, mimo że każdy element jest kopiowany dokładnie raz.

Heatmapa komponentów ilustruje ten wzorzec na całej przestrzeni parametrów:

![bs3 heatmap components](./plots/bs3_static/bs3_components_heatmap.png)

`t_total` jest nadal większe niż suma zmierzonych faz - różnica rośnie liniowo z $p$. Porównanie dla $n = 2{,}5\text{M}$:

| | $p = 1$ | $p = 48$ |
|---|---|---|
| Suma faz [s] | 0.300 | 0.330 |
| `t_total` [s] | 0.315 | 1.000 |
| Niezliczona różnica [s] | 0.014 | 0.671 |

![bs3 threads=1 total](./plots/bs3_static/bs3_threads1_total.png)
![bs3 threads=48 total](./plots/bs3_static/bs3_threads48_total.png)

Przy $p = 1$ różnica jest pomijalna ($0{,}014\,\text{s}$). Przy $p = 48$ brakujące $0{,}67\,\text{s}$ to koszt sekwencyjnej inicjalizacji `thread_buckets` w `create_buckets`: pętla po $p \cdot k = 48 \cdot 2{,}5 \cdot 10^6 = 120 \cdot 10^6$ obiektów `Bucket` dotyka ~$2{,}9\,\text{GB}$ metadanych oraz wywołuje *page faults* na slabie ($\approx 9{,}6\,\text{GB}$). Różnica ta rośnie w przybliżeniu liniowo z $p$ ($\approx 0{,}014\,\text{s}$ na wątek), co potwierdza liniową zależność kosztu inicjalizacji od $p \cdot k$.

**Prawo Amdahla.** Wariant 3 narusza podstawowe założenie prawa Amdahla: że sekwencyjna frakcja $f_s$ jest stała i niezależna od $p$. Mamy dwa efekty o odwrotnej charakterystyce:

- Koszt inicjalizacji `thread_buckets` wynosi $O(p \cdot k)$ - rośnie *liniowo* z $p$. Im więcej wątków, tym więcej pracy sekwencyjnej poza mierzonym regionem.
- Faza **Merge** ma złożoność pracy $O(p \cdot k)$: $p$ równoległych wątków wykonuje każdy $O(k)$ iteracji, ale każda iteracja generuje $p$ chybień pamięci podręcznej - efektywnie czas merga jest stały lub rośnie z $p$.

W efekcie efektywna sekwencyjna frakcja $f_s(p)$ jest funkcją rosnącą, a nie stałą - model Amdahla w ogóle nie stosuje się do opisu tego zachowania. Odwrócony gradient na heatmapie jest tego bezpośrednim dowodem: zamiast $S(p) \to 1/f_s$, mamy $S(p) \to 0$ przy $p \to \infty$.

### Porównanie wariantów między sobą

Porównanie `t_total` dla $n = 2\,500\,000$:

| $p$ | bs2 [s] | bs3 [s] | bs2 / bs3 |
|---|---|---|---|
| 1 | 0.345 | 0.315 | 1.10× |
| 4 | 0.104 | 0.358 | 0.29× |
| 8 | 0.059 | 0.393 | 0.15× |
| 12 | 0.044 | 0.419 | 0.10× |
| 24 | 0.033 | 0.607 | 0.05× |
| 48 | 0.032 | 1.000 | 0.03× |

Przy $p = 1$ oba warianty są zbliżone: bs3 jest nieznacznie szybszy ($0{,}315\,\text{s}$ vs $0{,}345\,\text{s}$), ponieważ eliminuje koszt locków w fazie rozdziału. Dla każdego $p > 1$ bs2 jest szybszy i przewaga rośnie z liczbą wątków - przy $p = 48$ bs2 jest ponad $30\times$ szybsze.

Główne przyczyny przewagi bs2 przy wielu wątkach:

- **Brak fazy scalania.** bs2 wstawia bezpośrednio do kubełków globalnych; bs3 musi po rozdziale wykonać mergę o koszcie $O(p \cdot k)$, która nie skaluje się z $p$.
- **Brak kosztu inicjalizacji `thread_buckets`.** bs2 nie alokuje prywatnych kubełków; bs3 inicjalizuje sekwencyjnie $p \cdot k$ struktur, co przy $p = 48$, $k = 2{,}5 \cdot 10^6$ pochłania $\approx 0{,}67\,\text{s}$ poza mierzonym zakresem.
- **Lock contention w bs2 jest ograniczone.** Przy $k = n$ każdy kubełek przyjmuje średnio jeden element, więc dwa wątki rzadko rywalizują o ten sam lock.

Wariant 3 mógłby być konkurencyjny przy znacznie mniejszym $k$ (mało kubełków, wiele elementów na kubełek), gdzie lock contention w bs2 byłoby poważnym problemem i koszt $O(p \cdot k)$ merga byłby pomijalny wobec rozdziału. Tego natomiast to sprawozdanie nie badało, gdyż badane były warunki perfekcyjne.

