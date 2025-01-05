UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
    CC = gcc
    CFLAGS = -std=c89 -DLITTLE_ENDIAN
    EXEEXT =
    LIBEXT = .so
    PYTHON = python3
else ifeq ($(UNAME_S),Darwin)
    CC = clang
    CFLAGS = -std=c89 -Wno-multichar
    EXEEXT =
    LIBEXT = .dylib
    PYTHON = python3
else ifeq ($(OS),Windows_NT)
    CC = tcc
    CFLAGS = -std=c89 -DLITTLE_ENDIAN
    EXEEXT = .exe
    LIBEXT = .dll
    PYTHON = python
else
    CC = gcc
    CFLAGS = -std=c89 -DLITTLE_ENDIAN
    EXEEXT =
    LIBEXT = .so
    PYTHON = python3
endif

test$(EXEEXT): test.c libdosemu$(LIBEXT)
	$(CC) $(CFLAGS) test.c -o test$(EXEEXT) -L. -ldosemu

libdosemu$(LIBEXT): emu.c opcodes.c
	$(CC) $(CFLAGS) -shared emu.c -o libdosemu$(LIBEXT) -DLITTLE_ENDIAN

opcodes.c: build_opcodes.py
	$(PYTHON) build_opcodes.py > opcodes.c

run-test: test$(EXEEXT)
	./test$(EXEEXT) first.com

