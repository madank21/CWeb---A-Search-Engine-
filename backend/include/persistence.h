#ifndef CWEB_PERSISTENCE_H
#define CWEB_PERSISTENCE_H

#include "index.h"
#include <stddef.h>
#include <stdint.h>

#define CWEB_MAGIC_INDEX "CWIX"
#define CWEB_MAGIC_DOCS  "CWDX"
#define CWEB_MAGIC_TRIE  "CWTX"
#define CWEB_MAGIC_META  "CWMX"

#define CWEB_FORMAT_VERSION 1

typedef struct {
    char     magic[4];
    uint32_t version;
    uint64_t payload_len;
} FileHeader;

int persistence_save_index(const InvertedIndex *idx, const char *dir_path);
InvertedIndex *persistence_load_index(const char *dir_path);

/* Incremental index updater */
int persistence_incremental_update(InvertedIndex *idx, const char *pages_dir, const char *index_dir);

#endif /* CWEB_PERSISTENCE_H */
