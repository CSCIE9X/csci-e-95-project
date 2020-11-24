#include <stdio.h>
#include <stdarg.h>

#include "compiler.h"
#include "parser.h"

extern int errno;

void compiler_print_error(YYLTYPE location, const char *format, ...) {
    va_list ap;
    fprintf(stdout, "Error (%d, %d) to (%d, %d): ",
            location.first_line, location.first_column,
            location.last_line, location.last_column);
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
    fputc('\n', stdout);
}

