
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
};

struct VertexOutput {
    float4 pos : SV_POSITION;
    float3 color : COLOR;
};

ConstantBuffer<Camera> camera : register(b0);
ConstantBuffer<Object> object : register(b1);

[shader("vertex")]
VertexOutput vertex_main(VertexInput input) {
    VertexOutput output;
    float4 worldPos = mul(object.model, float4(input.pos, 1.0));
    float4 viewPos = mul(camera.view, worldPos);

    output.pos = mul(camera.projection, viewPos);
    output.color = input.color;

    return output;
}

[shader("fragment")]
float4 fragment_main(VertexOutput input) : SV_Target {
    return float4(input.color, 1.0);
}
