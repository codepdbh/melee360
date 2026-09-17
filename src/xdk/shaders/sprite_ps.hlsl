sampler2D spriteAtlas : register(s0);

float4 main(float4 color : COLOR0, float2 texcoord : TEXCOORD0) : COLOR0
{
    return tex2D(spriteAtlas, texcoord) * color;
}
