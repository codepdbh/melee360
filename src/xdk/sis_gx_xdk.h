#ifndef M360_SIS_GX_XDK_H
#define M360_SIS_GX_XDK_H
/* GX boundary for the unchanged original SIS interpreter. */
void M360_SisMatrix(float m[3][4]);
void M360_SisDepth(int enabled);
void M360_SisTextureInit(GXTexObj*, void*, unsigned, unsigned, unsigned);
void M360_SisTextureLoad(GXTexObj*);
void M360_SisColor(GXColor);
void M360_SisUseTexture(int enabled);
void M360_SisBegin(unsigned count);
void M360_SisPosition(float, float, float);
void M360_SisTexCoord(float, float);
void M360_SisEnd(void);
#define GXLoadPosMtxImm(m,i) M360_SisMatrix(m)
#define GXSetZMode(e,c,w) M360_SisDepth(e)
#define HSD_StateSetZMode(e,c,w) M360_SisDepth(e)
#define GXInitTexObj(o,p,w,h,f,s,t,m) M360_SisTextureInit(o,p,w,h,f)
#define GXLoadTexObj(o,i) M360_SisTextureLoad(o)
#define GXSetTevColor(i,c) M360_SisColor(c)
#define GXSetTevAlphaIn(s,a,b,c,d) M360_SisUseTexture((b)==GX_CA_TEXA)
#define GXBegin(p,f,n) M360_SisBegin(n)
#define GXPosition3f32(x,y,z) M360_SisPosition(x,y,z)
#define GXTexCoord2f32(u,v) M360_SisTexCoord(u,v)
#define GXEnd() M360_SisEnd()
/* SIS uses a fixed direct-position/UV, constant RGB + texture-alpha pipeline;
 * its native submit configures these states for every quad. */
#define GXClearVtxDesc() ((void)0)
#define GXSetCullMode(c) ((void)0)
#define GXSetCurrentMtx(i) ((void)0)
#define GXSetNumChans(n) ((void)0)
#define HSD_StateSetNumChans(n) ((void)0)
#define GXSetNumTexGens(n) ((void)0)
#define GXSetNumTevStages(n) ((void)0)
#define GXSetVtxAttrFmt(f,a,c,t,s) ((void)0)
#define GXSetVtxDesc(a,t) ((void)0)
#define GXSetTexCoordGen2(a,b,c,d,e,f) ((void)0)
#define GXSetTevOrder(a,b,c,d) ((void)0)
#define GXSetTevColorIn(a,b,c,d,e) ((void)0)
#define GXSetTevColorOp(a,b,c,d,e,f) ((void)0)
#define GXSetTevAlphaOp(a,b,c,d,e,f) ((void)0)
#define GXSetBlendMode(a,b,c,d) ((void)0)
#define GXSetAlphaCompare(a,b,c,d,e) ((void)0)
#define GXSetColorUpdate(a) ((void)0)
#define GXSetAlphaUpdate(a) ((void)0)
#define HSD_StateInvalidate(a) ((void)0)
#endif
