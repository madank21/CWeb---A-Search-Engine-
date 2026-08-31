#include "ranking.h"
#include "config/config.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SearchResultList *search_result_list_create(size_t initial_cap) {
    if (initial_cap == 0) initial_cap = 16;
    SearchResultList *list = (SearchResultList *)calloc(1, sizeof(SearchResultList));
    if (!list) return NULL;
    list->results = (SearchResult *)calloc(initial_cap, sizeof(SearchResult));
    list->capacity = initial_cap;
    list->count = 0;
    list->search_time_ms = 0.0;
    return list;
}

void search_result_list_free(SearchResultList *list) {
    if (!list) return;
    free(list->results);
    free(list);
}

float calculate_tfidf(float wtf, size_t df, size_t total_docs) {
    if (wtf <= 0.0f) return 0.0f;
    float tf = 1.0f + (float)log(wtf);
    float idf = (float)log(((double)total_docs + 1.0) / ((double)df + 1.0)) + 1.0f;
    return tf * idf;
}

float calculate_bm25(float wtf, size_t df, size_t total_docs, int doc_len, double avgdl, float k1, float b) {
    if (wtf <= 0.0f) return 0.0f;
    if (avgdl <= 0.0) avgdl = 1.0;

    double idf = log(((double)total_docs - (double)df + 0.5) / ((double)df + 0.5) + 1.0);
    if (idf < 0.0) idf = 0.0;

    double num = (double)wtf * ((double)k1 + 1.0);
    double den = (double)wtf + (double)k1 * (1.0 - (double)b + (double)b * ((double)doc_len / avgdl));

    return (float)(idf * (num / den));
}

static int compare_search_results(const void *a, const void *b) {
    const SearchResult *ra = (const SearchResult *)a;
    const SearchResult *rb = (const SearchResult *)b;

    /* Score comparison (higher is better) */
    float diff = rb->score - ra->score;
    if (fabsf(diff) > 1e-6f) {
        return (diff > 0.0f) ? 1 : -1;
    }

    /* Tie-breaker 1: term coverage */
    if (rb->matched_terms != ra->matched_terms) {
        return rb->matched_terms - ra->matched_terms;
    }

    /* Tie-breaker 2: modified_time (newer is better) */
    long time_a = ra->doc ? ra->doc->modified_time : 0;
    long time_b = rb->doc ? rb->doc->modified_time : 0;
    if (time_b != time_a) {
        return (time_b > time_a) ? 1 : -1;
    }

    /* Tie-breaker 3: lower document_id */
    return ra->document_id - rb->document_id;
}

void sort_search_results(SearchResultList *list) {
    if (!list || list->count <= 1) return;
    qsort(list->results, list->count, sizeof(SearchResult), compare_search_results);
}
