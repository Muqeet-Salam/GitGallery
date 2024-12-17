#include "github.h"
#include "utils.h"
#include <stdbool.h>
#include <curl/curl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct memory {
    char *data;
    size_t size;
};

static size_t curl_data_write(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct memory *mem = (struct memory *)userp;
    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) return 0;
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = '\0';
    return realsize;
}

static char *fetch_url(const char *url) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    struct memory chunk = {0};
    CURLcode res;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "github-image-viewer/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_data_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(chunk.data);
        return NULL;
    }

    return chunk.data;
}

static char *extract_json_string(const char *src, const char *key) {
    const char *p = strstr(src, key);
    if (!p) return NULL;

    p = strchr(p, ':');
    if (!p) return NULL;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '"') return NULL;
    p++;

    const char *start = p;
    while (*p && *p != '"') {
        if (*p == '\\' && *(p + 1)) p += 2;
        else p++;
    }
    if (*p != '"') return NULL;

    size_t len = p - start;
    char *value = malloc(len + 1);
    if (!value) return NULL;
    memcpy(value, start, len);
    value[len] = '\0';
    return value;
}

static bool append_text(char **dst, size_t *len, const char *text) {
    size_t add = strlen(text);
    char *next = realloc(*dst, *len + add + 1);
    if (!next)
        return false;
    *dst = next;
    memcpy(*dst + *len, text, add + 1);
    *len += add;
    return true;
}

static bool append_fmt(char **dst, size_t *len, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (needed < 0)
        return false;

    size_t n = (size_t)needed;
    char *next = realloc(*dst, *len + n + 1);
    if (!next)
        return false;
    *dst = next;

    va_start(args, fmt);
    vsnprintf(*dst + *len, n + 1, fmt, args);
    va_end(args);
    *len += n;
    return true;
}

static char *dup_n(const char *s, size_t n) {
    char *out = malloc(n + 1);
    if (!out)
        return NULL;
    memcpy(out, s, n);
    out[n] = '\0';
    return out;
}

static bool starts_with_path_prefix(const char *path, const char *prefix) {
    size_t n = strlen(prefix);
    if (strncmp(path, prefix, n) != 0)
        return false;
    return path[n] == '\0' || path[n] == '/';
}

static bool folder_seen(char **folders, size_t folder_count, const char *name) {
    for (size_t i = 0; i < folder_count; ++i) {
        if (strcmp(folders[i], name) == 0)
            return true;
    }
    return false;
}

static char *encode_path_preserve_slash(const char *path) {
    char *encoded = url_encode(path);
    if (!encoded)
        return NULL;

    size_t n = strlen(encoded);
    char *fixed = malloc(n + 1);
    if (!fixed) {
        free(encoded);
        return NULL;
    }

    size_t i = 0;
    size_t j = 0;
    while (encoded[i] != '\0') {
        if (encoded[i] == '%' && (encoded[i + 1] == '2') &&
            (encoded[i + 2] == 'F' || encoded[i + 2] == 'f')) {
            fixed[j++] = '/';
            i += 3;
            continue;
        }
        fixed[j++] = encoded[i++];
    }
    fixed[j] = '\0';
    free(encoded);
    return fixed;
}

