#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

bool is_image_extension(const char *name);
char *read_file_text(const char *path);
char *url_encode(const char *str);

#endif // UTILS_H
