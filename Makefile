#-------------------------------------------------------------------------------
# A basic Makefile by Adam Mitchell - 09/03/2018
#-------------------------------------------------------------------------------
# Basic operation:
# 1. Put source files in src/
# 2. Put include files in inc/
# 3.  $ make
#
# Object files are placed in /obj and can be removed with:
#  $ make clean
#
# #include directives search in the following order:
# 1. The subdirectory of inc/ that matches the subdirectory of src/ of the file
# 2. The inc/ directory
#-------------------------------------------------------------------------------

#Compiler Flags
CC = gcc
FLAGS = -Wall -g
CFLAGS = -I/usr/local/include
OFLAGS = -L/usr/local/lib -llua -lm -ldl

#Executable name
TARGET = le

#Object list (corresponds to source files)
OBJECTS = le.o

SRCDIR = src
INCDIR = inc
OBJDIR = obj

#-------------------------------------------------------------------------------
_OBJ = $(addprefix $(OBJDIR)/,$(OBJECTS))

.PHONY: all clean cleanall

all: CFLAGS += $(_OUT)
all: $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(INCDIR)/$(patsubst $(SRCDIR)/%,%,$(dir $<)) -I$(INCDIR) -c -o $@ $<

$(TARGET): $(_OBJ)
	$(CC) -o $@ $^ $(OFLAGS)

clean:
	rm -rf $(OBJDIR)

cleanall: clean
	rm -f $(TARGET)
