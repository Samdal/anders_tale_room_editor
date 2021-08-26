CFLAGS = -std=gnu99 -O0 -g3 -pthread

LDFLAGS = -ldl -lX11 -lXi -lm

build: src/main.c
	gcc -o bin/app $(CFLAGS) src/*.c $(LDFLAGS)

linux-win: src/main.c
	x86_64-w64-mingw32-gcc -o bin/app -Os -pthread src/*.c -static -lkernel32 -luser32 -lshell32 -lgdi32 -lwinmm -lopengl32

.PHONY: run clean

run: build
	./bin/app

clean:
	rm -rf bin/
	mkdir bin/
