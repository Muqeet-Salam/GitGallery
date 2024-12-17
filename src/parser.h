#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stddef.h>

bool parse_github_repo(const char *input, char *owner, size_t owner_size,
                       char *repo, size_t repo_size, char *ref, size_t ref_size,
                       char *path, size_t path_size);

#endif // PARSER_H
