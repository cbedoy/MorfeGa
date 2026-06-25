cmake_minimum_required(VERSION 3.22.1)

project("morfega")

add_library(morfega SHARED
    main.cpp
    AndroidOut.cpp
    Renderer.cpp
    Shader.cpp
    TextureAsset.cpp
    Utility.cpp
    Camera.cpp
    Player.cpp
    Chunk.cpp
    ChunkManager.cpp
    PerlinNoise.cpp
)

find_package(game-activity REQUIRED CONFIG)

set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")

target_link_libraries(morfega
    game-activity::game-activity_static
    EGL
    GLESv3
    jnigraphics
    android
    log)
