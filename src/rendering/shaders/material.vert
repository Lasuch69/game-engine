#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 projView;
    mat4 view;
    int lightCount;
} ubo;

layout(push_constant) uniform MeshPushConstants {
    mat4 model;
    mat4 modelViewNormal;
} constants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outColor;
layout(location = 3) out vec2 outTexCoord;

void main() {
    vec4 vertPos4 = ubo.view * constants.model * vec4(inPosition, 1.0);
    outPosition = vec3(vertPos4) / vertPos4.w;

	outNormal = normalize(mat3(constants.modelViewNormal) * inNormal);
    outColor = inColor;
    outTexCoord = inTexCoord;

    gl_Position = ubo.projView * constants.model * vec4(inPosition, 1.0);
}
