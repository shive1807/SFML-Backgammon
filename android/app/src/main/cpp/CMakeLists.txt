cmake_minimum_required(VERSION 3.22.1)

project(SFMLBackgammonAndroid LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../.." ABSOLUTE)

set(SFML_APP_SOURCES
	"${PROJECT_ROOT}/src/main.cpp"
	"${PROJECT_ROOT}/src/engine/Engine.cpp"
	"${PROJECT_ROOT}/src/backgammon/BackgammonGame.cpp"
	"${PROJECT_ROOT}/src/backgammon/BackgammonGameInput.cpp"
	"${PROJECT_ROOT}/src/backgammon/BackgammonGameUpdate.cpp"
	"${PROJECT_ROOT}/src/backgammon/BackgammonGameMoves.cpp"
	"${PROJECT_ROOT}/src/backgammon/BackgammonGameRender.cpp"
)

add_library(sfml-app SHARED ${SFML_APP_SOURCES})

target_include_directories(sfml-app PRIVATE "${PROJECT_ROOT}/src")

find_package(SFML 3 CONFIG REQUIRED COMPONENTS Graphics Window System Main)

target_link_libraries(sfml-app
	PRIVATE
		SFML::Main
		SFML::Graphics
)
