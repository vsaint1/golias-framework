// [shader("vertex")]
// float4 vertex_main(uint vertexID: SV_VertexID) : SV_Position {
//     static const float2 positions[3] = { float2(0, -0.5), float2(0.5, 0.5), float2(-0.5, 0.5) };

//     return float4(positions[vertexID], 0.0, 1.0);
// }

// [shader("fragment")]
// float4 fragment_main() : SV_Target {
//     return float4(1, 0, 0, 1);
// }


struct VertexInput {
   [[vk::location(0)]] float2 pos : POSITION;
   [[vk::location(1)]] float3 color : COLOR;
};

struct VertexOutput {
    float4 pos : SV_POSITION;
    [[vk::location(1)]] float3 color : COLOR;
};

[shader("vertex")]
VertexOutput vertex_main(VertexInput input) {
    VertexOutput output;
    output.pos = float4(input.pos, 0.0, 1.0);
    output.color = input.color;
    return output;
}

[shader("fragment")]
float4 fragment_main(VertexOutput input) : SV_Target {
    return float4(input.color, 1.0);
}