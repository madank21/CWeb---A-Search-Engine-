#include "http_server.h"
#include "config/config.h"
#include "html_parser.h"
#include "logger.h"
#include "persistence.h"
#include "query_parser.h"
#include "ranking.h"
#include "snippet.h"
#include "tokenizer.h"
#include "trie.h"

#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
typedef SOCKET socket_t;
#define close_socket(s) closesocket(s)
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int socket_t;
#define close_socket(s) close(s)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

/* Atomic build flag */
static _Atomic bool g_index_building = false;
static time_t g_server_start_time = 0;
static bool g_server_running = false;
static socket_t g_server_listen_fd = INVALID_SOCKET;

/* Config references */
static char g_pages_dir[256];
static char g_index_dir[256];

/* LRU Cache */
typedef struct CacheNode {
    char             *key;
    char             *json_response;
    struct CacheNode *prev;
    struct CacheNode *next;
} CacheNode;

typedef struct {
    CacheNode       *head;
    CacheNode       *tail;
    size_t           size;
    size_t           capacity;
    size_t           hits;
    size_t           misses;
    pthread_mutex_t  lock;
} LRUCache;

static LRUCache g_cache;

static void cache_init(size_t capacity) {
    g_cache.head = NULL;
    g_cache.tail = NULL;
    g_cache.size = 0;
    g_cache.capacity = capacity;
    g_cache.hits = 0;
    g_cache.misses = 0;
    pthread_mutex_init(&g_cache.lock, NULL);
}

static char *cache_get(const char *key) {
    pthread_mutex_lock(&g_cache.lock);
    CacheNode *node = g_cache.head;
    while (node) {
        if (strcmp(node->key, key) == 0) {
            /* Move to front */
            if (node != g_cache.head) {
                if (node == g_cache.tail) {
                    g_cache.tail = node->prev;
                    if (g_cache.tail) g_cache.tail->next = NULL;
                } else {
                    node->prev->next = node->next;
                    node->next->prev = node->prev;
                }
                node->next = g_cache.head;
                node->prev = NULL;
                g_cache.head->prev = node;
                g_cache.head = node;
            }
            g_cache.hits++;
            char *copy = strdup(node->json_response);
            pthread_mutex_unlock(&g_cache.lock);
            return copy;
        }
        node = node->next;
    }
    g_cache.misses++;
    pthread_mutex_unlock(&g_cache.lock);
    return NULL;
}

static void cache_put(const char *key, const char *json_response) {
    pthread_mutex_lock(&g_cache.lock);

    /* Check if already present */
    CacheNode *node = g_cache.head;
    while (node) {
        if (strcmp(node->key, key) == 0) {
            free(node->json_response);
            node->json_response = strdup(json_response);
            pthread_mutex_unlock(&g_cache.lock);
            return;
        }
        node = node->next;
    }

    if (g_cache.size >= g_cache.capacity && g_cache.tail) {
        CacheNode *evict = g_cache.tail;
        if (evict->prev) {
            g_cache.tail = evict->prev;
            g_cache.tail->next = NULL;
        } else {
            g_cache.head = NULL;
            g_cache.tail = NULL;
        }
        free(evict->key);
        free(evict->json_response);
        free(evict);
        g_cache.size--;
    }

    CacheNode *new_node = (CacheNode *)calloc(1, sizeof(CacheNode));
    new_node->key = strdup(key);
    new_node->json_response = strdup(json_response);
    new_node->next = g_cache.head;
    if (g_cache.head) g_cache.head->prev = new_node;
    g_cache.head = new_node;
    if (!g_cache.tail) g_cache.tail = new_node;
    g_cache.size++;

    pthread_mutex_unlock(&g_cache.lock);
}

static void cache_clear(void) {
    pthread_mutex_lock(&g_cache.lock);
    CacheNode *node = g_cache.head;
    while (node) {
        CacheNode *tmp = node;
        node = node->next;
        free(tmp->key);
        free(tmp->json_response);
        free(tmp);
    }
    g_cache.head = NULL;
    g_cache.tail = NULL;
    g_cache.size = 0;
    pthread_mutex_unlock(&g_cache.lock);
}

/* Rate Limiter Token Bucket */
typedef struct RateBucket {
    char              ip[64];
    double            tokens;
    time_t            last_update;
    struct RateBucket *next;
} RateBucket;

static RateBucket *g_rate_buckets = NULL;
static pthread_mutex_t g_rate_lock = PTHREAD_MUTEX_INITIALIZER;

