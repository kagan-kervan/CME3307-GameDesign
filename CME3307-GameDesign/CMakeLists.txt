cmake_minimum_required(VERSION 3.10.0)
project(SpaceOut VERSION 0.1.0 LANGUAGES C CXX)

add_executable(SpaceOut AlienSprite.cpp Background.cpp Bitmap.cpp GameEngine.cpp SpaceOut.cpp Sprite.cpp)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
