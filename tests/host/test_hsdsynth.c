#include "hsdsynth_xdk_compat.h"

#include <stdio.h>

extern void M360_HSD_HeapInit(void);
extern int M360_HsdSynthSelfTest(void);

int main(void)
{
    M360_HSD_HeapInit();

    int ok = M360_HsdSynthSelfTest();
    if (ok) {
        printf("[M360][SYNTH] HSD synth/devcom self-test passed\n");
        return 0;
    }
    printf("[M360][SYNTH] HSD synth/devcom self-test FAILED\n");
    return 1;
}
