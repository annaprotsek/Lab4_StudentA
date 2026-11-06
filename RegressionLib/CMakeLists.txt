cmake_minimum_required(VERSION 3.20)
project(RegressionLib VERSION 1.0.0 LANGUAGES CXX)

# Етап 1: -DREGRESSION_SHARED=OFF  -> статична бібліотека (.lib)
# Етап 2: -DREGRESSION_SHARED=ON   -> динамічна бібліотека (.dll + імпортна .lib)
option(REGRESSION_SHARED "Збирати як динамічну бібліотеку (DLL)" ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

if(REGRESSION_SHARED)
    add_library(RegressionLib SHARED src/regression.cpp)
    # Макрос лише для збірки самої DLL: dllexport замість dllimport
    target_compile_definitions(RegressionLib PRIVATE REGRESSION_EXPORTS)
else()
    add_library(RegressionLib STATIC src/regression.cpp)
    # Для статичної бібліотеки декоратори dllexport/dllimport не потрібні
    target_compile_definitions(RegressionLib PUBLIC REGRESSION_STATIC)
endif()

# PUBLIC: шлях до include/ автоматично отримає кожен, хто лінкує RegressionLib
target_include_directories(RegressionLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(RegressionLib PUBLIC cxx_std_20)

# Локальний тестовий застосунок (етап 1 і перевірка етапу 2)
add_executable(LocalTest test/main.cpp)
target_link_libraries(LocalTest PRIVATE RegressionLib)

# SDK-пакунок для партнера (лише для DLL):
#   cmake --install build --config Release --prefix ../RegressionSDK
# створить: README.md, include/, lib/ (імпортна .lib), bin/ (.dll)
if(REGRESSION_SHARED)
    install(TARGETS RegressionLib
            RUNTIME DESTINATION bin      # RegressionLib.dll
            ARCHIVE DESTINATION lib      # RegressionLib.lib (імпортна)
            LIBRARY DESTINATION lib)
    install(FILES include/regression.h DESTINATION include)
    install(FILES sdk/README.md DESTINATION .)
endif()
