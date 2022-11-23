SOURCES := src/voxel_utils.c src/voxel.c src/main.c
OBJECTS := $(SOURCES:src/%.c=bin/%.o)
HEADERS := 3rd/uthash.h src/include/voxel.h src/include/voxel_utils.h src/include/voxel_types.h
INCLUDES := 3rd src/include

override CFLAGS += -std=c11 -Wall -Wextra -D_POSIX_SOURCE -D_GNU_SOURCE $(INCLUDES:%=-I%)
override LDFLAGS += -lm -lrt

.PHONY: all clean

all: bin/voxel

bin/voxel: $(OBJECTS)
	@printf "\033[0;32mLinking \"$@\"...\033[0m\n"
	@gcc $(OBJECTS) $(LDFLAGS) -o bin/voxel

bin/%.o: src/%.c Makefile $(HEADERS)
	@printf "\033[0;32mBuilding \"$@\"...\033[0m\n"
	@gcc $< $(CFLAGS) -c -o $@

clean:
	@rm -f $(OBJECTS) bin/voxel
