CC = gcc
CFLAGS += -g -Wall -Wextra -pedantic
LDFLAGS =

# Begin Flex setup.
LEX = flex
LFLAGS +=

ifeq (Darwin,$(shell uname))
	LDLIBS += -ll
else
	LDLIBS += -lfl
endif

%.yy.c : %.lex
	$(LEX) $(LFLAGS) --header-file=$(subst .yy.c,.yy.h,$@) -o $@ $<

%.yy.h : %.yy.c
	@touch $@
# End Flex setup.

# Begin Bison setup.
YACC = bison
YFLAGS +=
LDLIBS += -ly

%.tab.c : %.y
	$(YACC) $(YFLAGS) $<

%.tab.h : %.tab.c
	@touch $@
# End Bison setup.

EXECS = compiler
SRCS = compiler.c parser.tab.c scanner.yy.c node.c symbol.c type.c ir.c mips.c
OBJS = $(subst .c,.o,$(SRCS))

all : $(EXECS)

clean :
	rm -f $(EXECS) *.o *.yy.[ch] *.tab.[ch] *.output .depend

depend: .depend

.depend: $(SRCS)
	$(CC) $(CFLAGS) -MM $^ > .depend

-include .depend

compiler: $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LDLIBS)