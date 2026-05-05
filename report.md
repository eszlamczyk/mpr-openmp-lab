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

Celem tego sprawozdania jest przedstawienie wykorzystania `OpenMP` - bazowanego na dyrektywach API stworzeonego do tworzenia równoległych programów na współdzielonych architekturach pamięciowych. W tym sprawozdaniu będzie badany wpływ różnych funkcji `OpenMP` na performance poszczególnych części algorytmu **sortowania kubełkowego** (*ang. bucket sort*). 

W całym sprawozdaniu używamy następujących oznaczeń:

- $n$ — liczba elementów tablicy wejściowej,
- $k$ — liczba kubełków (w implementacji `n_buckets`; przy domyślnych parametrach $k = n$),
- $p$ — liczba wątków OpenMP (w implementacji `n_threads = omp_get_max_threads()`),
- $c$ — pojemność startowa pojedynczego prywatnego kubełka (w implementacji $c = 10$).

### Sortowanie kubełkowe

Sortowanie kubełkowe (*ang. bucket sort*) jest algorytmem sortowania działającym w czasie liniowo-oczekiwanym dla danych o jednostajnym rozkładzie. Dane wejściowe dzielone są na $k$ kubełków, z których każdy odpowiada pewnemu przedziałowi wartości. Algorytm składa się z czterech faz:

1. **(b) Rozdział** — każdy element tablicy trafia do kubełka odpowiadającego jego wartości.
2. **(c) Sortowanie kubełków** — każdy kubełek sortowany jest niezależnie dowolnym algorytmem porównawczym (np. quicksort).
3. **(d) Wyznaczenie pozycji zapisu** — dla każdego kubełka obliczana jest pozycja startowa w tablicy wynikowej (prefix sum rozmiarów kubełków).
4. **(d) Zapis** — posortowane kubełki przepisywane są kolejno do tablicy wyjściowej.

**Złożoność** przy $k$ kubełkach i $n$ elementach:

| Przypadek | Złożoność |
|---|---|
| Oczekiwana (rozkład jednostajny, $k = n$) | $O(n)$ |
| Pesymistyczna (wszystkie elementy w jednym kubełku) | $O(n \log n)$ |

**Potencjalne miejsca zrównoleglenia:**

| Faza | Niezależność iteracji | Możliwość zrównoleglenia | Uwaga |
|---|---|---|---|
| (b) Rozdział | Nie — wiele wątków może pisać do tego samego kubełka | Tak, ale wymaga synchronizacji | Konieczne blokady lub eliminacja współdzielenia |
| (c) Sortowanie kubełków | Tak — kubełki są od siebie niezależne | Tak | Warto zastosować harmonogramowanie dynamiczne ze względu na nierówne rozmiary kubełków |
| (d) Prefix sum | Nie — każdy termin zależy od poprzedniego | Zasadniczo sekwencyjne | $O(k)$ — koszt pomijalny wobec pozostałych faz |
| (d) Zapis | Tak — każdy kubełek trafia do rozłącznego fragmentu tablicy | Tak | Brak wyścigu przy zapisie |

Wąskim gardłem jest **faza rozdziału** — jedyna wymagająca synchronizacji przy równoległym wykonaniu. Dalsze sekcje opisują dwie strategie jej zrównoleglenia: ochronę kubełków blokadami (wariant 2) oraz eliminację współdzielenia przez prywatne kubełki per wątek (wariant 3).

## 2. Tworzenie losowych liczb

### Implementacja

Tablica wejściowa wypełniana jest liczbami pseudolosowymi z przedziału $[0, n)$ przy użyciu funkcji `erand48`. Każdy wątek inicjalizuje własny, niezależny stan generatora (`xsubi[3]`) entropią systemową pobraną przez `getentropy` — dzięki temu wątki nie współdzielą żadnego stanu i nie wymagają synchronizacji.

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

Wybór harmonogramowania (`schedule`) ma tu istotny wpływ na wydajność, ponieważ każda iteracja ma identyczny koszt — losowanie jednej liczby przez `erand48`. Przebadano pięć wariantów:

| Wariant | Opis |
|---|---|
| `static(N/T)` | Statyczne, chunk = $n/p$ (jawne, obliczone poza regionem) |
| `static(default)` | Statyczne, chunk domyślny (kompilator/runtime dzieli równomiernie) |
| `static(1)` | Statyczne, chunk = 1 (round-robin po iteracjach) |
| `dynamic(N/T)` | Dynamiczne, chunk = $n/p$ |
| `dynamic(default)` | Dynamiczne, chunk = 1 (domyślny dla `dynamic`) |

