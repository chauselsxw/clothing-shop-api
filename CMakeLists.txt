cmake_minimum_required(VERSION 3.20)
project(clothing_shop CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Visual Studio: читать исходники как UTF-8 (в комментариях русские буквы)
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Библиотеки (их ставит vcpkg по файлу vcpkg.json)
find_package(libpqxx CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# shop_core - весь код приложения, кроме main.cpp.
# Его используют и сервер, и тесты.
add_library(shop_core STATIC
    src/config.cpp
    src/migrations.cpp
    src/repository.cpp
    src/rules.cpp
    src/service.cpp
    src/routes.cpp
)
target_include_directories(shop_core PUBLIC src)
target_link_libraries(shop_core PUBLIC
    libpqxx::pqxx
    httplib::httplib
    nlohmann_json::nlohmann_json
)
# Путь к папке проекта, чтобы программа находила .env и папку migrations
target_compile_definitions(shop_core PRIVATE PROJECT_DIR="${CMAKE_SOURCE_DIR}")
if(WIN32)
    target_compile_definitions(shop_core PUBLIC _WIN32_WINNT=0x0A00)
endif()

# Сервер
add_executable(shop_api src/main.cpp)
target_link_libraries(shop_api PRIVATE shop_core)

# Тесты
enable_testing()

add_executable(unit_tests tests/unit_tests.cpp)
target_link_libraries(unit_tests PRIVATE shop_core GTest::gtest GTest::gtest_main)
add_test(NAME unit_tests COMMAND unit_tests)

add_executable(integration_tests tests/integration_tests.cpp)
target_link_libraries(integration_tests PRIVATE shop_core GTest::gtest GTest::gtest_main)
add_test(NAME integration_tests COMMAND integration_tests)
