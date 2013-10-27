CC = g++
INC = -I.
OPT = -O3
CFLAGS = $(INC) $(OPT)

DEPS = cache.h		block.h		buffer.h
OBJ = 	main.o		cache.o		block.o		buffer.o

sim_cache: $(OBJ)
	$(CC) -o $@ $^ 

%.o: %.cpp $(DEPS)
	$(CC) -c $< -o $@ $(CFLAGS)

clean: 
	rm -rf *.o
