#ifndef MELEE360_CAMPAIGN_XDK_H
#define MELEE360_CAMPAIGN_XDK_H
#include <melee/gm/types.h>
enum { M360_CLASSIC_COMBAT_ROUNDS = 7 };
typedef struct M360CampaignEncounter {
    unsigned ordinal, flags, seconds, opponents, stkind;
    unsigned ckinds[3];
} M360CampaignEncounter;
int M360_ClassicBegin(unsigned ckind);
int M360_ClassicEncounter(unsigned round, M360CampaignEncounter* out);
int M360_CampaignPlayers(unsigned mode, const M360CampaignEncounter* encounter,
    unsigned difficulty, unsigned ckind, unsigned color, unsigned stocks,
    PlayerInitData* players, int* itemFreq);
#endif