### Wyniki

Poniższy wykres słupkowy pokazuje czas wykonania wszystkich wariantów dla $n = 10\,000\,000$:

![scheduler summary bars](../plots/scheduler_summary_bars.png)

`dynamic(default)` jest znacząco gorszy od pozostałych we wszystkich konfiguracjach wątków — przy chunk = 1 każda z $10^7$ iteracji wymaga oddzielnego pobrania pracy od schedulera, co generuje ogromny narzut synchronizacji. Pozostałe cztery warianty są do siebie zbliżone.

Zależność czasu od liczby wątków dla różnych rozmiarów problemu:

![scheduler time by N](../plots/scheduler_time_by_N.png)

Dla małych $n$ (100–10k) wariant `static(N/T)` zachowuje się gorzej niż pozostałe statyczne — narzut związany z wywołaniem `omp_get_max_threads()` i obliczeniem chunk poza regionem równoległym przewyższa zysk z podziału tak małej pracy. Dla dużych $n$ (100k–10M) wszystkie warianty poza `dynamic(default)` skalują się poprawnie.

Przyśpieszenie w funkcji liczby wątków:

![scheduler speedup by N](../plots/scheduler_speedup_by_N.png)

Dla $n = 10\,000\,000$ warianty `static(default)`, `static(1)` i `dynamic(N/T)` osiągają przyspieszenie bliskie idealnemu. Wyniki dla małych $n$ są poniżej $1\times$ — koszt uruchomienia regionu równoległego przewyższa zysk z zrównoleglenia tak małej pracy.

Zależność czasu od rozmiaru problemu na skali logarytmicznej, dla każdej liczby wątków:

![scheduler time by threads](../plots/scheduler_time_by_threads.png)

We wszystkich konfiguracjach wątków czas rośnie liniowo z $n$ (prosta linia na skali log-log), co potwierdza oczekiwaną złożoność $O(n/p)$ na wątek. `dynamic(default)` leży konsekwentnie powyżej pozostałych o stały czynnik narzutu schedulera.

### Wybór harmonogramowania

Na podstawie wyników w finalnej implementacji (`random_array` w `random.c`) zastosowano `schedule(static)` bez jawnego chunk — OpenMP dzieli iteracje równomiernie między wątki, co dla identycznego kosztu każdej iteracji jest optymalnym wyborem: brak narzutu dynamicznego schedulera i brak ryzyka false sharing przy małych chunkach.

### Prawo Amdahla

Prawo Amdahla mówi, że dla programu z sekwencyjną frakcją $f_s$ maksymalne przyspieszenie wynosi:

$$S(p) = \frac{1}{f_s + \frac{1-f_s}{p}} \xrightarrow{p \to \infty} \frac{1}{f_s}$$

W `random_array` jedyną sekwencyjną częścią jest inicjalizacja generatora przez `getentropy` (stały koszt, niezależny od $n$). Dla dużego $n$ frakcja ta zanika i $f_s \approx 0$ — przyspieszenie zbliża się do idealnego. Potwierdza to wariant `static(default)` dla $n = 10\,000\,000$:

| $p$ | przyspieszenie | idealne |
|---|---|---|
| 2 | 1.99× | 2.00× |
| 8 | 7.89× | 8.00× |
| 16 | 15.70× | 16.00× |
| 32 | 31.25× | 32.00× |
| 48 | 46.71× | 48.00× |

Prawo Amdahla jest tu dobrze zachowane. Wyniki poniżej $1\times$ dla małych $n$ nie są naruszeniem prawa — dla bardzo małej pracy narzut uruchomienia regionu równoległego (tworzenie wątków, synchronizacja) staje się dominujący. Amdahl modeluje przyspieszenie dla ustalonego $n$; dla $n \to 0$ przyspieszenie zawsze spada poniżej 1.

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
| `create_buckets(n, capacity)` | Alokuje tablicę `n` kubełków jednym `malloc` na metadane + `n` osobnych `malloc` na `elems` |
| `destroy_buckets(buckets, n)` | Zwalnia wszystkie `elems` i tablicę metadanych |

