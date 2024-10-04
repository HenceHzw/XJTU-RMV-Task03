cmake_minimum_required(VERSION 3.0.0)
project(task)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
find_package(OpenCV REQUIRED)

find_package(Eigen3 3.3 REQUIRED NO_MODULE)

find_package(Ceres REQUIRED)

include_directories(include
${OpenCV_INCLUDE_DIRS}
${EIGEN3_INCLUDE_DIR}
${CERES_INCLUDE_DIRS}
)

aux_source_directory(./src ALL_SRCS)
set(SRC ./cc)

add_executable(task main.cpp ${ALL_SRCS})
target_link_libraries(task 
${OpenCV_LIBS}
${CERES_LIBRARIES}
Eigen3::Eigen 
)