#version 450 core

layout (set=0, binding=1) uniform u_UniformBuf
{
	mat4 mvp;
};
layout (location=0) in vec3 inPos;

layout (location=0) out vec4 outPos;

void main(void)
{
	outPos = mvp * vec4(inPos, 0.0);
}