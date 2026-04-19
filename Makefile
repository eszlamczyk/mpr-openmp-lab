UNAME_S := $(shell uname -s)

# --- Linux Configuration ---
ifeq ($(UNAME_S),Linux)
    CC      := gcc
    CFLAGS  := -Wall -Wextra -O3 -fopenmp
    LDFLAGS := -fopenmp
endif

# --- macOS Configuration ---
ifeq ($(UNAME_S),Darwin)
    CC      := clang
    CFLAGS  := -Wall -Wextra -O3 -Xclang -fopenmp -I/opt/homebrew/opt/libomp/include
    LDFLAGS := -L/opt/homebrew/opt/libomp/lib -lomp -fopenmp
endif

# Pass CSV=1 to enable CSV output mode:  make bs2 CSV=1
ifeq ($(CSV),1)
    CFLAGS += -DWRITE_TO_CSV
endif

.PHONY: bs2 bs3 clean

clean:
	rm -rf out/*

bs2: out/bucket_sort2.o out/bucket.o out/random.o out/main.o
	mkdir -p out
	$(CC) $(CFLAGS) -DBS2 -o out/bs2 $^ $(LDFLAGS)

out/bucket_sort2.o: src/bucket_sort2.c
	mkdir -p out
	$(CC) $(CFLAGS) -c -o $@ $< -DBS2

bs3: out/bucket_sort3.o out/bucket.o out/random.o out/main.o
	mkdir -p out
	$(CC) $(CFLAGS) -DBS3 -o out/bs3 $^ $(LDFLAGS)

out/bucket_sort3.o: src/bucket_sort3.c
	mkdir -p out
	$(CC) $(CFLAGS) -c -o $@ $< -DBS3

out/bucket.o: src/bucket.c
	mkdir -p out
	$(CC) $(CFLAGS) -c -o $@ $<

out/random.o: src/random.c
	mkdir -p out
	$(CC) $(CFLAGS) -c -o $@ $<

out/main.o: src/main.c
	mkdir -p out
	$(CC) $(CFLAGS) -c -o $@ $<