static int check_rate_limit(const char *ip, int *out_remaining) {
    pthread_mutex_lock(&g_rate_lock);
    time_t now = time(NULL);

    RateBucket *b = g_rate_buckets;
    while (b) {
        if (strcmp(b->ip, ip) == 0) break;
        b = b->next;
    }

    if (!b) {
        b = (RateBucket *)calloc(1, sizeof(RateBucket));
        snprintf(b->ip, sizeof(b->ip), "%s", ip);
        b->tokens = CWEB_RATE_LIMIT_BURST;
        b->last_update = now;
        b->next = g_rate_buckets;
        g_rate_buckets = b;
    }

    double elapsed = difftime(now, b->last_update);
    b->tokens += elapsed * CWEB_RATE_LIMIT_RPS;
    if (b->tokens > CWEB_RATE_LIMIT_BURST) {
        b->tokens = CWEB_RATE_LIMIT_BURST;
    }
    b->last_update = now;

    if (b->tokens >= 1.0) {
        b->tokens -= 1.0;
        if (out_remaining) *out_remaining = (int)b->tokens;
        pthread_mutex_unlock(&g_rate_lock);
        return 1; /* Allowed */
    }

    if (out_remaining) *out_remaining = 0;
    pthread_mutex_unlock(&g_rate_lock);
    return 0; /* Rate limited */
}

/* URL decoding helper */
static void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit((unsigned char)a) && isxdigit((unsigned char)b))) {
            if (a >= 'a' && a <= 'f') a = a - 'a' + 'A';
            if (a >= 'A' && a <= 'F') a = a - 'A' + 10;
            else a = a - '0';
            if (b >= 'a' && b <= 'f') b = b - 'a' + 'A';
            if (b >= 'A' && b <= 'F') b = b - 'A' + 10;
            else b = b - '0';
            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

/* HTTP Helper Response Writer */
static void send_http_response(socket_t client_fd, int status_code, const char *status_text, const char *json_body, int rate_remaining) {
    char header[1024];
    size_t body_len = json_body ? strlen(json_body) : 0;

    snprintf(header, sizeof(header),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: application/json; charset=utf-8\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type, X-Client-ID\r\n"
             "Access-Control-Expose-Headers: X-RateLimit-Limit, X-RateLimit-Remaining, X-Server-Engine\r\n"
             "X-Server-Engine: CWeb/2.0-C17\r\n"
             "X-RateLimit-Limit: %d\r\n"
             "X-RateLimit-Remaining: %d\r\n"
             "X-RateLimit-Reset: 1\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n",
             status_code, status_text, CWEB_RATE_LIMIT_RPS, rate_remaining, body_len);

    send(client_fd, header, (int)strlen(header), 0);
    if (body_len > 0) {
        send(client_fd, json_body, (int)body_len, 0);
    }
}

/* Helper to escape strings according to RFC 8259 JSON specifications */
static char *json_escape_string(const char *src) {
    if (!src) return strdup("");
    size_t len = strlen(src);
    /* Allocate enough space for worst case (\u00XX per char) + 1 null terminator */
    size_t cap = len * 6 + 1;
    char *dst = (char *)malloc(cap);
    if (!dst) return strdup("");

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)src[i];
        switch (c) {
            case '"':  dst[j++] = '\\'; dst[j++] = '"'; break;
            case '\\': dst[j++] = '\\'; dst[j++] = '\\'; break;
            case '\b': dst[j++] = '\\'; dst[j++] = 'b'; break;
            case '\f': dst[j++] = '\\'; dst[j++] = 'f'; break;
            case '\n': dst[j++] = '\\'; dst[j++] = 'n'; break;
            case '\r': dst[j++] = '\\'; dst[j++] = 'r'; break;
            case '\t': dst[j++] = '\\'; dst[j++] = 't'; break;
            default:
                if (c < 0x20) {
                    j += (size_t)snprintf(&dst[j], 7, "\\u%04x", c);
                } else {
                    dst[j++] = c;
                }
                break;
        }
    }
    dst[j] = '\0';
    return dst;
}

static void send_json_error(socket_t client_fd, int status_code, const char *code_str, const char *message, int rate_remaining) {
    char *esc_msg = json_escape_string(message);
    char json[1024];
    snprintf(json, sizeof(json),
             "{\"error\":true,\"code\":\"%s\",\"message\":\"%s\",\"status\":%d}",
             code_str ? code_str : "ERROR", esc_msg ? esc_msg : "", status_code);
    free(esc_msg);
    send_http_response(client_fd, status_code, (status_code == 400 ? "Bad Request" : status_code == 404 ? "Not Found" : status_code == 409 ? "Conflict" : status_code == 429 ? "Too Many Requests" : "Internal Error"), json, rate_remaining);
}

