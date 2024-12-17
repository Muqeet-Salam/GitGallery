#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

bool is_image_extension(const char *name) {
    if (!name)
        return false;

    const char *ext = strrchr(name, '.');
    if (!ext || ext == name)
        return false;

    ext++;
    if (strcasecmp(ext, "png") == 0) return true;
    if (strcasecmp(ext, "jpg") == 0) return true;
    if (strcasecmp(ext, "jpeg") == 0) return true;
    if (strcasecmp(ext, "webp") == 0) return true;
    if (strcasecmp(ext, "gif") == 0) return true;
    return false;
}

char *read_file_text(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    if (fread(buffer, 1, size, f) != (size_t)size) {
        free(buffer);
        fclose(f);
        return NULL;
    }

    buffer[size] = '\0';
    fclose(f);
    return buffer;
}

char *url_encode(const char *str) {
    if (!str) return NULL;
    char *enc = malloc(strlen(str) * 3 + 1);
    char *p = enc;
    while (*str) {
        if (isalnum((unsigned char)*str) || strchr("-_.~", *str)) {
            *p++ = *str;
        } else {
            sprintf(p, "%%%02X", (unsigned char)*str);
            p += 3;
        }
        str++;
    }
    *p = '\0';
    return enc;
}
