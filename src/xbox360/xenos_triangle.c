#include "m360_log.h"

#include <stdint.h>
#include <string.h>
#include <xenos/xe.h>

#include "m360_xenos_shaders.h"

struct m360_test_vertex {
    float position[3];
    float normal[3];
    float tangent[3];
    uint32_t color;
    float uv[2];
};

int m360_xenos_draw_triangle(struct XenosDevice *device)
{
    static const struct XenosVBFFormat format = {
        5, {
            {XE_USAGE_POSITION, 0, XE_TYPE_FLOAT3},
            {XE_USAGE_NORMAL, 0, XE_TYPE_FLOAT3},
            {XE_USAGE_TANGENT, 0, XE_TYPE_FLOAT3},
            {XE_USAGE_COLOR, 0, XE_TYPE_UBYTE4},
            {XE_USAGE_TEXCOORD, 0, XE_TYPE_FLOAT2}
        }
    };
    static const struct m360_test_vertex vertices[3] = {
        {{ 0.0f,  0.70f, 0.5f}, {0,0,-1}, {1,0,0}, 0xff3030ff, {0.5f,0}},
        {{-0.70f, -0.55f, 0.5f}, {0,0,-1}, {1,0,0}, 0x30ff30ff, {0,1}},
        {{ 0.70f, -0.55f, 0.5f}, {0,0,-1}, {1,0,0}, 0x3030ffff, {1,1}}
    };
    static const float identity[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
    };
    static const float light[4] = {0, 0, -1, 0};
    struct XenosShader *pixel_shader, *vertex_shader;
    struct XenosVertexBuffer *vertex_buffer;
    void *mapped;

    pixel_shader = Xe_LoadShaderFromMemory(device, m360_shader_ps);
    vertex_shader = Xe_LoadShaderFromMemory(device, m360_shader_vs);
    if (!pixel_shader || !vertex_shader)
        return -1;
    Xe_InstantiateShader(device, pixel_shader, 0);
    Xe_InstantiateShader(device, vertex_shader, 0);
    Xe_ShaderApplyVFetchPatches(device, vertex_shader, 0, &format);

    vertex_buffer = Xe_CreateVertexBuffer(device, sizeof(vertices));
    if (!vertex_buffer)
        return -1;
    mapped = Xe_VB_Lock(device, vertex_buffer, 0, sizeof(vertices), XE_LOCK_WRITE);
    if (!mapped)
        return -1;
    memcpy(mapped, vertices, sizeof(vertices));
    Xe_VB_Unlock(device, vertex_buffer);

    Xe_InvalidateState(device);
    Xe_SetVertexShaderConstantF(device, 0, identity, 4);
    Xe_SetVertexShaderConstantF(device, 4, identity, 4);
    Xe_SetPixelShaderConstantF(device, 0, light, 1);
    Xe_SetShader(device, SHADER_TYPE_PIXEL, pixel_shader, 0);
    Xe_SetShader(device, SHADER_TYPE_VERTEX, vertex_shader, 0);
    Xe_SetStreamSource(device, 0, vertex_buffer, 0, sizeof(struct m360_test_vertex));
    Xe_DrawPrimitive(device, XE_PRIMTYPE_TRIANGLELIST, 0, 1);
    Xe_SetClearColor(device, 0x101828ff);
    Xe_Resolve(device);
    Xe_Sync(device);
    M360_LOG_GPU("Xenos test triangle submitted");
    return 0;
}
