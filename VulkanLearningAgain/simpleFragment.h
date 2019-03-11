// simpleFragment.h
// Details: Provides a C-style definition for the compiled SPR-V C-formatted code
//		corresponding to simpleFragment.glsl
//	In the pre-build steps, simpleFragment.glsl is compiled into SPR-V using roughly the following:
//		glslc -fshader-stage=vertex -mfmt=c simpleFragment.glsl -o spr-v-c/simpleFragment.spv
//	The above line compiles simpleFragment.glsl as a vertex shader and outputs the resulting binary
//		SPR-V code as a C-style initializer list. Then we can just #include it as shown below to
//		define it as an unsigned int buffer.

#pragma once

const unsigned int simpleFragmentSPRV[] =
#include "spr-v-c/simpleFragment.spv"
;

const size_t simpleFragmentSPRVLength = sizeof(simpleFragmentSPRV);
