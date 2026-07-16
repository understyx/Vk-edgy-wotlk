cmake_minimum_required(VERSION 3.10)
project(VkLayerWoTLK)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Vulkan - if not found, we'll use headers from include directory
find_package(Vulkan)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/jshtml)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/jshtml/AppCore)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/jshtml/JavaScriptCore)

if(Vulkan_FOUND)
    include_directories(${VULKAN_INCLUDE_DIRS})
else()
    message(WARNING "Vulkan not found, assuming headers in system path")
endif()

# Source files
set(VKLAYER_SOURCES
    src/main.cpp
    src/vklayer/vk_layer.cpp
    src/jshtml/jshtml.cpp
    src/jshtml/ultralight_manager.cpp
)

# Create the Vulkan layer shared library
add_library(vklayer_understyx_wotlk SHARED ${VKLAYER_SOURCES})

# Link directories
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

# Link Vulkan and Ultralight
if(Vulkan_FOUND)
    target_link_libraries(vklayer_understyx_wotlk PRIVATE ${VULKAN_LIBRARIES})
endif()
set(LIB_PATH "/home/stygian/programming/Vk-edgy-wotlk/lib")

target_link_libraries(vklayer_understyx_wotlk PRIVATE 
    ${LIB_PATH}/libUltralight.so
    ${LIB_PATH}/libUltralightCore.so
    ${LIB_PATH}/libWebCore.so
    ${LIB_PATH}/libAppCore.so
)

# Set compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(vklayer_understyx_wotlk PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -fPIC
        -fvisibility=hidden
        -fpermissive
    )
endif()

# Set RPATH for runtime library loading
set_target_properties(vklayer_understyx_wotlk PROPERTIES
    BUILD_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    INSTALL_RPATH "$ORIGIN/lib:$ORIGIN/../lib"
)

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
