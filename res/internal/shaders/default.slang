struct PerFrame {
    float4x4 view;
    float4x4 projection;
    float3 cameraPosition;
};

struct PerObject {
    float4x4 model;
};

struct PerMaterial {
    float4 color;
};

struct VertexInput {
    float3 pos : POSITION;
    float3 color : COLOR;
    float2 uv : TEXCOORD;
};

struct VertexOutput {
    float4 pos : SV_POSITION;
    float3 color : COLOR;
    float2 uv : TEXCOORD;
};

ConstantBuffer<PerFrame> perFrame : register(b0);
ConstantBuffer<PerMaterial> perMaterial : register(b1);
ConstantBuffer<PerObject> perObject : register(b2);

Texture2D<float4> baseMap : register(t0, space1);
SamplerState baseMapSampler : register(s0, space1);

[shader("vertex")]
VertexOutput vertex_main(VertexInput input) {
    VertexOutput output;
    float4 worldPos = mul(perObject.model, float4(input.pos, 1.0));
    float4 viewPos = mul(perFrame.view, worldPos);

    output.pos = mul(perFrame.projection, viewPos);
    output.color = input.color;
    output.uv = input.uv;

    return output;
}

[shader("fragment")]
float4 fragment_main(VertexOutput input) : SV_Target {
    float4 texColor = baseMap.Sample(baseMapSampler, input.uv);
    return float4(input.color * perMaterial.color.rgb * texColor.rgb, 1.0);
}
