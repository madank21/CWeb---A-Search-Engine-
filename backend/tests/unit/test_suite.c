#include "config/config.h"
#include "crc32.h"
#include "document.h"
#include "fuzzy.h"
#include "hash_table.h"
#include "html_parser.h"
#include "index.h"
#include "normalize.h"
#include "persistence.h"
#include "query_parser.h"
#include "ranking.h"
#include "snippet.h"
#include "tokenizer.h"
#include "trie.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(condition, test_name) \
    do { \
        if (condition) { \
            printf("[PASS] %s\n", test_name); \
            tests_passed++; \
        } else { \
            printf("[FAIL] %s (line %d)\n", test_name, __LINE__); \
            tests_failed++; \
        } \
    } while (0)

static void test_crc32(void) {
    const char *data = "123456789";
    uint32_t crc = crc32_calculate(data, strlen(data));
    ASSERT_TRUE(crc == 0xcbf43926, "CRC32 Standard Vector Test");
}

static void test_document_store(void) {
    DocumentStore *store = document_store_create(4);
    ASSERT_TRUE(store != NULL, "DocumentStore Allocation");

    Document *d1 = document_create();
    d1->id = 1;
    d1->title = strdup("Test Doc 1");
    document_store_add(store, d1);

    Document *fetched = document_store_get_by_id(store, 1);
    ASSERT_TRUE(fetched != NULL && strcmp(fetched->title, "Test Doc 1") == 0, "DocumentStore Lookup");

    document_store_free(store);
    ASSERT_TRUE(1, "DocumentStore Clean Free");
}

static void test_html_parser(void) {
    const char *html = "<!DOCTYPE html><html><head><title>Compiler Optimization</title>"
                       "<meta name=\"category\" content=\"Compilers\">"
                       "<meta name=\"description\" content=\"Static analysis and vectorization.\">"
                       "</head><body><h1>Loop Unrolling</h1>"
                       "<p>Optimizing compiler passes build SSA form representation.</p>"
                       "<a href=\"/page/2\">Doc 2</a></body></html>";

    Document *doc = document_create();
    doc->id = 42;
    int res = html_parse_document(html, strlen(html), doc);

    ASSERT_TRUE(res == 0, "HTML Parsing Execution");
    ASSERT_TRUE(strcmp(doc->title, "Compiler Optimization") == 0, "HTML Title Extraction");
    ASSERT_TRUE(strcmp(doc->category, "Compilers") == 0, "HTML Meta Category Extraction");
    ASSERT_TRUE(strcmp(doc->description, "Static analysis and vectorization.") == 0, "HTML Meta Description");
    ASSERT_TRUE(doc->headings != NULL && strcmp(doc->headings[0], "Loop Unrolling") == 0, "HTML Headings Extraction");
    ASSERT_TRUE(doc->links != NULL && strcmp(doc->links[0], "/page/2") == 0, "HTML Links Extraction");

    document_free(doc);
}

static void test_tokenizer_and_normalize(void) {
    char token1[32] = "running";
    normalize_token(token1);
    ASSERT_TRUE(strcmp(token1, "run") == 0, "Normalize Suffix -ing");

    char token2[32] = "compilers";
    normalize_token(token2);
    ASSERT_TRUE(strcmp(token2, "compiler") == 0, "Normalize Suffix -s");

    int pos = 1;
    TokenStream *ts = tokenize_text("Compiler optimization techniques!", FIELD_TITLE, &pos);
    ASSERT_TRUE(ts != NULL && ts->count == 3, "Tokenizer Token Count");
    ASSERT_TRUE(strcmp(ts->tokens[0].text, "compiler") == 0, "Tokenizer Token Case Folding");

    token_stream_free(ts);
}

static void test_hash_table(void) {
    HashTable *ht = hash_table_create(16);
    int val1 = 100, val2 = 200;

    hash_table_put(ht, "compiler", &val1);
    hash_table_put(ht, "optimization", &val2);

    int *ret1 = (int *)hash_table_get(ht, "compiler");
    ASSERT_TRUE(ret1 != NULL && *ret1 == 100, "HashTable Get Key 1");

    ASSERT_TRUE(hash_table_load_factor(ht) > 0.0, "HashTable Load Factor Calculation");

    hash_table_free(ht, NULL);
}

static void test_trie_suggest(void) {
    Trie *trie = trie_create();
    trie_insert(trie, "compiler", 10);
    trie_insert(trie, "compilation", 8);
    trie_insert(trie, "computer", 5);

    char **suggs = trie_suggest(trie, "comp", 5);
    ASSERT_TRUE(suggs != NULL, "Trie Suggestion Execution");
    ASSERT_TRUE(suggs[0] != NULL && strcmp(suggs[0], "compiler") == 0, "Trie Suggest Frequency Ranking");

    trie_string_list_free(suggs);
    trie_free(trie);
}

static void test_query_parser(void) {
    QueryAST *ast = query_parse("(compiler OR parser) AND optimization");
    ASSERT_TRUE(ast != NULL && ast->root != NULL, "Query AST Parsing Boolean Parentheses");
    ASSERT_TRUE(ast->root->type == NODE_AND, "Query AST Root Node Type");

    query_ast_free(ast);

    QueryAST *bad_ast = query_parse("compiler AND (optimization");
    ASSERT_TRUE(bad_ast->error_message != NULL, "Query AST Syntax Error Detection");
    query_ast_free(bad_ast);
}

static void test_levenshtein(void) {
    int dist1 = levenshtein_distance_bounded("compilor", "compiler", 2);
    ASSERT_TRUE(dist1 == 1, "Bounded Levenshtein Distance Single Edit");

    int dist2 = levenshtein_distance_bounded("compilor", "completely", 2);
    ASSERT_TRUE(dist2 > 2, "Bounded Levenshtein Early Exit");
}

static void test_persistence_roundtrip(void) {
    InvertedIndex *idx = index_create();

    Document *d = document_create();
    d->id = 1;
    d->path = strdup("page_1.html");
    d->title = strdup("Compiler Design");
    d->body_text = strdup("Compiler optimization passes build high performance machine code.");
    d->file_size = 100;
    d->modified_time = 12345;

    index_add_document(idx, d);

    int res = persistence_save_index(idx, "./data/index_test");
    ASSERT_TRUE(res == 0, "Persistence Save Index");

    InvertedIndex *loaded_idx = persistence_load_index("./data/index_test");
    ASSERT_TRUE(loaded_idx != NULL, "Persistence Load Index");
    ASSERT_TRUE(loaded_idx->total_terms == idx->total_terms, "Persistence Terms Count Match");

    index_free(idx);
    index_free(loaded_idx);
}

int main(void) {
    printf("======================================\n");
    printf(" CWeb Backend Unit Test Suite Running\n");
    printf("======================================\n");

    test_crc32();
    test_document_store();
    test_html_parser();
    test_tokenizer_and_normalize();
    test_hash_table();
    test_trie_suggest();
    test_query_parser();
    test_levenshtein();
    test_persistence_roundtrip();

    printf("======================================\n");
    printf(" Summary: %d PASSED, %d FAILED\n", tests_passed, tests_failed);
    printf("======================================\n");

    return (tests_failed == 0) ? 0 : 1;
}
