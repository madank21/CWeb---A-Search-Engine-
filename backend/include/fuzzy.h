#ifndef CWEB_FUZZY_H
#define CWEB_FUZZY_H

#include <stddef.h>

/* Bounded Levenshtein distance calculation with early exit if distance > max_dist */
int levenshtein_distance_bounded(const char *s1, const char *s2, int max_dist);

#endif /* CWEB_FUZZY_H */
