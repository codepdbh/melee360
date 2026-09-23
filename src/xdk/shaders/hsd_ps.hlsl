sampler2D texture0 : register(s0);
sampler2D texture1 : register(s1);
float4 stages[48] : register(c0);
float4 baseColor : register(c48);
float4 specColor : register(c49);
float4 fogColor : register(c50);
float4 alphaCompare0 : register(c51);
float4 alphaCompare1 : register(c52);
float4 alphaOp : register(c53);
bool slot0Pre : register(b0);
bool slot1Pre : register(b1);
bool slot0Post : register(b2);
bool slot1Post : register(b3);
bool slot0Tev : register(b4);
bool slot1Tev : register(b5);
bool vertexBase : register(b6);
bool diffuseLit : register(b7);
bool specular : register(b8);
bool alphaTest : register(b9);
bool fogEnabled : register(b10);

float4 Stage(int b, float4 prev, float4 x)
{
    float3 in0 = stages[b + 0].x * prev.rgb + stages[b + 0].y * x.rgb +
                 stages[b + 0].z * x.aaa + stages[b + 4].rgb;
    float3 in1 = stages[b + 1].x * prev.rgb + stages[b + 1].y * x.rgb +
                 stages[b + 1].z * x.aaa + stages[b + 5].rgb;
    float3 in2 = stages[b + 2].x * prev.rgb + stages[b + 2].y * x.rgb +
                 stages[b + 2].z * x.aaa + stages[b + 6].rgb;
    float3 in3 = stages[b + 3].x * prev.rgb + stages[b + 3].y * x.rgb +
                 stages[b + 3].z * x.aaa + stages[b + 7].rgb;
    float4 ia = stages[b + 8] * prev.a + stages[b + 9] * x.a +
                float4(stages[b + 4].a, stages[b + 5].a, stages[b + 6].a,
                       stages[b + 7].a);
    float4 op = stages[b + 10];
    float4 opa = stages[b + 11];
    float3 c = (in3 + op.x * lerp(in0, in1, in2) + op.y) * op.z;
    float a = (ia.w + opa.x * lerp(ia.x, ia.y, ia.z) + opa.y) * opa.z;
    if (op.w > 0.5)
        c = saturate(c);
    if (opa.w > 0.5)
        a = saturate(a);
    return float4(c, a);
}

float4 ApplySlot0(float4 c, float2 uv)
{
    float4 t = tex2D(texture0, uv);
    if (slot0Tev)
        t = Stage(0, float4(0.0, 0.0, 0.0, 0.0), t);
    return Stage(12, c, t);
}

float4 ApplySlot1(float4 c, float2 uv)
{
    float4 t = tex2D(texture1, uv);
    if (slot1Tev)
        t = Stage(24, float4(0.0, 0.0, 0.0, 0.0), t);
    return Stage(36, c, t);
}

float Compare(float a, float4 test)
{
    float v = floor(a * 255.0 + 0.5);
    return dot(float3(v < test.w, v == test.w, v > test.w), test.xyz);
}

float4 main(float4 ras0 : COLOR0, float4 ras1 : COLOR1, float4 uv : TEXCOORD0,
            float fog : TEXCOORD1) : COLOR0
{
    float4 c = vertexBase ? ras0 : baseColor;
    if (slot0Pre)
        c = ApplySlot0(c, uv.xy);
    if (slot1Pre)
        c = ApplySlot1(c, uv.zw);
    if (diffuseLit)
        c = saturate(c * ras0);
    if (specular)
        c.rgb = saturate(c.rgb + specColor.rgb * ras1.rgb);
    if (slot0Post)
        c = ApplySlot0(c, uv.xy);
    if (slot1Post)
        c = ApplySlot1(c, uv.zw);
    if (alphaTest) {
        float p0 = Compare(c.a, alphaCompare0) > 0.5 ? 1.0 : 0.0;
        float p1 = Compare(c.a, alphaCompare1) > 0.5 ? 1.0 : 0.0;
        float x = p0 + p1 - 2.0 * p0 * p1;
        float passed = dot(alphaOp, float4(p0 * p1, p0 + p1 - p0 * p1, x, 1.0 - x));
        clip(passed - 0.5);
    }
    if (fogEnabled)
        c.rgb = lerp(c.rgb, fogColor.rgb, fog);
    return c;
}
