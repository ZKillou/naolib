CC = gcc
CFLAGS = -Wall -Wextra -Isrc
LDFLAGS = 

# Recherche de tous les fichiers .c récursivement
SRCS = $(shell find src -name "*.c")
# Transformation des .c en .o dans un dossier build/
OBJS = $(patsubst src/%.c, build/%.o, $(SRCS))

TARGET = exterieur

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Règle générique pour les fichiers objets
build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET)

.PHONY: all clean
