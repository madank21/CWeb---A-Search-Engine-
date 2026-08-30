#include "persistence.h"
#include "crc32.h"
#include "html_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir_compat(path) _mkdir(path)
#else
#define mkdir_compat(path) mkdir(path, 0755)
#endif

static void write_header(FILE *fp, const char magic[4], uint64_t payload_len) {
    FileHeader hdr;
    memcpy(hdr.magic, magic, 4);
    hdr.version = CWEB_FORMAT_VERSION;
    hdr.payload_len = payload_len;
    fwrite(&hdr, sizeof(FileHeader), 1, fp);
}

static int read_and_validate_header(FILE *fp, const char expected_magic[4], uint64_t *out_payload_len) {
    FileHeader hdr;
    if (fread(&hdr, sizeof(FileHeader), 1, fp) != 1) return -1;
    if (memcmp(hdr.magic, expected_magic, 4) != 0) return -1;
    if (hdr.version > CWEB_FORMAT_VERSION) return -1;
    *out_payload_len = hdr.payload_len;
    return 0;
}

int persistence_save_index(const InvertedIndex *idx, const char *dir_path) {
    if (!idx || !dir_path) return -1;
    mkdir_compat(dir_path);

    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/index.dat", dir_path);

    FILE *fp = fopen(filepath, "wb");
    if (!fp) return -1;

    /* Reserve space for header */
    FileHeader dummy_hdr;
    memset(&dummy_hdr, 0, sizeof(dummy_hdr));
    fwrite(&dummy_hdr, sizeof(FileHeader), 1, fp);

    long payload_start = ftell(fp);

    /* Write terms */
    uint32_t term_count = (uint32_t)idx->total_terms;
    fwrite(&term_count, sizeof(uint32_t), 1, fp);

    for (size_t i = 0; i < idx->term_table->num_buckets; i++) {
        HashNode *node = idx->term_table->buckets[i];
        while (node) {
            PostingList *plist = (PostingList *)node->value;
            if (plist && plist->doc_freq > 0) {
                uint16_t tlen = (uint16_t)strlen(node->key);
                fwrite(&tlen, sizeof(uint16_t), 1, fp);
                fwrite(node->key, 1, tlen, fp);

                uint32_t df = (uint32_t)plist->doc_freq;
                fwrite(&df, sizeof(uint32_t), 1, fp);

                for (size_t k = 0; k < plist->doc_freq; k++) {
                    int32_t doc_id = (int32_t)plist->postings[k].document_id;
                    uint32_t tf = (uint32_t)plist->postings[k].frequency;
                    uint32_t pos_count = (uint32_t)plist->postings[k].position_count;

                    fwrite(&doc_id, sizeof(int32_t), 1, fp);
                    fwrite(&tf, sizeof(uint32_t), 1, fp);
                    fwrite(&pos_count, sizeof(uint32_t), 1, fp);

                    if (pos_count > 0 && plist->postings[k].positions) {
                        fwrite(plist->postings[k].positions, sizeof(uint32_t), pos_count, fp);
                    }
                }
            }
            node = node->next;
        }
    }

    long payload_end = ftell(fp);
    uint64_t payload_len = (uint64_t)(payload_end - payload_start);

    /* Calculate CRC32 over payload */
    fseek(fp, payload_start, SEEK_SET);
    uint8_t *payload_buf = (uint8_t *)malloc(payload_len);
    if (!payload_buf) { fclose(fp); return -1; }
    fread(payload_buf, 1, payload_len, fp);
    uint32_t crc = crc32_calculate(payload_buf, payload_len);
    free(payload_buf);

    /* Write CRC32 at end */
    fseek(fp, payload_end, SEEK_SET);
    fwrite(&crc, sizeof(uint32_t), 1, fp);

    /* Go back and write real header */
    fseek(fp, 0, SEEK_SET);
    write_header(fp, CWEB_MAGIC_INDEX, payload_len);

    fclose(fp);
    return 0;
}

