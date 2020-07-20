CC_LIBS  = -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL raylib/src/libraylib.a
CC_INC   = -I. -Iraylib/src

all:
	mkdir -p build && cd build && cmake .. && cmake --build .

test:
	wget "https://github.com/raysan5/raylib/releases/download/3.0.0/raylib-noexamples-3.0.0.tar.gz"
	tar -xzf "raylib-noexamples-3.0.0.tar.gz"
	rm "raylib-noexamples-3.0.0.tar.gz"
	mv "raylib-3.0.0/" "raylib/"
	if [ -f "raylib/src/raylib.h" ]; then echo "raylib successfully downloaded"; else exit 1; fi
	make -C raylib/src/
	if [ -f "raylib/src/libraylib.a" ]; then echo "raylib successfully compiled"; else exit 2; fi
	clang $(CC_LIBS) $(CC_INC) -o what demo.c