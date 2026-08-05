[shader("vertex")]
float4 vertex_main(uint vertexID: SV_VertexID) : SV_Position {
    static const float2 positions[3] = { float2(-1, -1), float2(3, -1), float2(-1, 3) };

    return float4(positions[vertexID], 0.0, 1.0);
}

[shader("fragment")]
float4 fragment_main() : SV_Target {
    return float4(1, 0, 0, 1);
}

