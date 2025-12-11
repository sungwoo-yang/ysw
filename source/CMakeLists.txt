# author Rudy Castan
# date 2025 Fall
# CS200 Computer Graphics I
# copyright DigiPen Institute of Technology

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(SOURCE_CODE 

    CS200/Image.hpp CS200/Image.cpp
    CS200/ImGuiHelper.hpp CS200/ImGuiHelper.cpp
    CS200/ImmediateRenderer2D.hpp CS200/ImmediateRenderer2D.cpp
    CS200/IRenderer2D.hpp
    CS200/NDC.hpp
    CS200/Renderer2DUtils.hpp CS200/Renderer2DUtils.cpp
    CS200/RenderingAPI.hpp CS200/RenderingAPI.cpp
    CS200/RGBA.hpp

    Engine/Physics/Reflection.hpp Engine/Physics/Reflection.cpp
    Engine/AnimationEditor.hpp Engine/AnimationEditor.cpp
    Engine/Animation.hpp Engine/Animation.cpp
    Engine/Camera.hpp Engine/Camera.cpp
    Engine/Collision.hpp Engine/Collision.cpp
    Engine/Component.hpp
    Engine/ComponentManager.hpp
    Engine/CountdownTimer.hpp Engine/CountdownTimer.cpp
    Engine/Dash.hpp Engine/Dash.cpp
    Engine/Engine.hpp Engine/Engine.cpp
    Engine/Error.hpp
    Engine/Font.hpp Engine/Font.cpp
    Engine/FPS.hpp
    Engine/GameObject.hpp Engine/GameObject.cpp
    Engine/GameObjectManager.hpp Engine/GameObjectManager.cpp
    Engine/GameObjectTypes.hpp
    Engine/GameState.hpp
    Engine/GameStateManager.hpp Engine/GameStateManager.cpp
    Engine/Input.hpp Engine/Input.cpp
    Engine/Logger.hpp Engine/Logger.cpp
    Engine/MapManager.h Engine/MapManager.cpp
    Engine/MapElement.h Engine/MapElement.cpp
    Engine/Matrix.hpp Engine/Matrix.cpp
    Engine/Particle.hpp Engine/Particle.cpp
    Engine/Path.hpp Engine/Path.cpp
    Engine/Polygon.h
    Engine/Random.hpp Engine/Random.cpp
    Engine/Rect.hpp
    Engine/ShowCollision.hpp Engine/ShowCollision.cpp
    Engine/Sprite.hpp Engine/Sprite.cpp
    Engine/Texture.hpp Engine/Texture.cpp
    Engine/TextureManager.hpp Engine/TextureManager.cpp
    Engine/Timer.hpp
    Engine/Vec2.hpp Engine/Vec2.cpp
    Engine/Window.hpp Engine/Window.cpp
    Engine/Bone.hpp
    Engine/Skeleton.hpp Engine/Skeleton.cpp
     

    Game/Bonfire.hpp Game/Bonfire.cpp
    Game/Door.hpp Game/Door.cpp
    Game/MainMenu.hpp Game/MainMenu.cpp
    Game/MiniMap.hpp Game/MiniMap.cpp
    Game/Mode1.hpp Game/Mode1.cpp
    Game/Mode2.hpp Game/Mode2.cpp
    Game/Player.hpp Game/Player.cpp
    Game/RedLaser.hpp Game/RedLaser.cpp
    Game/Shield.hpp Game/Shield.cpp
    Game/Sign.hpp Game/Sign.cpp
    Game/Splash.hpp Game/Splash.cpp
    Game/Star.hpp Game/Star.cpp
    Game/TargetStar.hpp Game/TargetStar.cpp
    Game/WorldTextManager.hpp Game/WorldTextManager.cpp
    Game/YellowLaser.hpp Game/YellowLaser.cpp

    OpenGL/Buffer.hpp OpenGL/Buffer.cpp
    OpenGL/Environment.hpp
    OpenGL/Framebuffer.hpp OpenGL/Framebuffer.cpp
    OpenGL/GL.cpp OpenGL/GL.hpp
    OpenGL/GLConstants.hpp
    OpenGL/GLTypes.hpp
    OpenGL/Handle.hpp
    OpenGL/Shader.cpp OpenGL/Shader.hpp
    OpenGL/Texture.hpp OpenGL/Texture.cpp
    OpenGL/VertexArray.cpp OpenGL/VertexArray.hpp

    main.cpp
)

add_executable(ASTAR ${SOURCE_CODE})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_CODE})

target_link_libraries(ASTAR PRIVATE project_options dependencies)
target_include_directories(ASTAR PRIVATE .)

# Check the IS_DEVELOPER_VERSION cache variable
# This is set by the cmake configure preset
#if (IS_DEVELOPER_VERSION)
#target_compile_definitions(ASTAR PRIVATE DEVELOPER_VERSION)
#endif()

target_compile_definitions(ASTAR PRIVATE
    $<$<CONFIG:Debug>:DEVELOPER_VERSION>
)

if(EMSCRIPTEN)

    # https://emscripten.org/docs/tools_reference/settings_reference.html
    # ASSERTIONS=1                  - we want asserts to work
    # WASM=1                        - we want web assembly generated rather than just javascript
    # DISABLE_EXCEPTION_CATCHING=0  - we are using exceptions so let's not disable them
    # ALLOW_MEMORY_GROWTH=1         - we don't know exactly how much heap space we'll need so allow it to grow
    # EXIT_RUNTIME=1                - have exiting actually stop the program
    # SINGLE_FILE=1                 - generate everything into one html file
    # --embed-file                  - https://emscripten.org/docs/tools_reference/emcc.html#emcc-embed-file
    # --use-preload-cache           - help with faster reloads : https://emscripten.org/docs/compiling/Deploying-Pages.html#providing-a-quick-second-time-load
    # -lembind                      - to call c++ from javascript https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html
    # --shell-file                  - to customize the webpage https://emscripten.org/docs/compiling/Deploying-Pages.html#build-files-and-custom-shell
    target_link_options(ASTAR PRIVATE 
    -sASSERTIONS=$<$<BOOL:${IS_DEVELOPER_VERSION}>:1>$<$<NOT:$<BOOL:${IS_DEVELOPER_VERSION}>>:0>
    -sWASM=1 
    -sDISABLE_EXCEPTION_CATCHING=0 
    -sALLOW_MEMORY_GROWTH=1 
    -sEXIT_RUNTIME=1 
    -sSINGLE_FILE=1 
    --embed-file ${CMAKE_SOURCE_DIR}/Assets@/Assets
    --use-preload-cache
    -lembind
    --shell-file ${CMAKE_SOURCE_DIR}/app_resources/web/index_shell.html
    )

    set_target_properties(ASTAR PROPERTIES SUFFIX ".html")
elseif(WIN32)

    target_link_options(ASTAR PRIVATE 
        $<IF:$<BOOL:${IS_DEVELOPER_VERSION}>,/SUBSYSTEM:CONSOLE,/SUBSYSTEM:WINDOWS>
        /ENTRY:mainCRTStartup)

    set(ICON_RC ${CMAKE_CURRENT_BINARY_DIR}/app.rc)
    if(NOT EXISTS ${ICON_RC})
        set(ICON_FILE ${CMAKE_SOURCE_DIR}/app_resources/windows/icon.ico)
        file(WRITE ${ICON_RC} "IDR_MAINFRAME ICON \"${ICON_FILE}\"\nIDI_ICON1 ICON DISCARDABLE \"${ICON_FILE}\"")
    endif()

    target_sources(ASTAR PRIVATE ${ICON_RC})

endif()
