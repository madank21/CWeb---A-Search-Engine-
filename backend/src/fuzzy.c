#include "fuzzy.h"
#include <stdlib.h>
#include <string.h>

static int min3(int a, int b, int c) {
    int m = (a < b) ? a : b;
    return (m < c) ? m : c;
}

int levenshtein_distance_bounded(const char *s1, const char *s2, int max_dist) {
    if (!s1 || !s2) return max_dist + 1;
    int len1 = (int)strlen(s1);
    int len2 = (int)strlen(s2);

    if (abs(len1 - len2) > max_dist) return max_dist + 1;

    int col[256];
    for (int y = 0; y <= len1; y++) {
        col[y] = y;
    }

    for (int x = 1; x <= len2; x++) {
        int lastdiag = col[0];
        col[0] = x;
        int min_in_col = x;

        for (int y = 1; y <= len1; y++) {
            int oldcol = col[y];
            int cost = (s1[y - 1] == s2[x - 1]) ? 0 : 1;
            col[y] = min3(col[y] + 1, col[y - 1] + 1, lastdiag + cost);
            lastdiag = oldcol;
            if (col[y] < min_in_col) min_in_col = col[y];
        }

        if (min_in_col > max_dist) {
            return max_dist + 1;
        }
    }

    return col[len1];
}
