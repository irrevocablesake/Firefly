struct UniformData
{
    float aspectRatio;
};

struct PushConstants
{
    UniformData* uniformBufferBDA;
};

[[vk::binding(0, 0)]]
RWTexture2D<float4> outputImage;

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 id: SV_DispatchThreadID, uniform PushConstants pc) {
    UniformData data = *pc.uniformBufferBDA;

    outputImage[id.xy] = float4(0.0, 0.0, data.aspectRatio, 1);
}