UNAME_S := $(shell uname -s)

# TODO
# --- Linux Configuration ---
ifeq ($(UNAME_S),Linux)
    CC      := gcc
    CFLAGS  := -Wall -Wextra -O3 -fPIC
    LDFLAGS := -lm -lrt -lpthread
endif

# --- macOS Configuration ---
ifeq ($(UNAME_S),Darwin)
    CC      := clang
    CFLAGS  := -Wall -Wextra -O3 -Xclang -fopenmp -I/opt/homebrew/opt/libomp/include
    LDFLAGS := -L/opt/homebrew/opt/libomp/lib -lomp
endif

.PHONY: bs2 clean

clean:
	rm -rf out/*

bs2: out/bucket_sort2.o out/bucket.o out/random.o out/main.o
	mkdir -p out
	$(CC) $(CFLAGS) -DBS2 -o out/bs2 $^ $(LDFLAGS)

out/bucket_sort2.o: src/bucket_sort2.c
	mkdir -p out
	$(CC) $(CFLAGS) -c -o $@ $< -DBS2

out/bucket.o: src/bucket.c
	mkdir -p out
	$(CC) $(CFLAGS) -c -o $@ $<

out/random.o: src/random.c
	mkdir -p out
	$(CC) $(CFLAGS) -c -o $@ $<

out/main.o: src/main.c
	mkdir -p out
	$(CC) $(CFLAGS) -c -o $@ $<
