// simpleVertex.h
// Details: Provides a C-style definition for the compiled SPR-V C-formatted code
//		corresponding to simpleVertex.glsl
//	In the pre-build steps, simpleVertex.glsl is compiled into SPR-V using roughly the following:
//		glslc -fshader-stage=vertex -mfmt=c simpleVertex.glsl -o spr-v-c/simpleVertex.spv
//	The above line compiles simpleVertex.glsl as a vertex shader and outputs the resulting binary
//		SPR-V code as a C-style initializer list. Then we can just #include it as shown below to
//		define it as an unsigned int buffer.

#pragma once

const unsigned int simpleVertexSPRV[] =
#include "spr-v-c/simpleVertex.spv"
;

const size_t simpleVertexSPRVLength = sizeof(simpleVertexSPRV);