Kubełki **nie są dynamicznie rozszerzane** — `bucket_push` po prostu zwraca błąd przy przepełnieniu. Dobór pojemności jest obowiązkiem wywołującego; pojemność startowa musi uwzględniać pesymistyczny rozkład danych.

### `main.c` — punkt wejścia i parametryzacja

Program przyjmuje dwa opcjonalne argumenty: `n_elems` (domyślnie $10^6$) i ścieżkę do pliku CSV z wynikami. Zakres wartości jest ustalony jako $[0, n)$ — każda liczba losowana jest przez `erand48(...) * n`, co przy $k = n$ kubełkach daje jednostajny rozkład z oczekiwaną jedną liczbą na kubełek.

```c
static size_t bucket_idx(double elem) {
    return (size_t)elem;  // obcięcie do części całkowitej
}
```

Globalne `buckets` (wspólne dla obu wariantów) alokowane są z pojemnością 10 — wystarczającą przy jednostajnym rozkładzie, gdzie oczekiwana liczba elementów na kubełek wynosi 1. Pomiar `t_total` obejmuje całe wywołanie `bucket_sort`, a poprawność wyniku weryfikowana jest sekwencyjną pętlą `is_sorted` poza pomiarem czasu. Wyniki (czasy faz, flaga poprawności) dopisywane są do CSV bez nagłówka jeśli plik już istnieje.

### Wariant 2

// todo - jbrs

### Wariant 3

#### Opis algorytmu

Wariant 3 eliminuje konflikty przy zapisie do kubełków poprzez przydzielenie każdemu wątkowi **prywatnej kopii wszystkich kubełków**. Wątek czyta wyłącznie swój fragment tablicy wejściowej i zapisuje wyłącznie do własnego wiersza kubełków — żaden inny wątek nie dotyka tego samego adresu pamięci. Kosztem jest dodatkowy etap scalania, w którym prywatne kubełki o tym samym zakresie wartości są łączone w jeden kubełek globalny.

Algorytm przebiega w czterech etapach:

