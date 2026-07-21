cmake_minimum_required(VERSION 3.16)
project(VkLayerWoTLK)


if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-include> $<$<COMPILE_LANGUAGE:CXX>:cstdint>)
endif()


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===========================================================================
# Vulkan
# ===========================================================================
find_package(Vulkan)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
if(Vulkan_FOUND)
    include_directories(${VULKAN_INCLUDE_DIRS})
else()
    message(WARNING "Vulkan not found, assuming headers in system path")
endif()

# ===========================================================================
# RmlUi (fetched from GitHub)
# ===========================================================================
include(FetchContent)

FetchContent_Declare(
    RmlUi
    GIT_REPOSITORY https://github.com/mikke89/RmlUi.git
    GIT_TAG        6.0
    GIT_SHALLOW    TRUE
)

# Build only the core library; disable samples and tests
set(RMLUI_SAMPLES_BACKEND "None"   CACHE STRING "" FORCE)
set(BUILD_SAMPLES          OFF     CACHE BOOL   "" FORCE)
set(BUILD_TESTING          OFF     CACHE BOOL   "" FORCE)
set(RMLUI_THIRDPARTY_CONTAINERS ON CACHE BOOL   "" FORCE)

# Enable Lua bindings if a Lua installation is found
find_package(Lua QUIET)
if(LUA_FOUND)
    message(STATUS "  Lua found (${LUA_VERSION_STRING}) – enabling RmlUi Lua bindings")
    set(RMLUI_LUA_BINDINGS ON CACHE BOOL "" FORCE)
else()
    message(STATUS "  Lua not found – RmlUi Lua bindings disabled")
    set(RMLUI_LUA_BINDINGS OFF CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(RmlUi)

# ===========================================================================
# Shader compilation
# ===========================================================================
find_program(GLSLC NAMES glslc)
find_program(GLSLANG_VALIDATOR NAMES glslangValidator)

set(SHADER_SRC_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/src/rmlui/shaders)
set(SHADER_OUT_DIR  ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

if(GLSLC)
    set(SPV_COMPILER ${GLSLC})
    set(SPV_FLAGS    "")
elseif(GLSLANG_VALIDATOR)
    set(SPV_COMPILER ${GLSLANG_VALIDATOR})
    set(SPV_FLAGS    "-V")
else()
    message(WARNING "Neither glslc nor glslangValidator found – shaders will not be compiled")
endif()

if(SPV_COMPILER)
    set(VERT_SPV ${SHADER_OUT_DIR}/rmlui.vert.spv)
    set(FRAG_SPV ${SHADER_OUT_DIR}/rmlui.frag.spv)

    add_custom_command(
        OUTPUT  ${VERT_SPV}
        COMMAND ${SPV_COMPILER} ${SPV_FLAGS} -o ${VERT_SPV} ${SHADER_SRC_DIR}/rmlui.vert
        DEPENDS ${SHADER_SRC_DIR}/rmlui.vert
        COMMENT "Compiling rmlui.vert → SPIR-V"
    )
    add_custom_command(
        OUTPUT  ${FRAG_SPV}
        COMMAND ${SPV_COMPILER} ${SPV_FLAGS} -o ${FRAG_SPV} ${SHADER_SRC_DIR}/rmlui.frag
        DEPENDS ${SHADER_SRC_DIR}/rmlui.frag
        COMMENT "Compiling rmlui.frag → SPIR-V"
    )

    add_custom_target(RmlUiShaders DEPENDS ${VERT_SPV} ${FRAG_SPV})
endif()

# ===========================================================================
# Layer library
# ===========================================================================
set(VKLAYER_SOURCES
    src/main.cpp
    src/vklayer/vk_layer.cpp
    src/rmlui/SystemInterface.cpp
    src/rmlui/FileInterface.cpp
    src/rmlui/VulkanRenderer.cpp
    src/rmlui/OverlayUI.cpp
    src/rmlui/WowDataModel.cpp
    src/wowmemory/wowmemory.cpp
)

add_library(vklayer_understyx_wotlk SHARED ${VKLAYER_SOURCES})

# Depend on shader compilation if available
if(TARGET RmlUiShaders)
    add_dependencies(vklayer_understyx_wotlk RmlUiShaders)
endif()

target_link_libraries(vklayer_understyx_wotlk
    PRIVATE
        RmlUi::Core
        dl
        $<$<BOOL:${Vulkan_FOUND}>:${VULKAN_LIBRARIES}>
        $<$<BOOL:${LUA_FOUND}>:RmlUi::Lua>
        $<$<BOOL:${LUA_FOUND}>:${LUA_LIBRARIES}>
)

target_include_directories(vklayer_understyx_wotlk
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        $<$<BOOL:${LUA_FOUND}>:${LUA_INCLUDE_DIR}>
)

# Propagate RMLUI_LUA_BINDINGS define so OverlayUI.cpp can conditionally
# include the Lua headers.
if(LUA_FOUND)
    target_compile_definitions(vklayer_understyx_wotlk PRIVATE RMLUI_LUA_BINDINGS)
endif()

# ===========================================================================
# Compiler flags
# ===========================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(vklayer_understyx_wotlk PRIVATE
        -Wall
        -Wextra
        -fPIC
        -fvisibility=hidden
        -fpermissive
    )
endif()

set_target_properties(vklayer_understyx_wotlk PROPERTIES
    OUTPUT_NAME vklayer_understyx_wotlk
    PREFIX      "lib"
    SUFFIX      ".so"
)

# ===========================================================================
# Install
# ===========================================================================
install(TARGETS vklayer_understyx_wotlk LIBRARY DESTINATION lib)
install(FILES   VkLayer_understyx_WOTLK.json DESTINATION share/vulkan/icd.d)

# Install shaders alongside the library so the layer can load them at runtime
if(SPV_COMPILER)
    install(FILES ${VERT_SPV} ${FRAG_SPV} DESTINATION lib/shaders)
endif()

# Install UI documents and fonts
install(DIRECTORY ui/ DESTINATION lib/ui)

# ===========================================================================
# Deploy built artifacts to local dist directory
# ===========================================================================
set(DIST_DIR ${CMAKE_CURRENT_BINARY_DIR}/dist)

add_custom_command(TARGET vklayer_understyx_wotlk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}/ui
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}/shaders

    # Copy library and json manifest
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:vklayer_understyx_wotlk> ${DIST_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/VkLayer_understyx_WOTLK.json ${DIST_DIR}/

    # Copy UI directory completely (including fonts)
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/ui ${DIST_DIR}/ui

    # Copy RmlUi libraries
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:rmlui_core> ${DIST_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:rmlui_debugger> ${DIST_DIR}/

    COMMENT "Deploying built layer and assets to ${DIST_DIR}..."
)

# Conditionally copy compiled shaders
if(SPV_COMPILER)
    add_custom_command(TARGET vklayer_understyx_wotlk POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${VERT_SPV} ${FRAG_SPV} ${DIST_DIR}/shaders/
        COMMENT "Deploying shaders to ${DIST_DIR}/shaders/..."
    )
endif()

# ===========================================================================
# Status
# ===========================================================================
message(STATUS "WoTLK Vulkan Layer Configuration")
message(STATUS "  Build type     : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard   : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Vulkan headers : ${VULKAN_INCLUDE_DIRS}")
message(STATUS "  Shader compiler: ${SPV_COMPILER}")
message(STATUS "  Shader output  : ${SHADER_OUT_DIR}")
