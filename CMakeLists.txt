cmake_minimum_required(VERSION 3.10)
project(VkLayerWoTLK)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${VULKAN_INCLUDE_DIRS})

# Source files
set(VKLAYER_SOURCES
    src/main.cpp
    src/vklayer/vk_layer.cpp
)

# Create the Vulkan layer shared library
add_library(vklayer_understyx_wotlk SHARED ${VKLAYER_SOURCES})

# Link Vulkan
target_link_libraries(vklayer_understyx_wotlk PRIVATE ${VULKAN_LIBRARIES})

# Set compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(vklayer_understyx_wotlk PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -fPIC
    )
endif()

# Output library name
set_target_properties(vklayer_understyx_wotlk PROPERTIES
    OUTPUT_NAME vklayer_understyx_wotlk
    PREFIX "lib"
    SUFFIX ".so"
)

# Optional: Add install target
install(TARGETS vklayer_understyx_wotlk LIBRARY DESTINATION lib)
install(FILES VkLayer_understyx_WOTLK.json DESTINATION share/vulkan/icd.d)

# Print build information
message(STATUS "WoTLK Vulkan Layer Configuration")
message(STATUS "  Project: ${PROJECT_NAME}")
message(STATUS "  Vulkan: ${VULKAN_INCLUDE_DIRS}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
