#include "index.h"
#include "config/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static InvertedIndex *g_active_index = NULL;
static pthread_rwlock_t g_swap_lock = PTHREAD_RWLOCK_INITIALIZER;

PostingList *posting_list_create(void) {
    PostingList *list = (PostingList *)calloc(1, sizeof(PostingList));
    if (!list) return NULL;
    list->capacity = 4;
    list->postings = (Posting *)calloc(list->capacity, sizeof(Posting));
    list->doc_freq = 0;
    return list;
}

void posting_list_free(PostingList *list) {
    if (!list) return;
    for (size_t i = 0; i < list->doc_freq; i++) {
        free(list->postings[i].positions);
    }
    free(list->postings);
    free(list);
}

static float field_to_weight(DocumentField field) {
    switch (field) {
        case FIELD_TITLE: return CWEB_WEIGHT_TITLE;
        case FIELD_HEADING: return CWEB_WEIGHT_HEADING;
        case FIELD_KEYWORDS: return CWEB_WEIGHT_KEYWORDS;
        case FIELD_DESCRIPTION: return CWEB_WEIGHT_DESCRIPTION;
        case FIELD_BODY: return CWEB_WEIGHT_BODY;
        default: return 1.0f;
    }
}

InvertedIndex *index_create(void) {
    InvertedIndex *idx = (InvertedIndex *)calloc(1, sizeof(InvertedIndex));
    if (!idx) return NULL;

    idx->term_table = hash_table_create(4096);
    idx->doc_store = document_store_create(64);
    idx->trie = trie_create();
    idx->total_documents = 0;
    idx->total_terms = 0;
    idx->avg_doc_length = 0.0;
    idx->ref_count = 1;
    pthread_rwlock_init(&idx->lock, NULL);
    return idx;
}

static void posting_list_free_wrapper(void *ptr) {
    posting_list_free((PostingList *)ptr);
}

void index_free(InvertedIndex *idx) {
    if (!idx) return;
    pthread_rwlock_destroy(&idx->lock);
    hash_table_free(idx->term_table, posting_list_free_wrapper);
    document_store_free(idx->doc_store);
    trie_free(idx->trie);
    free(idx);
}

int index_add_document(InvertedIndex *idx, Document *doc) {
    if (!idx || !doc) return -1;

    pthread_rwlock_wrlock(&idx->lock);

    document_store_add(idx->doc_store, doc);

    TokenStream *ts = tokenize_document(doc);
    if (!ts) {
        pthread_rwlock_unlock(&idx->lock);
        return -1;
    }

    doc->word_count = (int)ts->count;

    for (size_t i = 0; i < ts->count; i++) {
        const char *term = ts->tokens[i].text;
        int pos = ts->tokens[i].position;
        DocumentField fld = ts->tokens[i].field;
        float weight = field_to_weight(fld);

        if (tokenizer_is_stopword(term)) {
            continue;
        }

        PostingList *plist = (PostingList *)hash_table_get(idx->term_table, term);
        if (!plist) {
            plist = posting_list_create();
            hash_table_put(idx->term_table, term, plist);
            idx->total_terms++;
        }

        /* Check if posting for this doc already exists */
        Posting *post = NULL;
        if (plist->doc_freq > 0 && plist->postings[plist->doc_freq - 1].document_id == doc->id) {
            post = &plist->postings[plist->doc_freq - 1];
        } else {
            for (size_t k = 0; k < plist->doc_freq; k++) {
                if (plist->postings[k].document_id == doc->id) {
                    post = &plist->postings[k];
                    break;
                }
            }
        }

        if (!post) {
            if (plist->doc_freq >= plist->capacity) {
                size_t new_cap = plist->capacity * 2;
                Posting *new_postings = (Posting *)realloc(plist->postings, new_cap * sizeof(Posting));
                if (!new_postings) {
                    token_stream_free(ts);
                    pthread_rwlock_unlock(&idx->lock);
                    return -1;
                }
                plist->postings = new_postings;
                plist->capacity = new_cap;
            }
            post = &plist->postings[plist->doc_freq++];
            post->document_id = doc->id;
            post->frequency = 0;
            post->positions = NULL;
            post->position_count = 0;
            post->field_weight_sum = 0.0f;
        }

        post->frequency++;
        post->field_weight_sum += weight;

        /* Append position */
        int *new_pos = (int *)realloc(post->positions, (post->position_count + 1) * sizeof(int));
        if (new_pos) {
            post->positions = new_pos;
            post->positions[post->position_count++] = pos;
        }

        trie_insert(idx->trie, term, 1);
    }

    token_stream_free(ts);

    idx->total_documents = idx->doc_store->count;
    
    /* Update average document length */
    double total_len = 0.0;
    for (size_t i = 0; i < idx->doc_store->count; i++) {
        total_len += idx->doc_store->docs[i]->word_count;
    }
    idx->avg_doc_length = (idx->total_documents > 0) ? (total_len / idx->total_documents) : 0.0;

    pthread_rwlock_unlock(&idx->lock);
    return 0;
}

int index_remove_document(InvertedIndex *idx, int document_id) {
    if (!idx) return -1;
    pthread_rwlock_wrlock(&idx->lock);

    /* Remove postings for document_id across all terms */
    for (size_t i = 0; i < idx->term_table->num_buckets; i++) {
        HashNode *node = idx->term_table->buckets[i];
        while (node) {
            PostingList *plist = (PostingList *)node->value;
            if (plist) {
                for (size_t k = 0; k < plist->doc_freq; k++) {
                    if (plist->postings[k].document_id == document_id) {
                        free(plist->postings[k].positions);
                        for (size_t m = k; m + 1 < plist->doc_freq; m++) {
                            plist->postings[m] = plist->postings[m + 1];
                        }
                        plist->doc_freq--;
                        break;
                    }
                }
            }
            node = node->next;
        }
    }

    pthread_rwlock_unlock(&idx->lock);
    return 0;
}

PostingList *index_get_posting_list(const InvertedIndex *idx, const char *term) {
    if (!idx || !term) return NULL;
    return (PostingList *)hash_table_get(idx->term_table, term);
}

/* RCU Global Pointer Management */
InvertedIndex *index_get_active_instance(void) {
    pthread_rwlock_rdlock(&g_swap_lock);
    InvertedIndex *idx = g_active_index;
    if (idx) {
        idx->ref_count++;
    }
    pthread_rwlock_unlock(&g_swap_lock);
    return idx;
}

void index_release_instance(InvertedIndex *idx) {
    if (!idx) return;
    pthread_rwlock_wrlock(&g_swap_lock);
    idx->ref_count--;
    int free_needed = (idx != g_active_index && idx->ref_count <= 0);
    pthread_rwlock_unlock(&g_swap_lock);

    if (free_needed) {
        index_free(idx);
    }
}

int index_atomic_swap(InvertedIndex *new_idx) {
    if (!new_idx) return -1;
    pthread_rwlock_wrlock(&g_swap_lock);
    InvertedIndex *old = g_active_index;
    g_active_index = new_idx;
    if (old) {
        old->ref_count--;
        if (old->ref_count <= 0) {
            index_free(old);
        }
    }
    pthread_rwlock_unlock(&g_swap_lock);
    return 0;
}
