#include "router.h"
#include "parser.h"
#include "github.h"
#include "utils.h"
#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>

static int send_response(struct MHD_Connection *connection, const char *data,
                         int status, const char *mime) {
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(data),
                                                                    (void *)data,
                                                                    MHD_RESPMEM_MUST_COPY);
    if (!response)
        return MHD_NO;

    MHD_add_response_header(response, "Content-Type", mime);
    int ret = MHD_queue_response(connection, status, response);
    MHD_destroy_response(response);
    return ret;
}

static int serve_static(struct MHD_Connection *connection, const char *url) {
    const char *file = "public/index.html";
    if (strcmp(url, "/style.css") == 0)
        file = "public/style.css";
    else if (strcmp(url, "/script.js") == 0)
        file = "public/script.js";

    char *body = read_file_text(file);
    if (!body)
        return send_response(connection, "Not Found", MHD_HTTP_NOT_FOUND, "text/plain");

    const char *mime = "text/html";
    if (strstr(file, ".css"))
        mime = "text/css";
    else if (strstr(file, ".js"))
        mime = "application/javascript";

    int r = send_response(connection, body, MHD_HTTP_OK, mime);
    free(body);
    return r;
}

enum MHD_Result router_handle_request(void *cls, struct MHD_Connection *connection,
                                      const char *url, const char *method,
                                      const char *version, const char *upload_data,
                                      size_t *upload_data_size, void **con_cls) {
    (void)cls;
    (void)version;
    (void)upload_data;
    (void)upload_data_size;
    (void)con_cls;

    if (strcmp(method, "GET") != 0)
        return send_response(connection, "Method Not Allowed", MHD_HTTP_METHOD_NOT_ALLOWED, "text/plain");

    if (strcmp(url, "/") == 0 || strcmp(url, "/index.html") == 0 ||
        strcmp(url, "/style.css") == 0 || strcmp(url, "/script.js") == 0) {
        return serve_static(connection, url);
    }

    if (strcmp(url, "/api/images") == 0) {
        const char *repo = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "repo");
        const char *query_path = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "path");
        const char *recursive_q = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "recursive");
        const char *page_q = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "page");
        const char *per_page_q = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "per_page");
        int recursive = (recursive_q && strcmp(recursive_q, "0") == 0) ? 0 : 1;
        int page = 1;
        int per_page = 50;

        if (page_q && *page_q)
            page = atoi(page_q);
        if (per_page_q && *per_page_q)
            per_page = atoi(per_page_q);

        if (!repo || strlen(repo) == 0)
            return send_response(connection, "Missing repo parameter", MHD_HTTP_BAD_REQUEST, "text/plain");

        char owner[256] = {0};
        char repository[256] = {0};
        char ref[256] = {0};
        char path[1024] = {0};
        if (!parse_github_repo(repo, owner, sizeof(owner), repository, sizeof(repository),
                               ref, sizeof(ref), path, sizeof(path))) {
            return send_response(connection, "Invalid GitHub repo URL", MHD_HTTP_BAD_REQUEST, "text/plain");
        }

        const char *effective_path = path;
        if (query_path && *query_path)
            effective_path = query_path;

        char *result = github_fetch_image_list(owner, repository, effective_path, ref,
                                               recursive, page, per_page);
        if (!result)
            return send_response(connection, "Error fetching from GitHub", MHD_HTTP_INTERNAL_SERVER_ERROR, "text/plain");

        int ret = send_response(connection, result, MHD_HTTP_OK, "application/json");
        free(result);
        return ret;
    }

    return send_response(connection, "Not Found", MHD_HTTP_NOT_FOUND, "text/plain");
}
