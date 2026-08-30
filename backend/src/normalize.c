#include "normalize.h"
#include <string.h>

static int is_vowel(char c) {
    return (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u');
}

void normalize_token(char *token) {
    if (!token) return;
    size_t len = strlen(token);
    if (len <= 3) return;

    /* Suffix: -ing */
    if (len > 5 && strcmp(token + len - 3, "ing") == 0) {
        token[len - 3] = '\0';
        len -= 3;
        /* Restore double consonant e.g. running -> run */
        if (len >= 3 && token[len - 1] == token[len - 2] && !is_vowel(token[len - 1])) {
            token[len - 1] = '\0';
        }
        return;
    }

    /* Suffix: -tion */
    if (len > 5 && strcmp(token + len - 4, "tion") == 0) {
        token[len - 4] = '\0';
        return;
    }

    /* Suffix: -ed */
    if (len > 4 && strcmp(token + len - 2, "ed") == 0) {
        token[len - 2] = '\0';
        len -= 2;
        if (len >= 3 && token[len - 1] == token[len - 2] && !is_vowel(token[len - 1])) {
            token[len - 1] = '\0';
        }
        return;
    }

    /* Suffix: -ies -> -y */
    if (len > 4 && strcmp(token + len - 3, "ies") == 0) {
        token[len - 3] = 'y';
        token[len - 2] = '\0';
        return;
    }

    /* Suffix: -es */
    if (len > 4 && strcmp(token + len - 2, "es") == 0) {
        token[len - 2] = '\0';
        return;
    }

    /* Suffix: -s */
    if (len > 3 && token[len - 1] == 's' && token[len - 2] != 's') {
        token[len - 1] = '\0';
        return;
    }
}
