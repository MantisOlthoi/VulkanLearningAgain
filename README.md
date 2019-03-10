# VulkanLearningAgain
This is just a personal project so I can finally get around to learning Vulkan in more detail.

This will initially take the form of a simple space simulator beginning with a starfighter in an asteroid field. To give myself a challenge I'd also like to implement some basic collision physics between the asteroids in the field. Preferably on a large scale so that the field can come alive. This will be implemented on the GPU as much as possible.

# Dependencies
This project depends on the SDL2 and Vulkan SDKs.
* Vulkan - Download the latest SDK from https://www.lunarg.com/vulkan-sdk/ and install it. It should setup the needed environment variables this project is looking for.
* SDL2 - Download the latest SDL2 SDL from https://www.libsdl.org/index.php and extract it somewhere. Then create a user or system environment variable, SDL2_SDK, and put it at the extracted SDL2 top-level folder.
