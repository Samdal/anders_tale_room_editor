CFLAGS = -std=gnu99 -O0 -g3 -pthread
LDFLAGS = -ldl -lX11 -lXi -lm
name = bin/anders_tale_room_editor

build: src/main.c
	gcc -o $(name) $(CFLAGS) src/*.c $(LDFLAGS)

linux-win: src/main.c
	x86_64-w64-mingw32-gcc -o $(name) -O3 -mwindows -pthread src/*.c -static -lkernel32 -luser32 -lshell32 -lgdi32 -lwinmm -lopengl32 -lcomdlg32

.PHONY: run clean

run: build
	./$(name)

clean:
	rm -rf bin/
	mkdir bin/