/* Request Dispatcher */
static void handle_client_connection(socket_t client_fd, const char *client_ip) {
    int rate_remaining = 0;
    if (!check_rate_limit(client_ip, &rate_remaining)) {
        send_json_error(client_fd, 429, "RATE_LIMITED", "Too many requests. Rate limit exceeded.", 0);
        close_socket(client_fd);
        return;
    }

    char request_buf[CWEB_MAX_HEADER_BYTES];
    int bytes_read = recv(client_fd, request_buf, sizeof(request_buf) - 1, 0);
    if (bytes_read <= 0) {
        close_socket(client_fd);
        return;
    }
    request_buf[bytes_read] = '\0';

    /* Parse request line */
    char method[16], path[1024], protocol[16];
    if (sscanf(request_buf, "%15s %1023s %15s", method, path, protocol) != 3) {
        send_json_error(client_fd, 400, "MALFORMED_REQUEST", "Unparsable HTTP request line", rate_remaining);
        close_socket(client_fd);
        return;
    }

    /* Handle CORS Preflight OPTIONS */
    if (strcmp(method, "OPTIONS") == 0) {
        send_http_response(client_fd, 200, "OK", "", rate_remaining);
        close_socket(client_fd);
        return;
    }

    /* Endpoint Router */

    /* 0. GET / (Root Welcome Endpoint) */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/") == 0) {
        const char *json = "{\"service\":\"CWeb Search Engine REST API\",\"version\":\"" CWEB_VERSION "\",\"status\":\"online\",\"endpoints\":[\"/health\",\"/stats\",\"/search?q=\",\"/suggest?q=\",\"/page/:id\"]}";
        send_http_response(client_fd, 200, "OK", json, rate_remaining);
        close_socket(client_fd);
        return;
    }

    /* 1. GET /api/v1/health */
    if (strcmp(method, "GET") == 0 && (strcmp(path, "/api/v1/health") == 0 || strcmp(path, "/health") == 0)) {
        InvertedIndex *idx = index_get_active_instance();
        int loaded = (idx && idx->total_documents > 0);
        size_t doc_count = idx ? idx->total_documents : 0;
        index_release_instance(idx);

        time_t uptime = time(NULL) - g_server_start_time;

        char json[256];
        snprintf(json, sizeof(json),
                 "{\"status\":\"ok\",\"version\":\"" CWEB_VERSION "\",\"index_loaded\":%s,\"documents\":%zu,\"uptime_seconds\":%ld}",
                 loaded ? "true" : "false", doc_count, (long)uptime);
        send_http_response(client_fd, 200, "OK", json, rate_remaining);
        close_socket(client_fd);
        return;
    }

    /* 2. GET /api/v1/stats */
    if (strcmp(method, "GET") == 0 && (strcmp(path, "/api/v1/stats") == 0 || strcmp(path, "/stats") == 0)) {
        InvertedIndex *idx = index_get_active_instance();
        size_t doc_count = idx ? idx->total_documents : 0;
        size_t term_count = idx ? idx->total_terms : 0;
        double load_factor = idx && idx->term_table ? hash_table_load_factor(idx->term_table) : 0.0;
        size_t collisions = idx && idx->term_table ? hash_table_collisions(idx->term_table) : 0;
        index_release_instance(idx);

        pthread_mutex_lock(&g_cache.lock);
        size_t cache_hits = g_cache.hits;
        size_t cache_misses = g_cache.misses;
        pthread_mutex_unlock(&g_cache.lock);

        time_t uptime = time(NULL) - g_server_start_time;

        char json[512];
        snprintf(json, sizeof(json),
                 "{\"documents_indexed\":%zu,\"unique_terms\":%zu,\"load_factor\":%.4f,\"hash_collisions\":%zu,\"cache_hits\":%zu,\"cache_misses\":%zu,\"uptime_seconds\":%ld}",
                 doc_count, term_count, load_factor, collisions, cache_hits, cache_misses, (long)uptime);
        send_http_response(client_fd, 200, "OK", json, rate_remaining);
        close_socket(client_fd);
        return;
    }

    /* 3. GET /api/v1/search?q=... */
    if (strcmp(method, "GET") == 0 && (strncmp(path, "/api/v1/search", 14) == 0 || strncmp(path, "/search", 7) == 0)) {
        /* Check Cache */
        char *cached = cache_get(path);
        if (cached) {
            send_http_response(client_fd, 200, "OK", cached, rate_remaining);
            free(cached);
            close_socket(client_fd);
            return;
        }

        /* Extract params */
        char q_param[256] = "";
        int page = 1;
        int page_size = CWEB_DEFAULT_PAGE_SIZE;
        RankingAlgorithm algo = RANKING_BM25;

        const char *query_start = strchr(path, '?');
        if (query_start) {
            query_start++;
            char *param_str = strdup(query_start);
            char *token = strtok(param_str, "&");
            while (token) {
                if (strncmp(token, "q=", 2) == 0) {
                    url_decode(q_param, token + 2);
                } else if (strncmp(token, "page=", 5) == 0) {
                    page = atoi(token + 5);
                } else if (strncmp(token, "page_size=", 10) == 0) {
                    page_size = atoi(token + 10);
                } else if (strncmp(token, "ranking=tfidf", 13) == 0) {
                    algo = RANKING_TFIDF;
                }
                token = strtok(NULL, "&");
            }
            free(param_str);
        }

        if (page < 1) page = 1;
        if (page_size < 1) page_size = 1;
        if (page_size > CWEB_MAX_RESULTS) page_size = CWEB_MAX_RESULTS;

        if (strlen(q_param) == 0) {
            send_json_error(client_fd, 400, "INVALID_QUERY_SYNTAX", "Query parameter 'q' is required.", rate_remaining);
            close_socket(client_fd);
            return;
        }

        /* Parse Query AST */
        QueryAST *ast = query_parse(q_param);
        if (ast->error_message) {
            send_json_error(client_fd, 400, "INVALID_QUERY_SYNTAX", ast->error_message, rate_remaining);
            query_ast_free(ast);
            close_socket(client_fd);
            return;
        }

        InvertedIndex *idx = index_get_active_instance();
        char *did_you_mean = NULL;

        clock_t t0 = clock();
        SearchResultList *results = query_evaluate(ast, idx, algo, CWEB_BM25_K1, CWEB_BM25_B, &did_you_mean);
        clock_t t1 = clock();

        double elapsed_ms = (double)(t1 - t0) * 1000.0 / CLOCKS_PER_SEC;

        /* Build JSON response */
        size_t total_results = results ? results->count : 0;
        size_t start_idx = (size_t)(page - 1) * (size_t)page_size;
        size_t end_idx = (start_idx + page_size < total_results) ? (start_idx + page_size) : total_results;

        size_t resp_cap = 8192;
        char *json = (char *)malloc(resp_cap);
        if (!json) {
            search_result_list_free(results);
            index_release_instance(idx);
            query_ast_free(ast);
            send_json_error(client_fd, 500, "INTERNAL_ERROR", "Memory allocation failed", rate_remaining);
            close_socket(client_fd);
            return;
        }

        char *esc_q = json_escape_string(q_param);
        char *esc_did_you_mean = did_you_mean ? json_escape_string(did_you_mean) : NULL;

        int written = snprintf(json, resp_cap,
                               "{\"query\":\"%s\",\"total\":%zu,\"page\":%d,\"page_size\":%d,\"ranking\":\"%s\",\"search_time_ms\":%.2f,\"did_you_mean\":%s%s%s,\"results\":[",
                               esc_q, total_results, page, page_size, (algo == RANKING_BM25 ? "bm25" : "tfidf"),
                               elapsed_ms, (esc_did_you_mean ? "\"" : ""), (esc_did_you_mean ? esc_did_you_mean : "null"), (esc_did_you_mean ? "\"" : ""));

        free(esc_q);
        free(esc_did_you_mean);

        for (size_t i = start_idx; i < end_idx; i++) {
            SearchResult *r = &results->results[i];
            char *snip = snippet_generate(r->doc, q_param, 150);

            char *esc_title = json_escape_string(r->doc ? r->doc->title : "Untitled");
            char *esc_cat = json_escape_string(r->doc ? r->doc->category : "General");
            char *esc_snip = json_escape_string(snip ? snip : "");

            size_t item_len = strlen(esc_title) + strlen(esc_cat) + strlen(esc_snip) + 512;
            char *item = (char *)malloc(item_len);
            if (item) {
                snprintf(item, item_len,
                         "%s{\"id\":%d,\"title\":\"%s\",\"url\":\"/page/%d\",\"category\":\"%s\",\"score\":%.4f,\"snippet\":\"%s\"}",
                         (i > start_idx ? "," : ""),
                         r->document_id,
                         esc_title,
                         r->document_id,
                         esc_cat,
                         r->score,
                         esc_snip);

                if ((size_t)written + strlen(item) + 16 > resp_cap) {
                    resp_cap = ((size_t)written + strlen(item) + 16) * 2;
                    json = (char *)realloc(json, resp_cap);
                }
                strcat(json, item);
                written += (int)strlen(item);
                free(item);
            }

            free(esc_title);
            free(esc_cat);
            free(esc_snip);
            free(snip);
        }

        strcat(json, "]}");

        /* Cache response */
        cache_put(path, json);

        send_http_response(client_fd, 200, "OK", json, rate_remaining);

        free(json);
        free(did_you_mean);
        search_result_list_free(results);
        index_release_instance(idx);
        query_ast_free(ast);
        close_socket(client_fd);
        return;
    }

    /* 4. GET /api/v1/suggest?q=... */
    if (strcmp(method, "GET") == 0 && (strncmp(path, "/api/v1/suggest", 15) == 0 || strncmp(path, "/suggest", 8) == 0)) {
        char prefix[128] = "";
        size_t limit = 5;

        const char *query_start = strchr(path, '?');
        if (query_start) {
            query_start++;
            char *param_str = strdup(query_start);
            char *token = strtok(param_str, "&");
            while (token) {
                if (strncmp(token, "q=", 2) == 0) {
                    url_decode(prefix, token + 2);
                } else if (strncmp(token, "limit=", 6) == 0) {
                    limit = atoi(token + 6);
                }
                token = strtok(NULL, "&");
            }
            free(param_str);
        }

        if (strlen(prefix) < 1) {
            send_json_error(client_fd, 400, "PREFIX_TOO_SHORT", "Prefix length must be at least 1 character.", rate_remaining);
            close_socket(client_fd);
            return;
        }

        InvertedIndex *idx = index_get_active_instance();
        char **suggestions = trie_suggest(idx ? idx->trie : NULL, prefix, limit);
        index_release_instance(idx);

        char json[2048] = "{\"suggestions\":[";
        if (suggestions) {
            for (size_t i = 0; suggestions[i] != NULL; i++) {
                char *esc_sug = json_escape_string(suggestions[i]);
                char buf[512];
                snprintf(buf, sizeof(buf), "%s\"%s\"", (i > 0 ? "," : ""), esc_sug);
                free(esc_sug);
                strcat(json, buf);
            }
            trie_string_list_free(suggestions);
        }
        strcat(json, "]}");

        send_http_response(client_fd, 200, "OK", json, rate_remaining);
        close_socket(client_fd);
        return;
    }

    /* 5. GET /api/v1/page/:id */
    if (strcmp(method, "GET") == 0 && (strncmp(path, "/api/v1/page/", 13) == 0 || strncmp(path, "/page/", 6) == 0)) {
        const char *id_str = strrchr(path, '/');
        if (id_str) id_str++;
        int id = id_str ? atoi(id_str) : -1;

        if (id <= 0) {
            send_json_error(client_fd, 400, "INVALID_ID", "Invalid or missing document ID", rate_remaining);
            close_socket(client_fd);
            return;
        }

        InvertedIndex *idx = index_get_active_instance();
        Document *doc = idx ? document_store_get_by_id(idx->doc_store, id) : NULL;

        if (!doc) {
            index_release_instance(idx);
            send_json_error(client_fd, 404, "DOCUMENT_NOT_FOUND", "No document found with specified ID", rate_remaining);
            close_socket(client_fd);
            return;
        }

        char *esc_title = json_escape_string(doc->title ? doc->title : "");
        char *esc_desc = json_escape_string(doc->description ? doc->description : "");
        char *esc_cat = json_escape_string(doc->category ? doc->category : "");
        char *esc_body = json_escape_string(doc->body_text ? doc->body_text : "");

        size_t cap = strlen(esc_title) + strlen(esc_desc) + strlen(esc_cat) + strlen(esc_body) + 2048;
        char *json = (char *)malloc(cap);
        if (json) {
            snprintf(json, cap,
                     "{\"id\":%d,\"title\":\"%s\",\"description\":\"%s\",\"category\":\"%s\",\"body_text\":\"%s\",\"word_count\":%d}",
                     doc->id, esc_title, esc_desc, esc_cat, esc_body, doc->word_count);
        }

        free(esc_title);
        free(esc_desc);
        free(esc_cat);
        free(esc_body);

        index_release_instance(idx);
        send_http_response(client_fd, 200, "OK", json ? json : "{}", rate_remaining);
        free(json);
        close_socket(client_fd);
        return;
    }

    /* 6. POST /api/v1/index/rebuild */
    if (strcmp(method, "POST") == 0 && (strcmp(path, "/api/v1/index/rebuild") == 0 || strcmp(path, "/index/rebuild") == 0 || strcmp(path, "/index") == 0)) {
        if (g_index_building) {
            send_json_error(client_fd, 409, "INDEX_BUILD_IN_PROGRESS", "Index rebuild already in progress.", rate_remaining);
            close_socket(client_fd);
            return;
        }

        g_index_building = true;
        cache_clear();

        /* Build new index */
        InvertedIndex *new_idx = index_create();
        for (int id = 1; id <= 50; id++) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/page_%d.html", g_pages_dir, id);

            FILE *fp = fopen(filepath, "r");
            if (fp) {
                fseek(fp, 0, SEEK_END);
                long len = ftell(fp);
                fseek(fp, 0, SEEK_SET);

                char *buf = (char *)malloc(len + 1);
                if (buf) {
                    fread(buf, 1, len, fp);
                    buf[len] = '\0';

                    Document *doc = document_create();
                    doc->id = id;
                    doc->path = strdup(filepath);
                    doc->file_size = len;
                    doc->modified_time = time(NULL);

                    html_parse_document(buf, len, doc);
                    index_add_document(new_idx, doc);
                    free(buf);
                }
                fclose(fp);
            }
        }

        index_atomic_swap(new_idx);
        persistence_save_index(new_idx, g_index_dir);
        g_index_building = false;

        const char *json = "{\"status\":\"success\",\"message\":\"Index rebuilt successfully.\"}";
        send_http_response(client_fd, 202, "Accepted", json, rate_remaining);
        close_socket(client_fd);
        return;
    }

    /* Fallthrough for undefined routes */
    send_json_error(client_fd, 404, "DOCUMENT_NOT_FOUND", "Endpoint route not found", rate_remaining);
    close_socket(client_fd);
}

