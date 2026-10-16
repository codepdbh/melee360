#include <string.h>
#include <dolphin/gx.h>
#include "hsd_particle_xdk.h"
#include "sis_gx_xdk.h"
void M360_HsdDrawTextQuad(const M360ParticleVertex*, const M360ParticleTexture*, int);
static float s_matrix[3][4];
static M360ParticleVertex s_corners[4];
static M360ParticleTexture s_texture;
static GXColor s_color;
static unsigned s_count, s_expected;
static int s_depth, s_useTexture;
typedef struct SisTexture { void* image; unsigned width, height, format; } SisTexture;
typedef char SisTextureFitsGXTexObj[sizeof(SisTexture)<=sizeof(GXTexObj)?1:-1];
void M360_SisMatrix(float m[3][4]) { memcpy(s_matrix,m,sizeof(s_matrix)); }
void M360_SisDepth(int enabled) { s_depth=enabled; }
void M360_SisTextureInit(GXTexObj* obj, void* image, unsigned w, unsigned h, unsigned format) {
    SisTexture t; t.image=image; t.width=w; t.height=h; t.format=format;
    memcpy(obj,&t,sizeof(t));
}
void M360_SisTextureLoad(GXTexObj* obj) {
    SisTexture t; memcpy(&t,obj,sizeof(t));
    memset(&s_texture,0,sizeof(s_texture));
    s_texture.image=t.image; s_texture.width=t.width; s_texture.height=t.height; s_texture.format=t.format;
}
void M360_SisColor(GXColor color) { s_color=color; }
void M360_SisUseTexture(int enabled) { s_useTexture=enabled; }
void M360_SisBegin(unsigned count) { s_count=0; s_expected=count; }
void M360_SisPosition(float x,float y,float z) {
    M360ParticleVertex* v;
    if(s_count>=4) return;
    v=&s_corners[s_count];
    v->x=s_matrix[0][0]*x+s_matrix[0][1]*y+s_matrix[0][2]*z+s_matrix[0][3];
    v->y=s_matrix[1][0]*x+s_matrix[1][1]*y+s_matrix[1][2]*z+s_matrix[1][3];
    v->z=s_matrix[2][0]*x+s_matrix[2][1]*y+s_matrix[2][2]*z+s_matrix[2][3];
    v->r=s_color.r/255.0f; v->g=s_color.g/255.0f; v->b=s_color.b/255.0f; v->a=s_color.a/255.0f;
}
void M360_SisTexCoord(float u,float v) {
    if(s_count>=4) return;
    s_corners[s_count].u=u; s_corners[s_count].v=v; ++s_count;
}
void M360_SisEnd(void) {
    if(s_expected==4 && s_count==4)
        M360_HsdDrawTextQuad(s_corners,s_useTexture?&s_texture:NULL,s_depth);
}
