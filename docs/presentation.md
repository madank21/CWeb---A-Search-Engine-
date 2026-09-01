# CWeb v2.0 Architecture Summary & Presentation

CWeb is a self-hosted, high-performance search engine built completely from scratch in C17, paired with a React Native / Web client.

## Core Engineering Highlights
1. **Custom HTML Tag Scanner**: Extracts title, description, category, keywords, headings, and body text while stripping comments and styles.
2. **Tokenizer & Normalization Pipeline**: UTF-8 safe scanning, ASCII case-folding, stop-word filtering, position tracking, and light normalization.
3. **Core Data Structures**: Custom FNV-1a Hash Table, Inverted Index with postings lists, and Prefix Autocomplete Trie.
4. **AST EBNF Query Parser**: Full boolean logic (`AND`, `OR`, `NOT`), phrase search `"..."`, field search `title:`, and bounded Levenshtein fuzzy matching (\(\le 2\)).
5. **Dual Ranking Algorithms**: Field-weighted TF-IDF & BM25 (\(k_1=1.2, b=0.75\)) with multi-term coordination multipliers.
6. **Custom Binary File Format**: Persistent binary format (`CWIX`) with versioning, payload size checks, and CRC32 checksums.
7. **Hand-Rolled Multi-Threaded HTTP Server**: RCU-style write lock for index pointer swapping, LRU query cache, token bucket rate limiter, and enumerated JSON error codes.
8. **Client Application**: React Native / Web client built with TanStack Query and Zustand, featuring zero client-side search logic.
9. **NFR Benchmarking**: Sub-millisecond p50 latency, p95 < 8 ms, index build < 130 ms, and RSS memory < 15 MB.
