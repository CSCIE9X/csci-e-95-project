#include <stdio.h>
#include "wordcount.h"

int main(int argc, char **argv) {
    char *filename = argv[argc - 1];
    FILE* input = fopen(filename, "r");
    struct wordCountSummary wcs;
    wcs.words = -1;
    wcs.lines = -1;
    runWordCount(input, stdout, &wcs);
    return 0;
}
