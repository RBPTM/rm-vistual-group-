cmake_minimum_required(VERSION 3.16)
project(ubuntu22_opencv_armor_lab VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc imgcodecs videoio highgui)

add_executable(armor_lab
    src/main.cpp
    src/armor_detector.cpp
)

target_include_directories(armor_lab PRIVATE include ${OpenCV_INCLUDE_DIRS})
target_link_libraries(armor_lab PRIVATE ${OpenCV_LIBS})
target_compile_options(armor_lab PRIVATE -Wall -Wextra -Wpedantic)

add_executable(image_toolbox src/image_toolbox.cpp)
target_include_directories(image_toolbox PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(image_toolbox PRIVATE ${OpenCV_LIBS})
target_compile_options(image_toolbox PRIVATE -Wall -Wextra -Wpedantic)
