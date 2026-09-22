sampler2D lumaTexture : register(s0);
sampler2D blueTexture : register(s1);
sampler2D redTexture : register(s2);

float4 main(float4 color : COLOR0, float2 texcoord : TEXCOORD0) : COLOR0
{
    float y = tex2D(lumaTexture, texcoord).r;
    float cb = tex2D(blueTexture, texcoord).r - 0.501961;
    float cr = tex2D(redTexture, texcoord).r - 0.501961;
    float3 rgb = float3(y + 1.402 * cr,
                        y - 0.344136 * cb - 0.714136 * cr,
                        y + 1.772 * cb);
    return float4(saturate(rgb), 1.0) * color;
}
