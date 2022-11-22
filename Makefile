BIN:=voxel
override CFLAGS += -std=gnu11 -D_POSIX_SOURCE -D_GNU_SOURCE
override LDFLAGS += -lm -lrt

.PHONY: all clean

all: voxel

voxel: voxel.o
	gcc voxel.o $(LDFLAGS) -o $(BIN)

voxel.o: voxel.c
	gcc -c voxel.c $(CFLAGS)

clean:
	rm -f $(BIN)
	rm -f *.o
