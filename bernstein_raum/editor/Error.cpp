#include "Error.h"

#include <stdio.h>

void ErrorPrint(char *buffer, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsprintf_s(buffer, MAX_ERROR_LENGTH - 1, format, args);
    va_end(args);
}