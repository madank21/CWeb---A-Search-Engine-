#include "trie.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TrieNode *trie_node_create(char ch) {
    TrieNode *node = (TrieNode *)calloc(1, sizeof(TrieNode));
    if (!node) return NULL;
    node->ch = ch;
    node->is_end_of_word = 0;
    node->frequency = 0;
    return node;
}

static void trie_node_free(TrieNode *node) {
    if (!node) return;
    for (int i = 0; i < 256; i++) {
        if (node->children[i]) {
            trie_node_free(node->children[i]);
        }
    }
    free(node);
}

Trie *trie_create(void) {
    Trie *trie = (Trie *)calloc(1, sizeof(Trie));
    if (!trie) return NULL;
    trie->root = trie_node_create('\0');
    trie->total_words = 0;
    return trie;
}

void trie_free(Trie *trie) {
    if (!trie) return;
    trie_node_free(trie->root);
    free(trie);
}

int trie_insert(Trie *trie, const char *word, int frequency) {
    if (!trie || !word || !*word) return -1;
    TrieNode *curr = trie->root;
    const unsigned char *p = (const unsigned char *)word;

    while (*p) {
        unsigned char idx = *p;
        if (!curr->children[idx]) {
            curr->children[idx] = trie_node_create((char)idx);
        }
        curr = curr->children[idx];
        p++;
    }

    if (!curr->is_end_of_word) {
        trie->total_words++;
    }
    curr->is_end_of_word = 1;
    curr->frequency += frequency;
    return 0;
}

int trie_search(const Trie *trie, const char *word) {
    if (!trie || !word) return 0;
    TrieNode *curr = trie->root;
    const unsigned char *p = (const unsigned char *)word;
    while (*p) {
        unsigned char idx = *p;
        if (!curr->children[idx]) return 0;
        curr = curr->children[idx];
        p++;
    }
    return curr->is_end_of_word ? curr->frequency : 0;
}

typedef struct {
    char *word;
    int frequency;
} SuggestItem;

static void collect_suggestions(TrieNode *node, char *buffer, int depth, SuggestItem *items, size_t *count, size_t cap) {
    if (!node) return;
    if (node->is_end_of_word) {
        buffer[depth] = '\0';
        if (*count < cap) {
            items[*count].word = strdup(buffer);
            items[*count].frequency = node->frequency;
            (*count)++;
        }
    }
    for (int i = 0; i < 256; i++) {
        if (node->children[i]) {
            buffer[depth] = node->children[i]->ch;
            collect_suggestions(node->children[i], buffer, depth + 1, items, count, cap);
        }
    }
}

static int compare_suggestions(const void *a, const void *b) {
    const SuggestItem *sa = (const SuggestItem *)a;
    const SuggestItem *sb = (const SuggestItem *)b;
    if (sb->frequency != sa->frequency) {
        return sb->frequency - sa->frequency;
    }
    return strcmp(sa->word, sb->word);
}

void trie_string_list_free(char **list) {
    if (!list) return;
    for (size_t i = 0; list[i] != NULL; i++) {
        free(list[i]);
    }
    free(list);
}

char **trie_suggest(const Trie *trie, const char *prefix, size_t limit) {
    if (!trie || limit == 0) return NULL;

    TrieNode *curr = trie->root;
    if (prefix && *prefix) {
        const unsigned char *p = (const unsigned char *)prefix;
        while (*p) {
            unsigned char idx = *p;
            if (!curr->children[idx]) {
                /* Prefix not found */
                char **empty = (char **)calloc(1, sizeof(char *));
                empty[0] = NULL;
                return empty;
            }
            curr = curr->children[idx];
            p++;
        }
    }

    size_t cap = 256;
    SuggestItem *items = (SuggestItem *)calloc(cap, sizeof(SuggestItem));
    size_t count = 0;

    char buffer[256];
    int depth = 0;
    if (prefix) {
        size_t plen = strlen(prefix);
        if (plen >= sizeof(buffer)) plen = sizeof(buffer) - 1;
        memcpy(buffer, prefix, plen);
        depth = (int)plen;
    }

    collect_suggestions(curr, buffer, depth, items, &count, cap);

    qsort(items, count, sizeof(SuggestItem), compare_suggestions);

    size_t result_count = (count < limit) ? count : limit;
    char **results = (char **)calloc(result_count + 1, sizeof(char *));

    for (size_t i = 0; i < result_count; i++) {
        results[i] = items[i].word;
    }
    results[result_count] = NULL;

    for (size_t i = result_count; i < count; i++) {
        free(items[i].word);
    }
    free(items);

    return results;
}
