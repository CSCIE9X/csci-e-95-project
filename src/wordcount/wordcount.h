#ifndef WORDCOUNT_WORDCOUNT_H
#define WORDCOUNT_WORDCOUNT_H
#include <stdio.h>

struct wordCountSummary {
    int lines;
    int words;
};

void runWordCount(FILE * in, FILE * out, struct wordCountSummary * summary);

#endif //WORDCOUNT_WORDCOUNT_H
