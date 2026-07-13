#version 450

void main() {
    // Square vertices in clockwise order for a triangle strip
    vec2 pos[4] = vec2[](
        vec2(-0.5, -0.5), // Bottom-left
        vec2(-0.5,  0.5), // Top-left
        vec2( 0.5, -0.5), // Bottom-right
        vec2( 0.5,  0.5)  // Top-right
    );
    gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0);
}