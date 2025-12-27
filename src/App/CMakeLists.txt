set(SRCS
    main.cpp
    Window.cpp
    Window.h
    Camera.cpp
    Camera.h
    Model.cpp
    Model.h

    Shaders/diffuse.fs
    Shaders/diffuse.vs
)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

qt_add_big_resources(RES resources.qrc)

add_executable(demo-app ${SRCS} ${RES})

target_link_libraries(demo-app
    PRIVATE
        Qt5::Widgets
        FGL::Base
        thirdparty::tinygltf
)

add_custom_target(run
    COMMAND demo-app
)