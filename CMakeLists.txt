cmake_minimum_required(VERSION 3.29)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()
project(cuda_nbody LANGUAGES C CXX CUDA)  # C needed by glad2's cmake subproject

# ── Language standards ────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD  23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 20)  # nvcc lags one cycle; 20 is the max CUDA 13.2 exposes
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# RTX 3060 Laptop = compute capability 8.6 (Ampere)
set(CMAKE_CUDA_ARCHITECTURES 86)

# Separable compilation lets the linker resolve device-side symbols across .cu files
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)

# ── System dependencies ───────────────────────────────────────────────────────
find_package(CUDAToolkit REQUIRED)   # provides CUDA::cudart
find_package(OpenGL     REQUIRED)

# ── Third-party via FetchContent (no vcpkg, no manual installs) ───────────────
include(FetchContent)

# GLFW 3.4
FetchContent_Declare(glfw3
GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw3)

# GLAD v2 — generates an OpenGL 4.6 core loader at configure time via Python.
# glad2 requires the Python package 'jinja2'. Install it automatically if absent.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import jinja2"
    RESULT_VARIABLE _jinja2_found
    ERROR_QUIET OUTPUT_QUIET
)
if(NOT _jinja2_found EQUAL 0)
message(STATUS "jinja2 not found — installing via pip (one-time, required by glad2)...")
execute_process(
    COMMAND ${Python3_EXECUTABLE} -m pip install jinja2 --quiet
    RESULT_VARIABLE _pip_result
    )
    if(NOT _pip_result EQUAL 0)
    message(FATAL_ERROR "pip install jinja2 failed. Run manually: pip install jinja2")
    endif()
endif()
# glad2's repo has NO root CMakeLists.txt; its cmake support lives in cmake/.
# FetchContent_Populate downloads without calling add_subdirectory, so we can
# point to cmake/ ourselves.  (CMP0169 deprecates this form in CMake ≥3.30;
# on 3.29 this is fine with no policy override needed.)
FetchContent_Declare(glad2
GIT_REPOSITORY https://github.com/Dav1dde/glad.git
GIT_TAG        v2.0.6
GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(glad2)
if(NOT glad2_POPULATED)
    FetchContent_Populate(glad2)
    endif()

    # cmake/CMakeLists.txt defines glad_add_library() — a CMake function that runs
# the glad2 Python generator to produce a .c loader + headers for our spec.
add_subdirectory("${glad2_SOURCE_DIR}/cmake" glad_cmake)
glad_add_library(glad STATIC REPRODUCIBLE LOADER API gl:core=4.6)

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(cuda_nbody
src/main.cu
src/renderer.cpp
src/galaxy.cpp
src/nbody.cu
)

target_compile_definitions(cuda_nbody PRIVATE
    $<IF:$<CONFIG:Debug>,NBODY_DEBUG,>
)

set_target_properties(cuda_nbody PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES         86
)

target_include_directories(cuda_nbody PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(cuda_nbody PRIVATE
    OpenGL::GL
    glfw
    glad
    CUDA::cudart
)

# opengl32 is already pulled in transitively by OpenGL::GL on Windows,
# but listing it explicitly silences some linker warnings.
if(WIN32)
    target_link_libraries(cuda_nbody PRIVATE opengl32)
endif()

# If nvcc rejects your MSVC version, uncomment the next line (see .claude/DEBUGGING.md).
# set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -allow-unsupported-compiler")

# ── POST-BUILD: copy shaders next to the executable ──────────────────────────
# Shader paths in code are resolved relative to argv[0], not the working directory.
# This prevents the "shaders not found" bug when running from VS Code / a debugger.
add_custom_command(TARGET cuda_nbody POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:cuda_nbody>/shaders"
    COMMENT "Copying shaders → $<TARGET_FILE_DIR:cuda_nbody>/shaders"
)

