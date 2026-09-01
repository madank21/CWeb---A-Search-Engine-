# CWeb Security Architecture & Threat Model

## 1. STRIDE Threat Model & Mitigations

| Threat Category | Potential Attack Vector | CWeb Backend Mitigation |
|---|---|---|
| **Spoofing / Tampering** | Binary index file corruption | CRC32 checksum footer verification + version check on startup. Corrupted files trigger fallback corpus re-index without crashing. |
| **Information Disclosure** | Path traversal via `/page/:id` | Document IDs are integer parameters resolved through in-memory DocumentStore pointers. No arbitrary path concatenation. |
| **Denial of Service** | Resource exhaustion / Slowloris | Payload body capped at 1 MB (`CWEB_MAX_BODY_BYTES`), header capped at 8 KB. Per-IP token-bucket rate limiter (20 RPS / 40 burst). |
| **Elevation of Privilege** | Memory corruption / Buffer overflow | Strict bound checks on all string buffers, allocation safety checks, compiled with `-Wall -Wextra -Werror -std=c17`. |
