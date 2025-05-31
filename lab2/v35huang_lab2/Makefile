# Makefile - ECE252 project with multiple targets

CC       = gcc
CFLAGS   = -Wall -g -std=c99 -Iinclude
LD       = gcc
LDFLAGS  = -g
LDLIBS   = -lz -lcurl -lpthread

SRC_DIR  = src
INC_DIR  = include
OBJ_DIR  = obj

COMMON_SRCS = $(filter-out $(SRC_DIR)/paster.c, $(wildcard $(SRC_DIR)/*.c))
COMMON_OBJS = $(COMMON_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

FIND_SRC = $(SRC_DIR)/paster.c

FIND_OBJ = $(OBJ_DIR)/paster.o

TARGETS = paster

.PHONY: all clean
all: $(TARGETS)


paster: $(FIND_OBJ) $(COMMON_OBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)


$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


-include $(COMMON_OBJS:.o=.d)


clean:
	rm -rf $(OBJ_DIR) paster all.png output_*.png
