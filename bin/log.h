#ifndef LOG_H
#define LOG_H
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#define MAX_COUNT 1024
#define MAX_STRING_LEN 2048
void logOpen(char *file);
void logging(const char* tag, const char* message, ...);
void setLogLevel(int level);
void logClose();
#endif /* LOG_H */