1. **(b) Rozdział** — każdy wątek czyta swój fragment `array[start..end)` i wstawia elementy do własnych prywatnych kubełków (`thread_buckets[tid * n_buckets + idx]`). Brak kolizji — wątek `A` pisze wyłącznie do wiersza `A`.
2. **(b') Scalanie** — pętla równoległa po indeksach kubełków: wątek otrzymuje podzbiór kubełków globalnych i dla każdego z nich konkatenuje odpowiednie wiersze prywatne od wszystkich wątków. Brak kolizji — każdy wątek pisze do innego `buckets[b]`.
3. **(c) Sortowanie kubełków** — niezależne sortowanie każdego kubełka globalnego przez `qsort`. Brak kolizji.
4. **(d) Zapis** — wyznaczenie pozycji zapisu (prefix sum, sekwencyjnie), a następnie równoległe przepisanie posortowanych kubełków do tablicy wyjściowej blokami `memcpy`. Każdy kubełek trafia do rozłącznego fragmentu tablicy — brak kolizji.

#### Implementacja

Kluczową strukturą danych jest `thread_buckets` — tablica $p \cdot k$ obiektów `Bucket` zaalokowana jako ciągły blok pamięci w układzie *row-major*:

$$
\underbrace{[\,b_0\ |\ b_1\ |\ \cdots\ |\ b_{k-1}\,]}_{\text{wątek 0}}\ \underbrace{[\,b_0\ |\ b_1\ |\ \cdots\ |\ b_{k-1}\,]}_{\text{wątek 1}}\ \cdots\ \underbrace{[\,b_0\ |\ b_1\ |\ \cdots\ |\ b_{k-1}\,]}_{\text{wątek } p-1}
$$

Dostęp do prywatnego kubełka $b$ wątku $t$ odbywa się przez indeks `thread_buckets[t * n_buckets + b]`, co zapewnia brak aliasingu między wierszami i pozwala kompilatorowi/procesorowi na skuteczną prefetching.

Pojemność startowa $c = 10$ jest wystarczająca przy rozkładzie jednostajnym: oczekiwana liczba elementów w prywatnym kubełku jednego wątku wynosi $\frac{n}{p \cdot k}$, co dla $k = n$ daje $\frac{1}{p} \leq 1$, czyli prywatny kubełek przyjmuje średnio co najwyżej jeden element — pojemność $c = 10$ jest z dużym zapasem. Kubełki nie są dynamicznie rozszerzane — przepełnienie sygnalizowane jest przez `PUSH_FAILED` i propagowane przez flagę `sort_failed`.

Jedyną zmienną współdzieloną z zapisem jest flaga `sort_failed` sygnalizująca przepełnienie kubełka; jej aktualizacja odbywa się przez `#pragma omp atomic write`, co jest wystarczające (flaga przechodzi tylko z `false` na `true`).

Etap rozdziału używa `#pragma omp parallel` (nie `parallel for`), ponieważ granice pętli zależą od `tid` obliczanego wewnątrz regionu. Etap scalania musi być **osobnym** regionem równoległym — niejawna bariera kończąca rozdział gwarantuje, że wszystkie wątki skończyły zapis do `thread_buckets` przed rozpoczęciem odczytu.

#### a) Ochrona danych współdzielonych

Wariant 3 jest zaprojektowany tak, by w żadnym etapie nie dochodziło do wyścigu o dane — zamiast synchronizować dostęp, eliminuje współdzielenie poprzez podział przestrzeni danych między wątki.

Tablica wejściowa `array[]` jest czytana wyłącznie przez właściciela fragmentu, a przy zapisie każdy wątek pisze do rozłącznego segmentu wyznaczonego przez prefix sum — nie ma potrzeby żadnej ochrony. Prywatne kubełki `thread_buckets` są z definicji wyłączną własnością wątku $t$ (wiersze w układzie *row-major*), więc zarówno zapis przy rozdziale, jak i odczyt przy scalaniu są wolne od konfliktów. Globalne `buckets[]` są w fazie scalania rozdzielone między wątki po indeksie kubełka: wątek piszący do `buckets[b]` jest jedynym, który dotyka tego kubełka — analogicznie przy sortowaniu i zapisie z powrotem.

Jedyną zmienną wymagającą synchronizacji jest flaga `sort_failed`, którą wiele wątków może jednocześnie próbować ustawić na `true` w razie przepełnienia kubełka. Zabezpieczona jest przez `#pragma omp atomic write`, co jest wystarczające, bo flaga zmienia się wyłącznie w jednym kierunku (`false` na `true`).

Wariant 3 nie używa żadnych locków. Poza `atomic write` na fladze błędu jedyną synchronizacją są niejawne bariery kończące regiony równoległe, które oddzielają etap rozdziału od scalania.

#### b) Złożoność obliczeniowa i pamięciowa

| Etap | Span (ścieżka krytyczna) | Praca łączna | Pamięć dodatkowa |
|---|---|---|---|
| **(b) Rozdział** | $O(n/p)$ | $O(n)$ | $O(p \cdot k \cdot c)$ — prywatne kubełki |
| **(b') Scalanie** | $O(n/p)$ | $O(n)$ | $O(n)$ — kubełki globalne |
| **(c) Sortowanie** | $O\!\left(\frac{n}{k}\log\frac{n}{k}\right)$ | $O\!\left(n\log\frac{n}{k}\right)$ | $O(1)$ na kubełek (dzięki użyciu qsort *in-place*) |
| **(d) Prefix sum** | $O(k)$ sekwencyjnie | $O(k)$ | $O(k)$ — tablica `write_at` |
| **(d) Zapis** | $O(n/p)$ | $O(n)$ | $O(1)$ |
| **Całość** | $O\!\left(\frac{n}{p} + \frac{n}{k}\log\frac{n}{k}\right)$ | $O\!\left(n\log\frac{n}{k}\right)$ | $O(n + p \cdot k \cdot c)$ |

Praca łączna wynosi $O\!\left(n \log \frac{n}{k}\right)$, co odpowiada złożoności sekwencyjnego sortowania kubełkowego — algorytm jest **sekwencyjnie efektywny**.

> **Uwaga dotycząca złożoności pamięciowej.**  
> Alokacja `thread_buckets` tworzy $p \cdot k$ obiektów `Bucket`, z których każdy posiada własną tablicę `elems` o pojemności $c$. Przy domyślnych parametrach testowych $n = k = 10^6$ i $p = 64$ (typowe dla Aresa) zaalokowana pamięć wynosi:
>
> $$p \cdot k \cdot \underbrace{24\,\text{B}}_{\texttt{sizeof(Bucket)}} + p \cdot k \cdot c \cdot \underbrace{8\,\text{B}}_{\texttt{sizeof(double)}} = 64 \cdot 10^6 \cdot (24 + 80)\,\text{B} \approx 6{,}6\,\text{GB}$$
>
> Warto zbadać, czy tak duże jednorazowe zapotrzebowanie na pamięć nie stanie się wąskim gardłem przy dużej liczbie wątków — zarówno ze względu na ograniczenia dostępnej RAM, jak i czas samej alokacji i inicjalizacji tej struktury, który może być nietrywialny w stosunku do czasu właściwego rozdziału.

## 4. Uruchamianie na Aresie

// todo - opisać odpalanie na aresie + skrypty

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

Otoczka skryptowa uruchamiająca `plot_slice.py` dla wszystkich kombinacji N i liczby wątków zdefiniowanych w skrypcie, dla każdego pliku `bs2_*.csv` i `bs3_*.csv` w katalogu `results/`. Wykresy zapisywane są w podkatalogu nazwanym zgodnie z plikiem CSV.

```bash
./utils/plot_all_slices.sh [katalog_wyjściowy]
# domyślnie: results/plots/
```

### Prezentacja wyników poszczególnych wariantów

Pomiary wykonano dla $n \in [100,\ 2\,500\,000]$ (37 wartości) oraz $p \in \{1, 2, 3, 4, 6, 8, 10, 12, 16, 20, 24, 32, 48\}$. Każda kombinacja była uruchamiana wielokrotnie; wykresy przedstawiają wartości uśrednione.

#### Wariant 2

Mapa cieplna poniżej przedstawia `t_total` w całej przestrzeni parametrów $(n, p)$. Ciemnoniebieski kolor oznacza krótki czas wykonania.

![bs2 heatmap total](../plots/bs2_static/bs2_total_heatmap.png)

Widoczny jest wyraźny gradient: czas rośnie wzdłuż osi $n$ i maleje wzdłuż osi $p$. Cieplejsze kolory pojawiają się wyłącznie w lewej części (mała liczba wątków przy dużym $n$). Przy 24 i więcej wątkach cała przestrzeń pozostaje niebieska, co wskazuje na dobre wykorzystanie równoległości.

Rozkład czasu na fazy dla największego problemu ($n = 2\,500\,000$):

![bs2 components N=2.5M](../plots/bs2_static/bs2_N2500000_components.png)

Dominującą fazą jest **rozdział** — każdy element wymaga pobrania locka na kubełek. Mimo tego faza skaluje się dobrze: przy $k = n$ kubełki są rzadko odwiedzane przez dwa wątki jednocześnie, więc lock contention jest ograniczone. Fazy sortowania i zapisu skalują się niemal idealnie, operując na rozłącznych fragmentach danych.

Zależność `t_total` od $n$ przy optymalnej liczbie wątków ($p = 24$) jest niemal liniowa:

![bs2 threads=24 total](../plots/bs2_static/bs2_threads24_total.png)

Czas przy $p = 24$ i $n = 2{,}5\text{M}$ wynosi $0{,}036\,\text{s}$ — algorytm zachowuje oczekiwaną złożoność $O(n)$.

Heatmapa komponentów potwierdza dominację fazy rozdziału oraz zanikanie fazy sortowania przy rosnącym $p$:

![bs2 heatmap components](../plots/bs2_static/bs2_components_heatmap.png)

Przyspieszenie `t_total` względem $p = 1$ dla $n = 2\,500\,000$:

| $p$ | $t_\text{total}$ [s] | przyspieszenie |
|---|---|---|
| 1 | 0.347 | 1.00× |
| 4 | 0.102 | 3.42× |
| 8 | 0.060 | 5.78× |
| 12 | 0.046 | 7.49× |
| 24 | 0.036 | 9.63× |
| 32 | 0.041 | 8.43× |
| 48 | 0.042 | 8.18× |

Przyspieszenie osiąga maksimum $9{,}6\times$ przy $p = 24$, po czym spada.

**Prawo Amdahla.** Dopasowanie do danych dla $p \leq 24$ daje sekwencyjną frakcję $f_s \approx 6{,}5\%$, co wyznacza teoretyczny sufit $S_{\max} = 1/f_s \approx 15{,}4\times$. Źródłem tej frakcji jest sekwencyjny prefix sum iterujący przez $k = n = 2{,}5 \cdot 10^6$ kubełków oraz stałe narzuty synchronizacji wątków.

Jednak przy $p = 32$ i $p = 48$ przyspieszenie spada poniżej wartości z $p = 24$, co jest już naruszeniem modelu Amdahla — prawo przewiduje monotonicznie rosnące (choć nasycające się) przyspieszenie. Przyczyną są dwa efekty nieobecne w modelu: **lock contention** w fazie rozdziału (więcej wątków rywalizuje o te same kubełki) oraz **nasycenie przepustowości pamięci** (równoległe zapisy do tablicy wejściowej przy zdefragmentowanych kubełkach). Oba efekty rosną z $p$, a nie są stałe — przez co rzeczywiste przyspieszenie cofa się zamiast jedynie plateau.

---

#### Wariant 3

Mapa cieplna `t_total` dla wariantu 3 wygląda zupełnie inaczej niż dla wariantu 2:

![bs3 heatmap total](../plots/bs3_static/bs3_total_heatmap.png)

Gradient jest **odwrócony**: czas rośnie wraz z $p$. Ciepłe kolory pojawiają się w prawej części mapy (wiele wątków), a przy $p = 1$ algorytm działa szybciej niż przy $p = 48$. Jest to jednoznaczny sygnał anty-skalowania.

Widok komponentów dla $n = 2\,500\,000$ pokazuje źródło problemu:

![bs3 components N=2.5M](../plots/bs3_static/bs3_N2500000_components.png)

Fazy **Distribute** i **Sort** skalują się poprawnie — obie maleją z $p$. Faza **Merge** jest płaska (zdominowana przez stały koszt sekwencyjny), natomiast **Write-back** rośnie liniowo z $p$: od $\approx 0{,}013\,\text{s}$ przy $p = 1$ do $\approx 1{,}75\,\text{s}$ przy $p = 48$. Jest to konsekwencja rozproszenia globalne `buckets[]` w pamięci — każdy kubełek ma osobno zaalokowaną tablicę `elems`, co przy wielu wątkach wykonujących `memcpy` jednocześnie nasila cache miss'y i nasyca szyny pamięciowe.

Heatmapa komponentów ilustruje ten wzorzec na całej przestrzeni parametrów:

![bs3 heatmap components](../plots/bs3_static/bs3_components_heatmap.png)

Poza wzrostem `t_writeback`, `t_total` jest znacząco większe niż suma zmierzonych faz. Porównanie zależności czasu od $n$ przy $p = 1$ i $p = 48$:

| | $p = 1$, $n = 2{,}5\text{M}$ | $p = 48$, $n = 2{,}5\text{M}$ |
|---|---|---|
| Suma faz [s] | ≈ 0.51 | ≈ 2.57 |
| `t_total` [s] | ≈ 0.51 | ≈ 11.86 |
| Niezliczona różnica [s] | ≈ 0.00 | ≈ 9.29 |

![bs3 threads=1 total](../plots/bs3_static/bs3_threads1_total.png)
![bs3 threads=48 total](../plots/bs3_static/bs3_threads48_total.png)

Przy $p = 1$ suma faz praktycznie pokrywa się z `t_total` — nie ma czasu "ukrytego". Przy $p = 48$ brakujące $\approx 9\,\text{s}$ to koszt `create_buckets(p \cdot k,\ c)` i `destroy_buckets(...)`: $48 \times 10^6$ wywołań `malloc`/`free` wykonywanych sekwencyjnie, poza zakresem pomiaru `t_distribute`. Problem ten narasta liniowo z $p$ i $n$, co tłumaczy odwrócony gradient na heatmapie.

**Prawo Amdahla.** Wariant 3 narusza podstawowe założenie prawa Amdahla: że sekwencyjna frakcja $f_s$ jest stała i niezależna od $p$. Tu oba problemy mają odwrotną charakterystykę:

- Koszt alokacji `thread_buckets` wynosi $O(p \cdot k)$ — rośnie *liniowo* z $p$. Im więcej wątków, tym więcej pracy sekwencyjnej przed pierwszą barierą.
- `t_writeback` rośnie z $p$ z powodu rosnącego nacisku na pamięć przy rozproszonych buforach kubełków.

W efekcie efektywna sekwencyjna frakcja $f_s(p)$ nie jest stałą, lecz funkcją rosnącą — model Amdahla w ogóle nie stosuje się do opisu tego zachowania. Odwrócony gradient na heatmapie jest tego bezpośrednim dowodem: zamiast $S(p) \to 1/f_s$, mamy $S(p) \to 0$ przy $p \to \infty$.

### Porównanie wariantów między sobą


