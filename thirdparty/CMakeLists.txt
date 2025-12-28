# Adding some hardcoded thirdparty libs

add_subdirectory(tinygltf)
file(CREATE_LINK "${CMAKE_CURRENT_LIST_DIR}/tinygltf/json.hpp" "${CMAKE_CURRENT_LIST_DIR}/tinygltf/tinygltf/json.hpp" COPY_ON_ERROR)
file(CREATE_LINK "${CMAKE_CURRENT_LIST_DIR}/tinygltf/stb_image.h" "${CMAKE_CURRENT_LIST_DIR}/tinygltf/tinygltf/stb_image.h" COPY_ON_ERROR)
file(CREATE_LINK "${CMAKE_CURRENT_LIST_DIR}/tinygltf/stb_image_write.h" "${CMAKE_CURRENT_LIST_DIR}/tinygltf/tinygltf/stb_image_write.h" COPY_ON_ERROR)
file(CREATE_LINK "${CMAKE_CURRENT_LIST_DIR}/tinygltf/tiny_gltf.h" "${CMAKE_CURRENT_LIST_DIR}/tinygltf/tinygltf/tiny_gltf.h" COPY_ON_ERROR)
include_directories(tinygltf)

# Disable warnings from thirdparty libs
if (MSVC)
    target_compile_options(tinygltf INTERFACE /WX-)
else()
    target_compile_options(tinygltf INTERFACE -Wno-error)
endif()


add_library(thirdparty::tinygltf ALIAS tinygltf)
