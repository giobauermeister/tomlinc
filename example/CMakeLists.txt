# Add the example executable
add_executable(parse_toml_file example.c)

# Link the library to the example
target_link_libraries(parse_toml_file tomlinc)

# Include the library's include directory
target_include_directories(parse_toml_file PRIVATE ${CMAKE_SOURCE_DIR}/include)
