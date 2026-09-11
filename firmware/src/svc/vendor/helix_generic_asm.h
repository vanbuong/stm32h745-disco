#ifndef _ASSEMBLY_H
#define _ASSEMBLY_H

/*
 * C fallback for Helix MULSHIFT32 / MADD64 (host GCC and Cortex-M4).
 * Force-included before helix sources so real/assembly.h is skipped.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef long long Word64;

static inline int MULSHIFT32(int x, int y)
{
    return (int)((((Word64)x) * ((Word64)y)) >> 32);
}

static inline int FASTABS(int x)
{
    int sign = x >> (int)((sizeof(int) * 8u) - 1u);

    x ^= sign;
    x -= sign;
    return x;
}

static inline int CLZ(int x)
{
    int n = 0;

    if (x == 0) {
        return (int)(sizeof(int) * 8u);
    }
    while ((x & (int)0x80000000) == 0) {
        n++;
        x <<= 1;
    }
    return n;
}

static inline Word64 MADD64(Word64 sum, int x, int y)
{
    return sum + ((Word64)x * (Word64)y);
}

static inline Word64 SHL64(Word64 x, int n)
{
    return x << n;
}

static inline Word64 SAR64(Word64 x, int n)
{
    return x >> n;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASSEMBLY_H */
