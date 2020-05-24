#ifndef GITI_LOG_H_
#define GITI_LOG_H_

#include <stdio.h>

#define log(format, ...) fprintf(stderr, format, ##__VA_ARGS__); fprintf(stderr, "\n")

#define log_debug(format, ...) fprintf(stderr, "[%s:%u]", __func__, __LINE__); fprintf(stderr, format, ##__VA_ARGS__); fprintf(stderr, "\n")

#endif
