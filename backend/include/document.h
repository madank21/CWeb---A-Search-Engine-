#ifndef CWEB_DOCUMENT_H
#define CWEB_DOCUMENT_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

typedef struct {
    int      id;
    char    *path;
    char    *title;
    char    *description;
    char    *category;
    char   **keywords;        /* NULL-terminated array of strings */
    char   **headings;        /* NULL-terminated array of strings */
    char   **links;           /* NULL-terminated array of strings */
    char    *body_text;       /* parsed, script/style/comment-stripped */
    long     file_size;
    long     modified_time;
    int      word_count;
    uint32_t doc_version;     /* bumped on re-index */
} Document;

typedef struct {
    Document **docs;
    size_t     count;
    size_t     capacity;
} DocumentStore;

Document *document_create(void);
void document_free(Document *doc);

DocumentStore *document_store_create(size_t initial_capacity);
void document_store_free(DocumentStore *store);
int document_store_add(DocumentStore *store, Document *doc);
Document *document_store_get_by_id(const DocumentStore *store, int id);

char **string_array_copy(char **arr);
void string_array_free(char **arr);

#endif /* CWEB_DOCUMENT_H */
