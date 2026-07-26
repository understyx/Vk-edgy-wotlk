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
    GIT_TAG        6.2
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
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

FetchContent_MakeAvailable(RmlUi json)

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
    src/vklayer/discord_presence.cpp
    src/rmlui/SystemInterface.cpp
    src/rmlui/FileInterface.cpp
    src/rmlui/VulkanRenderer.cpp
    src/rmlui/OverlayUI.cpp
    src/rmlui/WowDataModel.cpp
    src/wowmemory/memory_utils.cpp
    src/wowmemory/auras.cpp
    src/wowmemory/unit_info.cpp
    src/wowmemory/combat_log.cpp
    src/wowmemory/wowmemory.cpp
    src/wowmemory/object_manager.cpp
    src/wowmemory/objects/wow_object.cpp
    src/wowmemory/objects/wow_unit.cpp
    src/wowmemory/objects/wow_player.cpp
    src/wowmemory/objects/wow_game_object.cpp
    src/wowmemory/objects/wow_item.cpp
    src/wowmemory/objects/wow_container.cpp
    src/wowmemory/objects/wow_dynamic_object.cpp
    src/wowmemory/objects/wow_corpse.cpp
    src/wowmemory/client/c_data_store.cpp
    src/wowmemory/client/wow_client_services.cpp
    src/wowmemory/client/wow_console.cpp
    src/wowmemory/client/wow_world.cpp
    src/wowmemory/client/wow_camera.cpp
    src/wowmemory/client/helper.cpp
    src/wowmemory/client/wow_party.cpp
    src/wowmemory/client/wow_raid.cpp
    src/wowmemory/client/wow_quest.cpp
)

add_library(vklayer_understyx_wotlk SHARED ${VKLAYER_SOURCES})

# Depend on shader compilation if available
if(TARGET RmlUiShaders)
    add_dependencies(vklayer_understyx_wotlk RmlUiShaders)
endif()

target_link_libraries(vklayer_understyx_wotlk
    PRIVATE
        RmlUi::Core
        nlohmann_json::nlohmann_json
        dl
        $<$<BOOL:${Vulkan_FOUND}>:${VULKAN_LIBRARIES}>
        $<$<BOOL:${LUA_FOUND}>:RmlUi::Lua>
        $<$<BOOL:${LUA_FOUND}>:${LUA_LIBRARIES}>
)

target_include_directories(vklayer_understyx_wotlk
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/objects
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
# 32-bit Windows DLL for WoW process memory reading
# ===========================================================================
find_program(MINGW_GXX i686-w64-mingw32-g++)

if(MINGW_GXX)
    add_custom_target(WowMemoryDLL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/dist
        # Compile wowmemory.dll
        COMMAND ${MINGW_GXX} -std=c++20 -shared -fPIC -O2
            -static
            -static-libgcc
            -static-libstdc++
            -I${CMAKE_CURRENT_SOURCE_DIR}/include
            -I${CMAKE_CURRENT_SOURCE_DIR}/src
            -I${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory
            -I${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/objects
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory_dll/wowmemory_dll.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/memory_utils.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/auras.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/unit_info.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/combat_log.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/wowmemory.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/object_manager.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/objects/wow_object.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/objects/wow_unit.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/objects/wow_player.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/objects/wow_game_object.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/objects/wow_item.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/objects/wow_container.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/objects/wow_dynamic_object.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/objects/wow_corpse.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/client/c_data_store.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/client/wow_client_services.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/client/wow_console.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/client/wow_world.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/client/wow_camera.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/client/helper.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/client/wow_party.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/client/wow_raid.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory/client/wow_quest.cpp
            -lws2_32
            -o ${CMAKE_CURRENT_BINARY_DIR}/dist/wowmemory.dll
        # Compile injector.exe
        COMMAND ${MINGW_GXX} -std=c++20 -O2
            -static
            -static-libgcc
            -static-libstdc++
            ${CMAKE_CURRENT_SOURCE_DIR}/src/wowmemory_dll/injector.cpp
            -o ${CMAKE_CURRENT_BINARY_DIR}/dist/injector.exe
        COMMENT "Compiling 32-bit Windows DLL (wowmemory.dll) and Injector (injector.exe)"
    )
    add_dependencies(vklayer_understyx_wotlk WowMemoryDLL)
else()
    message(WARNING "i686-w64-mingw32-g++ compiler not found. wowmemory.dll and injector.exe will not be built.")
endif()

# ===========================================================================
# Deploy built artifacts to local dist directory
# ===========================================================================
set(DIST_DIR ${CMAKE_CURRENT_BINARY_DIR}/dist)

add_custom_command(TARGET vklayer_understyx_wotlk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}/ui
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}/shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}/docs

    # Copy library and json manifest
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:vklayer_understyx_wotlk> ${DIST_DIR}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/VkLayer_understyx_WOTLK.json ${DIST_DIR}/

    # Copy UI directory completely (including fonts)
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/ui ${DIST_DIR}/ui

    # Copy docs directory completely
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/docs ${DIST_DIR}/docs

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
