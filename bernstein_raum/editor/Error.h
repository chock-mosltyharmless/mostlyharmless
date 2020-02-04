#pragma once

#include <cstdarg>

#define MAX_ERROR_LENGTH 1024

void ErrorPrint(char *buffer, const char *format, ...);