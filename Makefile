CC := gcc
SRCDIR := src
OBJDIR := obj
DEPDIR := include
TARGET := jcomp
CFLAGS := -Wall -Wextra -Wpedantic -g

LIBS := 

_OBJS := jlex.o jparse.o main.o jsym.o classParser.o subroutineParser.o \
			expressionParser.o statementParser.o jgen.o 

OBJS := $(patsubst %,$(OBJDIR)/%,$(_OBJS))

_DEPS := jack.h jlex.h jparse.h jsym.h jgen.h
DEPS := $(patsubst %,$(DEPDIR)/%,$(_DEPS))

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPS) 
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(OBJDIR)/*.o $(TARGET)