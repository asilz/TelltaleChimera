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

layout(location = 0) in vec4 inPosition;
layout(location = 1) in float blendWeight;
layout(location = 2) in uvec4 blendIndex;
layout(location = 3) in vec4 normals;
layout(location = 4) in vec4 tangents;
layout(location = 5) in vec4 colors;
layout(location = 6) in vec2 texCoords;
layout(location = 7) in vec2 texCoords2;

layout(location = 0) out vec3 fragColor;

void main() {
    /*
    vec4 animatedPosition = vec4(0.0);
    mat4 jointTransform = mat4(0.0);
    vec4 inWeight = vec4(1.0, 0.0, 0.0, 0.0);
    for(int i = 0; i < 4; i++) {
        if(inWeight[i] == 0) {
            continue;
        }
        if(blendIndex[i] >= ubo.boneCount) {
            animatedPosition = inPosition;
            jointTransform = mat4(1.0);
            break;
        }

        mat4 jointMatrix = ubo.boneTransforms[blendIndex[i]];

        vec4 localPosition = jointMatrix * inPosition;
        animatedPosition += localPosition * inWeight[i];
        jointTransform += jointMatrix * inWeight[i];
    }
    vec4 positionWorld = ubo.model * animatedPosition;

    vec4 newVertex;
    newVertex = ((inverse(ubo.baseTransforms[blendIndex.x]) * ubo.baseTransforms[blendIndex.x]) * inPosition) * inWeight.x;
    newVertex = ((inverse(ubo.baseTransforms[blendIndex.y]) * ubo.baseTransforms[blendIndex.y]) * inPosition) * inWeight.y + newVertex;
    newVertex = ((inverse(ubo.baseTransforms[blendIndex.z]) * ubo.baseTransforms[blendIndex.z]) * inPosition) * inWeight.z + newVertex;
    newVertex = ((inverse(ubo.baseTransforms[blendIndex.w]) * ubo.baseTransforms[blendIndex.w]) * inPosition) * inWeight.w + newVertex;
    */

    //newVertex = vec4(inPosition, 1.0) * inWeight.x;
    //newVertex = vec4(inPosition, 1.0) * inWeight.y + newVertex;
    //newVertex = vec4(inPosition, 1.0) * inWeight.z + newVertex;
    //newVertex = vec4(inPosition, 1.0) * inWeight.w + newVertex;

    gl_Position = ubo.proj * ubo.view * ubo.model * ubo.vertexTransform * vec4(inPosition.xyz, 1.0);
    //gl_Position = vec4(inPosition.xyz, 1.0);
    fragColor = vec3(0.82, 0.06, 0.06);
}