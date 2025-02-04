#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 baseTransforms[256];
    mat4 boneTransforms[256];
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 vertexTransform;
    int boneCount;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in uvec4 inIndex;
layout(location = 2) in vec4 inWeight;

layout(location = 0) out vec3 fragColor;

void main() {
    vec4 animatedPosition = vec4(0.0);
    mat4 jointTransform = mat4(0.0);
    for(int i = 0; i < 4; i++) {
        if(inWeight[i] == 0) {
            continue;
        }
        if(inIndex[i] >= ubo.boneCount) {
            animatedPosition = vec4(inPosition, 1.0);
            jointTransform = mat4(1.0);
            break;
        }

        mat4 jointMatrix = ubo.boneTransforms[inIndex[i]];

        vec4 localPosition = jointMatrix * vec4(inPosition, 1.0);
        animatedPosition += localPosition * inWeight[i];
        jointTransform += jointMatrix * inWeight[i];
    }
    vec4 positionWorld = ubo.model * animatedPosition;

    vec4 newVertex;
    newVertex = ((inverse(ubo.baseTransforms[inIndex.x]) * ubo.baseTransforms[inIndex.x]) * vec4(inPosition, 1.0)) * inWeight.x;
    newVertex = ((inverse(ubo.baseTransforms[inIndex.y]) * ubo.baseTransforms[inIndex.y]) * vec4(inPosition, 1.0)) * inWeight.y + newVertex;
    newVertex = ((inverse(ubo.baseTransforms[inIndex.z]) * ubo.baseTransforms[inIndex.z]) * vec4(inPosition, 1.0)) * inWeight.z + newVertex;
    newVertex = ((inverse(ubo.baseTransforms[inIndex.w]) * ubo.baseTransforms[inIndex.w]) * vec4(inPosition, 1.0)) * inWeight.w + newVertex;

    //newVertex = vec4(inPosition, 1.0) * inWeight.x;
    //newVertex = vec4(inPosition, 1.0) * inWeight.y + newVertex;
    //newVertex = vec4(inPosition, 1.0) * inWeight.z + newVertex;
    //newVertex = vec4(inPosition, 1.0) * inWeight.w + newVertex;

    gl_Position = ubo.proj * ubo.view * ubo.model * ubo.vertexTransform * vec4(newVertex.xyzw);
    fragColor = vec3(0.82, 0.06, 0.06);
}