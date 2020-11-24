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

SRC_CORE = compiler.c parser.tab.c scanner.yy.c node.c symbol.c type.c ir.c mips.c

EXECS = compiler
SRCS = main.c ${SRC_CORE}
OBJS = $(subst .c,.o,$(SRCS))

TEST_EXECS = runAllTests
TEST_SRCS = runAllTests.c ${SRC_CORE}
TEST_OBJS = $(subst .c,.o,$(TEST_SRCS))

all : $(EXECS)

clean :
	rm -f $(EXECS) ${TEST_EXECS} *.o *.yy.[ch] *.tab.[ch] *.output .depend

depend: .depend

.depend: $(SRCS)
	$(CC) $(CFLAGS) -MM $^ > .depend

-include .depend

compiler: $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LDLIBS)

test: $(TEST_OBJS)
	$(CC) -o runAllTests $(LDFLAGS) $^ $(LDLIBS)