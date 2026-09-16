# GX usage inventory

Mechanical scan of calls in `src/melee` and `src/sysdolphin` at melee-pc
commit `6c2f5c5` found 117 distinct `GX*` call names. This is an initial static
inventory: function-pointer calls and macro expansion can add requirements.

Current Xenos status for every entry below is **PENDING**. Aurora implements
much of this surface for WebGPU, but no Aurora-to-Xenos backend exists yet.
`GXInsertDebugMarker` is additionally **UNKNOWN** until its call site and
Aurora implementation are audited. None is marked unnecessary without runtime
tracing evidence.

```text
GXBegin GXCallDisplayList GXClearVtxDesc GXCmd1u8 GXColor1u16 GXColor1x16
GXColor1x8 GXColor3u8 GXColor4u8 GXCopyDisp GXCopyTex GXEnableTexOffsets
GXEnd GXGetProjectionv GXGetTexBufferSize GXGetTexObjFmt GXGetTexObjHeight
GXGetTexObjWidth GXGetViewportv GXInit GXInitFogAdjTable GXInitLightAttn
GXInitLightColor GXInitLightDir GXInitLightDistAttn GXInitLightPos
GXInitLightSpot GXInitTexObj GXInitTexObjCI GXInitTexObjLOD GXInitTlutObj
GXInsertDebugMarker GXInvalidateTexAll GXInvalidateVtxCache
GXLoadLightObjImm GXLoadNrmMtxImm GXLoadPosMtxImm GXLoadTexMtxImm
GXLoadTexObj GXLoadTlut GXNormal3f32 GXPixModeSync GXPosition2f32
GXPosition2u8 GXPosition3f32 GXProject GXSetAlphaCompare GXSetAlphaUpdate
GXSETARRAY GXSetBlendMode GXSetChanAmbColor GXSetChanCtrl
GXSetChanMatColor GXSetColorUpdate GXSetCopyClamp GXSetCopyClear
GXSetCopyFilter GXSetCullMode GXSetCurrentMtx GXSetDispCopyDst
GXSetDispCopyGamma GXSetDispCopySrc GXSetDispCopyYScale GXSetDither
GXSetDrawDone GXSetDrawDoneCallback GXSetDstAlpha GXSetFieldMode GXSetFog
GXSetFogRangeAdj GXSetIndTexCoordScale GXSetIndTexMtx GXSetIndTexOrder
GXSetLineWidth GXSetMisc GXSetNumChans GXSetNumIndStages GXSetNumTevStages
GXSetNumTexGens GXSetPixelFmt GXSetPointSize GXSetProjection GXSetScissor
GXSetTevAlphaIn GXSetTevAlphaOp GXSetTevClampMode GXSetTevColor
GXSetTevColorIn GXSetTevColorOp GXSetTevColorS10 GXSetTevDirect
GXSetTevIndirect GXSetTevKAlphaSel GXSetTevKColor GXSetTevKColorSel
GXSetTevOp GXSetTevOrder GXSetTevSwapMode GXSetTevSwapModeTable
GXSetTexCoordGen GXSetTexCoordGen2 GXSetTexCopyDst GXSetTexCopySrc
GXSetViewport GXSetViewportJitter GXSetVtxAttrFmt GXSetVtxDesc
GXSetZCompLoc GXSetZMode GXSetZTexture GXTexCoord1f32 GXTexCoord1u8
GXTexCoord1x16 GXTexCoord1x8 GXTexCoord2f32 GXTexCoord2u8 GXWaitDrawDone
```

Implementation order is framebuffer/copy and viewport, vertex submission and
matrices, texture objects and sampling, blend/depth state, then only the TEV
combinations observed through instrumentation. A universal GX implementation
is explicitly out of scope for the first playable milestone.