char *github_fetch_image_list(const char *owner, const char *repo,
                              const char *path, const char *ref,
                              int recursive, int page, int per_page) {
    char api_url[2048];
    const char *base_path = (path && *path) ? path : "";
    const char *ref_value = (ref && *ref) ? ref : "HEAD";
    char *encoded_ref = NULL;
    encoded_ref = url_encode(ref_value);
    if (!encoded_ref)
        return NULL;

    snprintf(api_url, sizeof(api_url),
             "https://api.github.com/repos/%s/%s/git/trees/%s?recursive=1",
             owner, repo, encoded_ref);
    free(encoded_ref);

    char *json_text = fetch_url(api_url);
    if (!json_text) return NULL;

    char *out = malloc(1);
    if (!out) {
        free(json_text);
        return NULL;
    }
    out[0] = '\0';
    size_t out_len = 0;

    if (!append_fmt(&out, &out_len, "{\"path\":\"%s\",\"folders\":[", base_path)) {
        free(json_text);
        free(out);
        return NULL;
    }

    if (page < 1)
        page = 1;
    if (per_page < 1)
        per_page = 50;
    if (per_page > 200)
        per_page = 200;

    bool first_folder = true;
    bool first_image = true;
    char **folders = NULL;
    size_t folder_count = 0;

    const char *cursor = json_text;

    while (1) {
        const char *path_pos = strstr(cursor, "\"path\":\"");
        if (!path_pos)
            break;

        const char *obj_start = path_pos;
        while (obj_start > json_text && *obj_start != '{')
            obj_start--;

        char *item_path = extract_json_string(obj_start, "\"path\"");
        char *item_type = extract_json_string(obj_start, "\"type\"");
        if (!item_path || !item_type) {
            free(item_path);
            free(item_type);
            cursor = path_pos + 8;
            continue;
        }

        const char *rel = item_path;
        if (*base_path) {
            if (!starts_with_path_prefix(item_path, base_path)) {
                free(item_path);
                free(item_type);
                cursor = path_pos + 8;
                continue;
            }
            if (strcmp(item_path, base_path) == 0) {
                free(item_path);
                free(item_type);
                cursor = path_pos + 8;
                continue;
            }
            rel = item_path + strlen(base_path);
            if (*rel == '/')
                rel++;
        }

        if (*rel == '\0') {
            free(item_path);
            free(item_type);
            cursor = path_pos + 8;
            continue;
        }

        const char *slash = strchr(rel, '/');
        if (strcmp(item_type, "tree") == 0) {
            size_t seg_len = slash ? (size_t)(slash - rel) : strlen(rel);
            char *folder_name = dup_n(rel, seg_len);
            if (folder_name && !folder_seen(folders, folder_count, folder_name)) {
                char **next_folders = realloc(folders, (folder_count + 1) * sizeof(char *));
                if (next_folders) {
                    folders = next_folders;
                    folders[folder_count++] = folder_name;

                    char full_path[2048];
                    if (*base_path)
                        snprintf(full_path, sizeof(full_path), "%s/%s", base_path, folder_name);
                    else
                        snprintf(full_path, sizeof(full_path), "%s", folder_name);

                    if (!first_folder)
                        append_text(&out, &out_len, ",");
                    append_fmt(&out, &out_len, "{\"name\":\"%s\",\"path\":\"%s\"}", folder_name, full_path);
                    first_folder = false;
                } else {
                    free(folder_name);
                }
            } else {
                free(folder_name);
            }
        }

        free(item_path);
        free(item_type);
        cursor = path_pos + 8;
    }

    append_text(&out, &out_len, "],\"images\":[");

    cursor = json_text;
    size_t image_index = 0;
    size_t total_images = 0;
    size_t start = (size_t)(page - 1) * (size_t)per_page;
    size_t end = start + (size_t)per_page;
    while (1) {
        const char *path_pos = strstr(cursor, "\"path\":\"");
        if (!path_pos)
            break;

        const char *obj_start = path_pos;
        while (obj_start > json_text && *obj_start != '{')
            obj_start--;

        char *item_path = extract_json_string(obj_start, "\"path\"");
        char *item_type = extract_json_string(obj_start, "\"type\"");
        if (!item_path || !item_type) {
            free(item_path);
            free(item_type);
            cursor = path_pos + 8;
            continue;
        }

        const char *rel = item_path;
        if (*base_path) {
            if (!starts_with_path_prefix(item_path, base_path)) {
                free(item_path);
                free(item_type);
                cursor = path_pos + 8;
                continue;
            }
            if (strcmp(item_path, base_path) == 0) {
                free(item_path);
                free(item_type);
                cursor = path_pos + 8;
                continue;
            }
            rel = item_path + strlen(base_path);
            if (*rel == '/')
                rel++;
        }

        const char *slash = strchr(rel, '/');
        if (strcmp(item_type, "blob") == 0 && is_image_extension(rel) &&
            (recursive || !slash)) {
            total_images++;
            if (image_index >= start && image_index < end) {
                char *encoded_repo_path = encode_path_preserve_slash(item_path);
                if (encoded_repo_path) {
                    const char *name = strrchr(item_path, '/');
                    name = name ? name + 1 : item_path;
                    if (!first_image)
                        append_text(&out, &out_len, ",");
                    append_fmt(&out, &out_len,
                               "{\"name\":\"%s\",\"path\":\"%s\",\"download_url\":\"https://raw.githubusercontent.com/%s/%s/%s/%s\"}",
                               name, item_path, owner, repo, ref_value, encoded_repo_path);
                    first_image = false;
                    free(encoded_repo_path);
                }
            }
            image_index++;
        }

        free(item_path);
        free(item_type);
        cursor = path_pos + 8;
    }

    size_t total_pages = (total_images + (size_t)per_page - 1) / (size_t)per_page;
    append_fmt(&out, &out_len,
               "],\"pagination\":{\"page\":%d,\"per_page\":%d,\"total_images\":%zu,\"total_pages\":%zu}}",
               page, per_page, total_images, total_pages);

    for (size_t i = 0; i < folder_count; ++i) {
        free(folders[i]);
    }
    free(folders);

    free(json_text);
    return out;
}
