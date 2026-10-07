cmake_minimum_required(VERSION 3.20)
project(inventory_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(Crow GIT_REPOSITORY https://github.com/CrowCpp/Crow.git GIT_TAG v1.2.1)
FetchContent_MakeAvailable(Crow)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)
find_package(SQLite3 REQUIRED)

add_executable(inventory-backend src/main.cpp src/database/database.cpp src/services/stock_service.cpp)
target_link_libraries(inventory-backend PRIVATE Crow::Crow nlohmann_json::nlohmann_json SQLite::SQLite3)
target_compile_options(inventory-backend PRIVATE -Wall -Wextra -Wpedantic)
