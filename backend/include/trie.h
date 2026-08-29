#ifndef CWEB_TRIE_H
#define CWEB_TRIE_H

#include <stddef.h>

typedef struct TrieNode {
    char             ch;
    int              is_end_of_word;
    int              frequency;
    struct TrieNode *children[256];
} TrieNode;

typedef struct {
    TrieNode *root;
    size_t    total_words;
} Trie;

Trie *trie_create(void);
void trie_free(Trie *trie);
int trie_insert(Trie *trie, const char *word, int frequency);
int trie_search(const Trie *trie, const char *word);

/* Returns a NULL-terminated array of strings matching prefix, up to limit */
char **trie_suggest(const Trie *trie, const char *prefix, size_t limit);

void trie_string_list_free(char **list);

#endif /* CWEB_TRIE_H */
