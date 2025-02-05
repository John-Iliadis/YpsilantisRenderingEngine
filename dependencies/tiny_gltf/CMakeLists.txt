add_library(tiny_gltf STATIC src/tiny_gltf.cc)

target_include_directories(tiny_gltf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_definitions(tiny_gltf
        PRIVATE
        TINYGLTF_IMPLEMENTATION

        PUBLIC
        TINYGLTF_USE_CPP14
        TINYGLTF_NO_EXTERNAL_IMAGE
        TINYGLTF_NO_STB_IMAGE
        TINYGLTF_NO_STB_IMAGE_WRITE
)
