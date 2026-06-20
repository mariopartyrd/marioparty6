#include "game/frand.h"

static u32 frand_seed;

u32 frandom(u32 seed)
{
    s32 k, m;

    if (seed == 0) {
        seed = OSGetTime();
        seed ^= 0xD826BC89;
    }

    k = seed / (u32)0x1F31D;
    m = seed - (k * 0x1F31D);
    seed = k * 0xB14;
    seed =  seed - m * 0x41A7;
    return seed;
}

u32 frand(void) {
    return frand_seed = frandom(frand_seed);
}

f32 frandf(void) {
    u32 value = frand();
    float ret;
    value &= 0x7FFFFFFF;
    ret = (float)value/2147483648;
    return ret;
}

u32 frandmod(u32 arg0) {
    u32 ret;
    frand_seed = frandom(frand_seed);
    ret = (frand_seed & 0x7FFFFFFF)%arg0;
    return ret;
}