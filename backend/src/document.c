#include "document.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Document *document_create(void) {
    Document *doc = (Document *)calloc(1, sizeof(Document));
    if (!doc) return NULL;
    doc->id = -1;
    return doc;
}

void string_array_free(char **arr) {
    if (!arr) return;
    for (size_t i = 0; arr[i] != NULL; i++) {
        free(arr[i]);
    }
    free(arr);
}

char **string_array_copy(char **arr) {
    if (!arr) return NULL;
    size_t count = 0;
    while (arr[count] != NULL) count++;

    char **copy = (char **)calloc(count + 1, sizeof(char *));
    if (!copy) return NULL;

    for (size_t i = 0; i < count; i++) {
        copy[i] = strdup(arr[i]);
    }
    copy[count] = NULL;
    return copy;
}

void document_free(Document *doc) {
    if (!doc) return;
    free(doc->path);
    free(doc->title);
    free(doc->description);
    free(doc->category);
    string_array_free(doc->keywords);
    string_array_free(doc->headings);
    string_array_free(doc->links);
    free(doc->body_text);
    free(doc);
}

DocumentStore *document_store_create(size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 16;
    DocumentStore *store = (DocumentStore *)calloc(1, sizeof(DocumentStore));
    if (!store) return NULL;

    store->docs = (Document **)calloc(initial_capacity, sizeof(Document *));
    if (!store->docs) {
        free(store);
        return NULL;
    }
    store->count = 0;
    store->capacity = initial_capacity;
    return store;
}

void document_store_free(DocumentStore *store) {
    if (!store) return;
    for (size_t i = 0; i < store->count; i++) {
        document_free(store->docs[i]);
    }
    free(store->docs);
    free(store);
}

int document_store_add(DocumentStore *store, Document *doc) {
    if (!store || !doc) return -1;
    
    /* Check for duplicate ID or replace existing */
    for (size_t i = 0; i < store->count; i++) {
        if (store->docs[i]->id == doc->id) {
            document_free(store->docs[i]);
            store->docs[i] = doc;
            return 0;
        }
    }

    if (store->count >= store->capacity) {
        size_t new_cap = store->capacity * 2;
        Document **new_docs = (Document **)realloc(store->docs, new_cap * sizeof(Document *));
        if (!new_docs) return -1;
        store->docs = new_docs;
        store->capacity = new_cap;
    }

    store->docs[store->count++] = doc;
    return 0;
}

Document *document_store_get_by_id(const DocumentStore *store, int id) {
    if (!store) return NULL;
    for (size_t i = 0; i < store->count; i++) {
        if (store->docs[i]->id == id) {
            return store->docs[i];
        }
    }
    return NULL;
}
