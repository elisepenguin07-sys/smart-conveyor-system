#Variables declaration
#1 comipler and tools
CC = gcc

#2 Flags
CFLAGS = -Wall -g -O2
CPPFLAGS = -I./include -I/opt/homebrew/include
LDFLAGS = -lpaho-mqtt3cs

#3 Source files
SRC_DIR = .
BUILD_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))


#Phony targets
.PHONY: all clean

#Default Target
all: $(BUILD_DIR)/publisher

#Pattern Rules & Compilation
$(BUILD_DIR)/publisher: $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
clean:
	rm -rf $(BUILD_DIR)