InvertedIndex *persistence_load_index(const char *dir_path) {
    if (!dir_path) return NULL;

    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/index.dat", dir_path);

    FILE *fp = fopen(filepath, "rb");
    if (!fp) return NULL;

    uint64_t payload_len = 0;
    if (read_and_validate_header(fp, CWEB_MAGIC_INDEX, &payload_len) != 0) {
        fclose(fp);
        return NULL;
    }

    uint8_t *payload_buf = (uint8_t *)malloc(payload_len);
    if (!payload_buf) { fclose(fp); return NULL; }

    if (fread(payload_buf, 1, payload_len, fp) != payload_len) {
        free(payload_buf);
        fclose(fp);
        return NULL;
    }

    uint32_t stored_crc = 0;
    if (fread(&stored_crc, sizeof(uint32_t), 1, fp) != 1) {
        free(payload_buf);
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    uint32_t calc_crc = crc32_calculate(payload_buf, payload_len);
    if (calc_crc != stored_crc) {
        /* CRC mismatch - corrupt index */
        free(payload_buf);
        return NULL;
    }

    InvertedIndex *idx = index_create();
    if (!idx) { free(payload_buf); return NULL; }

    const uint8_t *p = payload_buf;
    const uint8_t *end = payload_buf + payload_len;

    if (p + sizeof(uint32_t) > end) { free(payload_buf); index_free(idx); return NULL; }

    uint32_t term_count = 0;
    memcpy(&term_count, p, sizeof(uint32_t));
    p += sizeof(uint32_t);

    for (uint32_t i = 0; i < term_count; i++) {
        if (p + sizeof(uint16_t) > end) break;
        uint16_t tlen = 0;
        memcpy(&tlen, p, sizeof(uint16_t));
        p += sizeof(uint16_t);

        if (p + tlen > end) break;
        char term[128];
        size_t copy_len = (tlen < sizeof(term) - 1) ? tlen : (sizeof(term) - 1);
        memcpy(term, p, copy_len);
        term[copy_len] = '\0';
        p += tlen;

        if (p + sizeof(uint32_t) > end) break;
        uint32_t df = 0;
        memcpy(&df, p, sizeof(uint32_t));
        p += sizeof(uint32_t);

        PostingList *plist = posting_list_create();

        for (uint32_t k = 0; k < df; k++) {
            if (p + sizeof(int32_t) + sizeof(uint32_t) + sizeof(uint32_t) > end) break;

            int32_t doc_id = 0;
            uint32_t tf = 0;
            uint32_t pos_count = 0;

            memcpy(&doc_id, p, sizeof(int32_t)); p += sizeof(int32_t);
            memcpy(&tf, p, sizeof(uint32_t)); p += sizeof(uint32_t);
            memcpy(&pos_count, p, sizeof(uint32_t)); p += sizeof(uint32_t);

            Posting post;
            post.document_id = doc_id;
            post.frequency = tf;
            post.position_count = pos_count;
            post.field_weight_sum = (float)tf;
            post.positions = NULL;

            if (pos_count > 0) {
                post.positions = (int *)calloc(pos_count, sizeof(int));
                if (p + pos_count * sizeof(uint32_t) <= end) {
                    memcpy(post.positions, p, pos_count * sizeof(uint32_t));
                    p += pos_count * sizeof(uint32_t);
                }
            }

            if (plist->doc_freq >= plist->capacity) {
                plist->capacity *= 2;
                plist->postings = (Posting *)realloc(plist->postings, plist->capacity * sizeof(Posting));
            }
            plist->postings[plist->doc_freq++] = post;
        }

        hash_table_put(idx->term_table, term, plist);
        trie_insert(idx->trie, term, 1);
    }

    free(payload_buf);
    idx->total_terms = idx->term_table->size;
    return idx;
}

int persistence_incremental_update(InvertedIndex *idx, const char *pages_dir, const char *index_dir) {
    if (!idx || !pages_dir) return -1;
    (void)index_dir;
    return 0;
}
