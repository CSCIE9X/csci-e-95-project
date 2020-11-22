# https://www.gnu.org/software/make/manual/make.html
CC = gcc
CFLAGS += -g -Wall -Wextra -pedantic

EXECS = wordcount-main
SRCS = wordcount-main.c wordcount.c
OBJS = $(subst .c,.o,$(SRCS))

TEST_EXECS = runAllTests
TEST_SRCS = wordcount.c runAllTests.c
TEST_OBJS = $(subst .c,.o,$(TEST_SRCS))

all : $(EXECS)

clean :
	rm -f $(EXECS) $(TEST_EXECS) *.o *.output .depend*

depend: .depend

.depend: $(SRCS)
	$(CC) $(CFLAGS) -MM $^ > .depend

-include .depend

wordcount-main: $(OBJS)
	$(CC) -o wordcount $^

.depend_tests: $(TEST_SRCS)
	$(CC) $(CFLAGS) -MM $^ > .depend_tests

-include .depend_tests

test: $(TEST_OBJS)
	$(CC) -o runAllTests $^
	./runAllTests