#ifndef CWEB_INDEX_H
#define CWEB_INDEX_H

#include "document.h"
#include "hash_table.h"
#include "tokenizer.h"
#include "trie.h"
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int   document_id;
    int   frequency;
    int  *positions;
    int   position_count;
    float field_weight_sum;
} Posting;

typedef struct {
    Posting *postings;
    size_t   doc_freq;
    size_t   capacity;
} PostingList;

typedef struct {
    HashTable     *term_table;   /* term -> PostingList* */
    DocumentStore *doc_store;
    Trie          *trie;
    size_t         total_documents;
    size_t         total_terms;
    double         avg_doc_length;
    int            ref_count;    /* for RCU reader tracking */
    pthread_rwlock_t lock;
} InvertedIndex;

InvertedIndex *index_create(void);
void index_free(InvertedIndex *idx);

int index_add_document(InvertedIndex *idx, Document *doc);
int index_remove_document(InvertedIndex *idx, int document_id);
PostingList *index_get_posting_list(const InvertedIndex *idx, const char *term);

PostingList *posting_list_create(void);
void posting_list_free(PostingList *list);

/* RCU Pointer Swap API */
InvertedIndex *index_get_active_instance(void);
void index_release_instance(InvertedIndex *idx);
int index_atomic_swap(InvertedIndex *new_idx);

#endif /* CWEB_INDEX_H */
