float4 projection[4] : register(c0);
float4 fogParams : register(c4);
float4 chanAmbient : register(c5);
float4 chanMaterial : register(c6);
float4 chanMode : register(c7);
float4 texMtx[4] : register(c8);
float4 texSrc : register(c12);
float4 lights[48] : register(c16);

struct VertexInput
{
    float3 position : POSITION0;
    float3 normal : NORMAL0;
    float4 color : COLOR0;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};

struct VertexOutput
{
    float4 position : POSITION0;
    float4 ras0 : COLOR0;
    float4 ras1 : COLOR1;
    float4 uv : TEXCOORD0;
    float fog : TEXCOORD1;
};

float2 TexCoord(float2 uv0, float2 uv1, float3 n, float source, float4 row0,
                float4 row1)
{
    float4 st = float4(source > 1.5 ? n.xy * float2(0.5, -0.5) + 0.5
                                    : (source > 0.5 ? uv1 : uv0), 1.0, 1.0);
    return float2(dot(row0, st), dot(row1, st));
}

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    float4 p = float4(input.position, 1.0);
    float4 clip = float4(dot(projection[0], p), dot(projection[1], p),
                         dot(projection[2], p), dot(projection[3], p));
    clip.z += clip.w;
    output.position = clip;

    float nl = dot(input.normal, input.normal);
    float3 n = nl > 1e-12 ? input.normal * rsqrt(nl) : float3(0.0, 0.0, 1.0);
    float4 lit = chanAmbient;
    float3 spec = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < 8; ++i) {
        float4 lpos = lights[i * 6 + 0];
        float4 ldir = lights[i * 6 + 1];
        float4 lcol = lights[i * 6 + 2];
        float4 cosAttn = lights[i * 6 + 3];
        float4 distAttn = lights[i * 6 + 4];
        float4 mask = lights[i * 6 + 5];
        if (mask.x + mask.z > 0.5) {
            float3 l = lpos.xyz - input.position;
            float dist2 = dot(l, l);
            float dist = sqrt(dist2);
            l = l / max(dist, 1e-6);
            float c = max(0.0, dot(l, ldir.xyz));
            float attn = max(0.0, cosAttn.x + cosAttn.y * c + cosAttn.z * c * c) /
                         max(dot(distAttn.xyz, float3(1.0, dist, dist2)), 1e-6);
            float diffuse = max(0.0, dot(l, n)) * attn;
            lit.rgb += mask.x * lcol.rgb * diffuse;
            lit.a += mask.z * lcol.a * diffuse;
        }
        if (mask.y > 0.5) {
            float3 l = normalize(lpos.xyz - input.position);
            float h = dot(n, l) >= 0.0 ? max(0.0, dot(n, ldir.xyz)) : 0.0;
            float3 k = normalize(distAttn.xyz);
            float3 hv = float3(1.0, h, h * h);
            float attn = max(0.0, dot(cosAttn.xyz, hv)) / max(dot(k, hv), 1e-6);
            spec += lcol.rgb * max(0.0, dot(l, n)) * attn;
        }
    }
    float4 material = chanMode.x > 0.5 && chanMode.x < 1.5 ? input.color : chanMaterial;
    float4 ras0 = material;
    if (chanMode.x > 1.5)
        ras0.rgb = material.rgb * saturate(lit.rgb);
    if (chanMode.y > 0.5)
        ras0.a = material.a * saturate(lit.a);
    output.ras0 = ras0;
    output.ras1 = float4(saturate(spec), 1.0);
    output.uv = float4(TexCoord(input.uv0, input.uv1, n, texSrc.x, texMtx[0], texMtx[1]),
                       TexCoord(input.uv0, input.uv1, n, texSrc.y, texMtx[2], texMtx[3]));

    float fog = 0.0;
    if (fogParams.z > 0.5) {
        float x = saturate((-input.position.z - fogParams.x) /
                           max(fogParams.y - fogParams.x, 1e-6));
        if (fogParams.z < 2.5)
            fog = x;
        else if (fogParams.z < 4.5)
            fog = 1.0 - exp2(-8.0 * x);
        else if (fogParams.z < 5.5)
            fog = 1.0 - exp2(-8.0 * x * x);
        else if (fogParams.z < 6.5)
            fog = exp2(-8.0 * (1.0 - x));
        else
            fog = exp2(-8.0 * (1.0 - x) * (1.0 - x));
    }
    output.fog = fog;
    return output;
}
