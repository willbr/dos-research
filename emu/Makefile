UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
    CC = gcc
    EXEEXT =
    LIBEXT = .so
    PYTHON = python3
endif
ifeq ($(UNAME_S),Darwin)
    CC = clang
    EXEEXT =
    LIBEXT = .dylib
    PYTHON = python3
endif
ifeq ($(OS),Windows_NT)
    CC = tcc
    EXEEXT = .exe
    LIBEXT = .dll
    PYTHON = python
else
    CC = gcc
    EXEEXT =
    LIBEXT = .so
    PYTHON = python3
endif

test$(EXEEXT): test.c libdosemu$(LIBEXT)
	$(CC) -std=c89 test.c -o test$(EXEEXT) -L. -ldosemu

libdosemu$(LIBEXT): emu.c opcodes.c
	$(CC) -std=c89 -shared emu.c -o libdosemu$(LIBEXT) -DLITTLE_ENDIAN

opcodes.c: build_opcodes.py
	$(PYTHON) build_opcodes.py > opcodes.c

run-test: test$(EXEEXT)
	./test$(EXEEXT) first.com

