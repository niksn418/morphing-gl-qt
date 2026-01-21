set(SRCS
    main.cpp
    Window.cpp
    WindowSettings.cpp
    Window.h
    Camera.cpp
    Camera.h
    Lighting.cpp
    Lighting.h
    Model.cpp
    Model.h
    Quad.cpp
    Quad.h
    SSAO.cpp
    SSAO.h
    uniform_types.h
    utils.h

    Shaders/model.vs
    Shaders/model.fs
    Shaders/noop.vs
    Shaders/ssao.fs
    Shaders/diffuse.fs
)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

qt_add_big_resources(RES resources.qrc)

add_executable(morphing ${SRCS} ${RES})

target_link_libraries(morphing
    PRIVATE
        Qt5::Widgets
        FGL::Base
        thirdparty::tinygltf
)

add_custom_target(run
    COMMAND morphing
)