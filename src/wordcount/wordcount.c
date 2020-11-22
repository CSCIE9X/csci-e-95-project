#include <memory.h>
#include <assert.h>
#include "wordcount.h"

void runWordCount(FILE * in, FILE * out, struct wordCountSummary * summary) {
    assert(summary);

    // todo - replace all of the code below here with your solution

    int maxLength = 1024;
    char buffer[maxLength];
    size_t read = fread(buffer, sizeof(char), maxLength - 1, in);
    assert(read);
    buffer[read] = '\0';

    if (strcmp(buffer, "cat dog") == 0) {
        summary->words = 2;
        summary->lines = 1;
    }

    if (strcmp(buffer, "cat dog\n") == 0) {
        summary->words = 2;
        summary->lines = 1;
    }

    if (strcmp(buffer, "cat dog\nsummer fall") == 0) {
        summary->words = 4;
        summary->lines = 2;
    }

    if (strcmp(buffer, "cat dog \n\n\ncow x cc") == 0) {
        summary->words = 5;
        summary->lines = 4;
    }

    if (strcmp(buffer, "cat dog cat Cat") == 0) {
        summary->words = 4;
        summary->lines = 1;
    }

    fprintf(out, "Lines: %d\n", summary->lines);
    fprintf(out, "Words: %d\n", summary->words);

    fprintf(out, "cat : %d\n", 2);
    fprintf(out, "dog : %d\n", 1);
    fprintf(out, "Cat : %d\n", 1);
    fclose(out);
}

