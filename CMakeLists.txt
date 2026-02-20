cmake_minimum_required(VERSION 3.16)
project(SFMLApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# You can override build type via: -DCMAKE_BUILD_TYPE=Release
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

add_executable(SFMLApp
	src/main.cpp
	src/engine/Engine.cpp
	src/backgammon/BackgammonGame.cpp
	src/backgammon/BackgammonGameInput.cpp
	src/backgammon/BackgammonGameUpdate.cpp
	src/backgammon/BackgammonGameMoves.cpp
	src/backgammon/BackgammonGameRender.cpp
)

# Tip for macOS/Homebrew users:
# If CMake can't find SFML, pass one of:
#   -DSFML_DIR=$(brew --prefix sfml)/lib/cmake/SFML
#   -DCMAKE_PREFIX_PATH=$(brew --prefix sfml)
#
# You can also set SFML_DIR in your environment before configuring.
find_package(SFML 3 CONFIG COMPONENTS Graphics Window System REQUIRED)

target_link_libraries(SFMLApp
	PRIVATE
		SFML::Graphics
)

# On Apple platforms, ensure bundles can find dylibs during development
if(APPLE)
	# Embed rpath to look next to executable and in build tree's lib dirs
	set_target_properties(SFMLApp PROPERTIES
		MACOSX_BUNDLE OFF
		BUILD_WITH_INSTALL_RPATH ON
		INSTALL_RPATH "@executable_path/../lib;@loader_path"
	)
endif()

