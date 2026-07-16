#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
    mat4 transform;
    vec2 translate;
    uint hasTexture;
    float padding;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    if (pc.hasTexture != 0u) {
        outColor = fragColor * texture(texSampler, fragTexCoord);
    } else {
        outColor = fragColor;
    }
}
