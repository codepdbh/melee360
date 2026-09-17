struct VertexInput
{
    float4 position : POSITION0;
    float4 color : COLOR0;
};

struct VertexOutput
{
    float4 position : POSITION0;
    float4 color : COLOR0;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.position = float4(input.position.x / 640.0f - 1.0f,
                             1.0f - input.position.y / 360.0f,
                             input.position.z,
                             input.position.w);
    output.color = input.color;
    return output;
}
