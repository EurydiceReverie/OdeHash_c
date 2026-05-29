# OdeHash v5 — Unified Makefile
# Build: make          (build all)
# Test:  make test     (run all tests)
# Clean: make clean

CC       = gcc
CFLAGS   = -O2 -Wall -Wextra -std=c11 -Iinclude
LDFLAGS  = -lm

SRCDIR   = src
BUILDDIR = build
TESTDIR  = tests

# v5 source files (256-bit field)
V5_OBJS = $(BUILDDIR)/permutation256.o $(BUILDDIR)/sponge256.o $(BUILDDIR)/hash_v5.o

.PHONY: all clean test test_v5 bench stream consttime

all: $(BUILDDIR)/ode_hash_v5$(EXE) $(BUILDDIR)/test_v5$(EXE)

# ---- main binary ----
$(BUILDDIR)/ode_hash_v5$(EXE): $(SRCDIR)/main.c $(V5_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ---- test binary ----
$(BUILDDIR)/test_v5$(EXE): $(TESTDIR)/test_v5.c $(V5_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ---- benchmark ----
$(BUILDDIR)/bench_v5$(EXE): $(TESTDIR)/bench.c $(V5_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ---- stream generator ----
$(BUILDDIR)/gen_stream_v5$(EXE): $(TESTDIR)/gen_stream_v5.c $(V5_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ---- constant-time test ----
$(BUILDDIR)/consttime_v5$(EXE): $(TESTDIR)/consttime.c $(V5_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ---- object files ----
$(BUILDDIR)/permutation256.o: $(SRCDIR)/permutation256.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/sponge256.o: $(SRCDIR)/sponge256.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/hash_v5.o: $(SRCDIR)/hash_v5.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR):
	if not exist $(BUILDDIR) mkdir $(BUILDDIR)

# ---- test targets ----
test: $(BUILDDIR)/test_v5$(EXE)
	@echo "=== OdeHash v5 Test Vectors ==="
	$(BUILDDIR)/test_v5$(EXE)

bench: $(BUILDDIR)/bench_v5$(EXE)
	$(BUILDDIR)/bench_v5$(EXE)

stream: $(BUILDDIR)/gen_stream_v5$(EXE)
	$(BUILDDIR)/gen_stream_v5$(EXE)

consttime: $(BUILDDIR)/consttime_v5$(EXE)
	$(BUILDDIR)/consttime_v5$(EXE)

# ---- clean ----
clean:
	if exist $(BUILDDIR) rmdir /s /q $(BUILDDIR)
