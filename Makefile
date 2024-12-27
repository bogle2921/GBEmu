INCLUDES= -I ./include
FLAGS= -g

OBJECTS=./build/cart.o
all: ${OBJECTS}
	gcc ${FLAGS} ${INCLUDES} ./src/main.c ${OBJECTS} -L ./lib -lmingw32 -lSDL2main -lSDL2 -o ./bin/gb-emu

./build/cart.o:./src/cart.c
	gcc ${FLAGS} ${INCLUDES} ./src/cart.c -c -o ./build/cart.o

clean:
	del build\*