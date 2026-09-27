struct UniformData {
    float imageWidth;
    float imageHeight;
};

[[vk::binding(0, 0)]]
RWTexture2D<float4> outputImage;

[[vk::binding(0, 1)]]
ConstantBuffer<UniformData> uniformBuffer;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 id: SV_DispatchThreadID) {
    float r = float(id.x) / ( uniformBuffer.imageWidth - 1 );
    float g = float(id.y) / ( uniformBuffer.imageHeight - 1 );
    float b = 0.0;

    outputImage[id.xy] = float4( b, g, r, 1);
}