/* Worker Thread Pool Function */
static void *worker_thread_func(void *arg) {
    (void)arg;
    while (g_server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        socket_t client_fd = accept(g_server_listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == INVALID_SOCKET) {
            if (!g_server_running) break;
            continue;
        }

        char client_ip[64] = "127.0.0.1";
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

        handle_client_connection(client_fd, client_ip);
    }
    return NULL;
}

int http_server_start(const HttpServerConfig *config) {
    if (!config) return -1;

    snprintf(g_pages_dir, sizeof(g_pages_dir), "%s", config->pages_dir ? config->pages_dir : CWEB_PAGES_DIR);
    snprintf(g_index_dir, sizeof(g_index_dir), "%s", config->index_dir ? config->index_dir : CWEB_INDEX_DIR);

    g_server_start_time = time(NULL);
    cache_init(CWEB_CACHE_SIZE);

#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return -1;
    }
#endif

    g_server_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_server_listen_fd == INVALID_SOCKET) {
        return -1;
    }

    int opt = 1;
    setsockopt(g_server_listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config->port ? config->port : CWEB_DEFAULT_PORT);
    addr.sin_addr.s_addr = inet_addr(config->host ? config->host : CWEB_DEFAULT_HOST);

    if (bind(g_server_listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        close_socket(g_server_listen_fd);
        return -1;
    }

    if (listen(g_server_listen_fd, 128) == SOCKET_ERROR) {
        close_socket(g_server_listen_fd);
        return -1;
    }

    g_server_running = true;

    /* Start Worker Threads */
    int num_threads = CWEB_THREAD_POOL_SIZE;
    pthread_t *threads = (pthread_t *)calloc(num_threads, sizeof(pthread_t));

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_thread_func, NULL);
    }

    log_json(LOG_LEVEL_INFO, "server_start", "system", "\"port\":8080");

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    return 0;
}

void http_server_stop(void) {
    g_server_running = false;
    if (g_server_listen_fd != INVALID_SOCKET) {
        close_socket(g_server_listen_fd);
        g_server_listen_fd = INVALID_SOCKET;
    }
#ifdef _WIN32
    WSACleanup();
#endif
}
