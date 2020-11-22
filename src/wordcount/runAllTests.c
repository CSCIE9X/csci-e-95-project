//
// Created by Mark Ford on 5/19/20.
//
#define CTEST_MAIN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ctest.h"
#include "wordcount.h"

int main(int argc, char **argv) {
    ctest_main(argc, (const char **) argv);
    return 0;
}

void assertSummary(char * input, char * buffer, int len, struct wordCountSummary *expected) {
    FILE *in = fmemopen((void *) input, strlen(input), "r");
    FILE *out = fmemopen((void *) buffer, len, "w");

    struct wordCountSummary summary;
    runWordCount(in, out, &summary);

    ASSERT_EQUAL(expected->words, summary.words);
    ASSERT_EQUAL(expected->lines, summary.lines);

    fclose(in);
    fclose(out);
}

void assertOutput(char * input, char * buffer, int len, char * expectedOutput) {
    FILE *in = fmemopen((void *) input, strlen(input), "r");
    FILE *out = fmemopen((void *) buffer, len, "w");

    struct wordCountSummary summary;
    runWordCount(in, out, &summary);

    ASSERT_STR(expectedOutput, buffer);

    fclose(in);
    fclose(out);
}

CTEST_DATA(wcSummary) {
    int bufferLength;
    char* buffer;
};

// Optional setup function for suite, called before every test in suite
CTEST_SETUP(wcSummary) {
    data->bufferLength = 1024;
    data->buffer = (char*)malloc(data->bufferLength);
    CTEST_LOG("%s() data=%p buffer=%p", __func__, (void*)data, (void*)data->buffer);
}

// Optional teardown function for suite, called after every test in suite
CTEST_TEARDOWN(wcSummary) {
    if (data->buffer) free(data->buffer);
}

CTEST2(wcSummary, noLineBreaks) {
    struct wordCountSummary expected = {1, 2};
    assertSummary("cat dog", data->buffer, data->bufferLength, &expected);
}

CTEST2(wcSummary, oneLineBreaks) {
    struct wordCountSummary expected = {1, 2};
    assertSummary("cat dog\n", data->buffer, data->bufferLength, &expected);
}

CTEST2(wcSummary, twoLines) {
    struct wordCountSummary expected = {2, 4};
    assertSummary("cat dog\nsummer fall", data->buffer, data->bufferLength, &expected);
}

CTEST2(wcSummary, multipleLines) {
    struct wordCountSummary expected = {4, 5};
    assertSummary("cat dog \n\n\ncow x cc", data->buffer, data->bufferLength, &expected);
}

CTEST2(wcSummary, output) {
    assertOutput("cat dog cat Cat", data->buffer, data->bufferLength,
            "Lines: 1\n"
            "Words: 4\n"
            "cat : 2\n"
            "dog : 1\n"
            "Cat : 1\n");
}
