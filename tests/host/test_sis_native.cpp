#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "dolphin/gx.h"
#include "hsd_particle_xdk.h"
static unsigned draws;
static M360ParticleVertex captured[4];
static M360ParticleTexture texture;
static bool textured;
static int depth;
void M360_HsdDrawTextQuad(const M360ParticleVertex* v,const M360ParticleTexture* t,int z) {
    ++draws;memcpy(captured,v,sizeof(captured));textured=t!=NULL;
    if(t) texture=*t;
    depth=z;
}
#include "../../src/xdk/sis_native_xdk.c"
#include "sis_tev_original.h"
static bool near(float a,float b) { return fabsf(a-b)<0.00001f; }
static void submit(unsigned count) {
    M360_SisBegin(count);
    for(unsigned i=0;i<4;++i) { M360_SisPosition((float)i,2,3);M360_SisTexCoord((float)i/4,0.5f); }
    M360_SisEnd();
}
int main() {
    float m[3][4]={{2,0,0,10},{0,3,0,20},{0,0,4,30}};
    M360_SisMatrix(m);GXColor color={255,128,0,64};M360_SisColor(color);
    unsigned char glyph[512]={0};GXTexObj obj;
    M360_SisTextureInit(&obj,glyph,32,32,0);M360_SisTextureLoad(&obj);
    M360_SisUseTexture(1);M360_SisDepth(1);submit(4);
    assert(draws==1 && textured && depth==1);
    assert(texture.image==glyph && texture.width==32 && texture.height==32 && texture.format==0);
    for(unsigned i=0;i<4;++i) {
        assert(near(captured[i].x,10+2.0f*i) && near(captured[i].y,26) && near(captured[i].z,42));
        assert(near(captured[i].r,1) && near(captured[i].g,128/255.0f) && near(captured[i].a,64/255.0f));
        assert(near(captured[i].u,(float)i/4) && near(captured[i].v,0.5f));
    }
    M360_SisUseTexture(0);M360_SisDepth(0);submit(4);
    assert(draws==2 && !textured && depth==0);
    submit(3);assert(draws==2);
    // Evaluate the actual native setup with the original hsd_ps Stage formula.
    for(unsigned glyphAlpha=0;glyphAlpha<=2;++glyphAlpha) {
        float ps[54*4]={0};ConfigureQuadTev(ps,true);
        const TevStage* s=(const TevStage*)(ps+12*4);
        const float prev[4]={0.2f,0.4f,0.6f,0.7f};
        const float tex[4]={0.5f,0.5f,0.5f,glyphAlpha*0.5f};
        for(unsigned c=0;c<3;++c) {
            float in[4];for(unsigned j=0;j<4;++j) in[j]=s->w[j][0]*prev[c]+s->w[j][1]*tex[c]+s->w[j][2]*tex[3]+s->k[j][c];
            const float out=(in[3]+s->op[0]*(in[0]+(in[1]-in[0])*in[2])+s->op[1])*s->op[2];
            assert(near(out,prev[c]));
        }
        float ia[4];for(unsigned j=0;j<4;++j) ia[j]=s->wa[0][j]*prev[3]+s->wa[1][j]*tex[3]+s->k[j][3];
        const float a=(ia[3]+s->opa[0]*(ia[0]+(ia[1]-ia[0])*ia[2])+s->opa[1])*s->opa[2];
        assert(near(a,prev[3]*tex[3]));
    }
    puts("PASS: SIS view matrix, color, UV, I4 texture, solid background, quad bounds and independent glyph alpha");
}
