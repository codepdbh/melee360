sampler2D texture0 : register(s0);
sampler2D texture1 : register(s1);
float4 fogColor : register(c0);

float4 main(float4 color : COLOR0, float2 texcoord0 : TEXCOORD0,
            float3 texcoord1 : TEXCOORD1) : COLOR0
{
    float4 result = tex2D(texture0, texcoord0) *
                    tex2D(texture1, texcoord1.xy) * color;
    result.rgb = lerp(result.rgb, fogColor.rgb, texcoord1.z);
    return result;
}
