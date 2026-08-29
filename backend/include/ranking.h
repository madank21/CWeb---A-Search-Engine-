#ifndef CWEB_RANKING_H
#define CWEB_RANKING_H

#include "document.h"
#include "index.h"
#include <stddef.h>

typedef enum {
    RANKING_BM25,
    RANKING_TFIDF
} RankingAlgorithm;

typedef struct {
    int   document_id;
    float score;
    int   matched_terms;
    int   total_query_terms;
    Document *doc;
} SearchResult;

typedef struct {
    SearchResult *results;
    size_t        count;
    size_t        capacity;
    double        search_time_ms;
} SearchResultList;

SearchResultList *search_result_list_create(size_t initial_cap);
void search_result_list_free(SearchResultList *list);

float calculate_tfidf(float wtf, size_t df, size_t total_docs);
float calculate_bm25(float wtf, size_t df, size_t total_docs, int doc_len, double avgdl, float k1, float b);

void sort_search_results(SearchResultList *list);

#endif /* CWEB_RANKING_H */
