#ifndef TEST_H
#define TEST_H

#include <stdio.h>

extern int g_fails;
extern int g_checks;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        g_checks++;                                                                                \
        if (!(cond)) {                                                                             \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                 \
            g_fails++;                                                                             \
        }                                                                                          \
    } while (0)

#endif /* TEST_H */
