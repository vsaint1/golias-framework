[shader("vertex")]
float4 vertex_main(uint vertexID: SV_VertexID) : SV_Position {
    static const float2 positions[3] = { float2(0, -0.5), float2(0.5, 0.5), float2(-0.5, 0.5) };

    return float4(positions[vertexID], 0.0, 1.0);
}

[shader("fragment")]
float4 fragment_main() : SV_Target {
    return float4(1, 0, 0, 1);
}


