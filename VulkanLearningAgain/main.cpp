#include <stdio.h>
#include <stdlib.h>
#include "vulkanEngine.h"
#include <exception>
#include <assert.h>
#include <chrono>

int main(void)
{
	try {
		VulkanEngine engine;

		auto startTime = std::chrono::high_resolution_clock::now();
		engine.init();
		auto endTime = std::chrono::high_resolution_clock::now();

		printf("Time to initialize engine: %lf seconds\n", std::chrono::duration<double>(endTime - startTime).count());
	}
	catch (std::exception &e)
	{
		fprintf(stderr, "ERROR (%s:%u): Caught exception : %s\n", __FILE__, __LINE__, e.what());
		system("pause");
		return 1;
	}

	system("pause");

	return 0;
}