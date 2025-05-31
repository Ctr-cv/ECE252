# Makefile - ECE252 project with multiple targets

CC       = gcc
CFLAGS   = -Wall -g -std=c99 -Iinclude
LD       = gcc
LDFLAGS  = -g
LDLIBS   = -lz

SRC_DIR  = src
INC_DIR  = include
OBJ_DIR  = obj

COMMON_SRCS = $(filter-out $(SRC_DIR)/find_png.c $(SRC_DIR)/cat_png.c, $(wildcard $(SRC_DIR)/*.c))
COMMON_OBJS = $(COMMON_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

FIND_SRC = $(SRC_DIR)/find_png.c
CAT_SRC  = $(SRC_DIR)/cat_png.c

FIND_OBJ = $(OBJ_DIR)/find_png.o
CAT_OBJ  = $(OBJ_DIR)/cat_png.o

TARGETS = findpng catpng

.PHONY: all clean
all: $(TARGETS)


findpng: $(FIND_OBJ) $(COMMON_OBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

catpng: $(CAT_OBJ) $(COMMON_OBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)


$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


-include $(COMMON_OBJS:.o=.d)


clean:
	rm -rf $(OBJ_DIR) findpng catpng all.png
