#include "parser.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static bool is_valid_segment(const char *s) {
    return s && *s != '\0';
}

static void safe_copy(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0)
        return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    snprintf(dst, dst_size, "%s", src);
}

bool parse_github_repo(const char *input, char *owner, size_t owner_size,
                       char *repo, size_t repo_size, char *ref, size_t ref_size,
                       char *path, size_t path_size) {
    if (!input || !owner || !repo || !ref || !path)
        return false;

    const char *p = input;
    if (strncmp(p, "https://", 8) == 0) p += 8;
    else if (strncmp(p, "http://", 7) == 0) p += 7;

    if (strncmp(p, "www.", 4) == 0)
        p += 4;

    if (strncmp(p, "github.com/", 11) != 0)
        return false;
    p += 11;

    char tmp[1024];
    strncpy(tmp, p, sizeof(tmp)-1);
    tmp[sizeof(tmp)-1] = '\0';

    char *parts[10];
    int count = 0;
    char *tok = strtok(tmp, "/");
    while (tok && count < 10) {
        parts[count++] = tok;
        tok = strtok(NULL, "/");
    }

    if (count < 2)
        return false;

    if (!is_valid_segment(parts[0]) || !is_valid_segment(parts[1]))
        return false;

    safe_copy(owner, owner_size, parts[0]);
    safe_copy(repo, repo_size, parts[1]);

    size_t repo_len = strlen(repo);
    if (repo_len > 4 && strcmp(repo + repo_len - 4, ".git") == 0) {
        repo[repo_len - 4] = '\0';
    }

    ref[0] = '\0';
    path[0] = '\0';

    int path_start = 2;
    if (count > 3 && (strcmp(parts[2], "tree") == 0 || strcmp(parts[2], "blob") == 0)) {
        safe_copy(ref, ref_size, parts[3]);
        path_start = 4;
    }

    if (count > path_start) {
        size_t len = 0;
        for (int i = path_start; i < count; ++i) {
            if (i > path_start) {
                if (len + 1 < path_size) {
                    path[len++] = '/';
                    path[len] = '\0';
                }
            }
            size_t add = strlen(parts[i]);
            if (len + add + 1 >= path_size)
                break;
            memcpy(path + len, parts[i], add);
            len += add;
            path[len] = '\0';
        }
    }

    return true;
}
