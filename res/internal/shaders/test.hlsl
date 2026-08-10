
struct Camera {
    float4x4 view;
    float4x4 projection;
};

struct Object {
    float4x4 model;
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


ConstantBuffer<Camera> camera : register(b0);
ConstantBuffer<Object> object : register(b1);
Texture2D<float4> albedo : register(t0, space1);
SamplerState baseSampler : register(s0, space1);

[shader("vertex")]
VertexOutput vertex_main(VertexInput input) {
    VertexOutput output;
    float4 worldPos = mul(object.model, float4(input.pos, 1.0));
    float4 viewPos = mul(camera.view, worldPos);

    output.pos = mul(camera.projection, viewPos);
    output.color = input.color;
    output.uv = input.uv;

    return output;
}

[shader("fragment")]
float4 fragment_main(VertexOutput input) : SV_Target {
    float4 texColor = albedo.Sample(baseSampler, input.uv);
    return float4(texColor.rgb, 1.0);
}
