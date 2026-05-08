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
    LDFLAGS := -L/opt/homebrew/opt/libomp/lib -lomp -Xclang -fopenmp
endif

.PHONY: bs2 bs3 bs2_check bs3_check scheduler all clean

all: out/bs2 out/bs3 out/bs2_check out/bs3_check out/scheduler

bs2: out/bs2
bs2_check: out/bs2_check

bs3: out/bs3
bs3_check: out/bs3_check

scheduler: out/scheduler

clean:
	rm -rf out/*

out/bs2: out/bucket_sort2.o out/bucket.o out/random.o out/output.o out/main.o
	mkdir -p out
	$(CC) $(CFLAGS) -DBS2 -o $@ $^ $(LDFLAGS)

out/bs2_check: out/bucket_sort2.o out/bucket.o out/random.o out/output.o out/sort_check.o out/main_check.o
	mkdir -p out
	$(CC) $(CFLAGS) -DBS2 -DSORT_CHECK -o $@ $^ $(LDFLAGS)

out/bucket_sort2.o: src/bucket_sort2.c
	mkdir -p out
	$(CC) $(CFLAGS) -c -o $@ $< -DBS2

out/bs3: out/bucket_sort3.o out/bucket.o out/random.o out/output.o out/main.o
	mkdir -p out
	$(CC) $(CFLAGS) -DBS3 -o $@ $^ $(LDFLAGS)

out/bs3_check: out/bucket_sort3.o out/bucket.o out/random.o out/output.o out/sort_check.o out/main_check.o
	mkdir -p out
	$(CC) $(CFLAGS) -DBS3 -DSORT_CHECK -o $@ $^ $(LDFLAGS)

out/bucket_sort3.o: src/bucket_sort3.c
	mkdir -p out
	$(CC) $(CFLAGS) -c -o $@ $< -DBS3

out/bucket.o: src/bucket.c
	mkdir -p out
	$(CC) $(CFLAGS) -c -o $@ $<

out/random.o: src/random.c
	mkdir -p out
	$(CC) $(CFLAGS) -c -o $@ $<

out/output.o: src/output.c
	mkdir -p out
	$(CC) $(CFLAGS) -c -o $@ $<

out/main.o: src/main.c
	mkdir -p out
	$(CC) $(CFLAGS) -c -o $@ $<

out/main_check.o: src/main.c
	mkdir -p out
	$(CC) $(CFLAGS) -DSORT_CHECK -c -o $@ $<

out/sort_check.o: src/sort_check.c
	mkdir -p out
	$(CC) $(CFLAGS) -c -o $@ $< -DSORT_CHECK

out/scheduler: src/scheduler_bench.c
	mkdir -p out
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
