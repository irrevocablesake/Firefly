struct VSOutput
{
    float4 position : SV_Position;
};

[shader("vertex")]
VSOutput main(uint vertexID : SV_VertexID)
{
    float2 positions[3] = {
        float2(-1.0, -1.0),
        float2( 3.0, -1.0),
        float2(-1.0,  3.0)
    };

    VSOutput output;
    output.position = float4(positions[vertexID], 0.0, 1.0);

    return output;
}