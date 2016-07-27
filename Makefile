TARGET  = mdmap
CFLAGS ?= -O2 -Wall
#CFLAGS ?= -g -Wall
LFLAGS  = -lusb-1.0
CC     ?= gcc
OBJDIR = obj

SRCS = $(wildcard *.c)
OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(PREFIX)$(CC) -o $(TARGET) $(OBJECTS) $(LFLAGS)

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(PREFIX)$(CC) -c -MMD -MP $(CFLAGS) $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: clean
clean:
	@rm -rf $(OBJDIR)

.PHONY: mrproper
mrproper: | clean
	@rm -f $(TARGET)

# Include auto-generated dependencies
-include $(SRCS:%.c=$(OBJDIR)/%.d)

