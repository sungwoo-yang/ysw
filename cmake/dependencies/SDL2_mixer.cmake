# We will be using SDL2_mixer for audio playback and mixing.
# Linux/Mac platforms have a very easy way to install these dependencies and expose them to compilers so,
#   we will use the builtin find_package
# Emscripten has their own port of SDL2_mixer builtin. We can just define --use-port=sdl2_mixer, and start using it.
# Windows does not have a simple way to get it, so we download official windows binaries and link against those

add_library(the_sdl2_mixer INTERFACE)

if(WIN32)
    # download binaries for SDL2_mixer for windows x64
    FetchContent_Declare(
        sdl2_mixer
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        URL https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.0/SDL2_mixer-devel-2.8.0-VC.zip
    )
    FetchContent_MakeAvailable(sdl2_mixer)

    target_include_directories(the_sdl2_mixer SYSTEM INTERFACE ${sdl2_mixer_SOURCE_DIR}/include)
    target_link_directories(the_sdl2_mixer INTERFACE ${sdl2_mixer_SOURCE_DIR}/lib/x64)
    target_link_libraries(the_sdl2_mixer INTERFACE SDL2_mixer)

    # Define a custom target to copy SDL2_mixer.dll and other audio codec DLLs to the build directory
    set(TEMP_EXE_FOLDER $<IF:$<BOOL:${CMAKE_RUNTIME_OUTPUT_DIRECTORY}>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY},${CMAKE_BINARY_DIR}>)
    
    add_custom_target(copy_sdl2_mixer_dlls
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${sdl2_mixer_SOURCE_DIR}/lib/x64/
            ${TEMP_EXE_FOLDER}/
        COMMENT "Copying SDL2_mixer DLLs to executable directory"
    )
    add_dependencies(the_sdl2_mixer copy_sdl2_mixer_dlls)

else()
    if(EMSCRIPTEN)
        # --use-port=sdl2_mixer - we want to use the built-in sdl2_mixer port
        target_compile_options(the_sdl2_mixer INTERFACE --use-port=sdl2_mixer)
        target_link_options(the_sdl2_mixer INTERFACE --use-port=sdl2_mixer)
    else()
        # on    Mac : brew install sdl2_mixer
        # on Ubuntu : apt install libsdl2-mixer-dev
        find_package(SDL2_mixer REQUIRED)
        target_include_directories(the_sdl2_mixer SYSTEM INTERFACE ${SDL2_MIXER_INCLUDE_DIRS})
        target_link_libraries(the_sdl2_mixer INTERFACE ${SDL2_MIXER_LIBRARIES})
    endif()
endif()