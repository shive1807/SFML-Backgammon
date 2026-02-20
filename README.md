# SFML Starter (CMake)

Minimal SFML project scaffold using CMake.

## Prerequisites (macOS)

1) Install Homebrew if needed: https://brew.sh/
2) Install SFML:

```bash
brew install sfml
```

## Build

Configure and build (Debug by default):

```bash
mkdir -p build
cd build
# Option A: Let CMake discover SFML automatically (works if SFML is in default Homebrew paths)
cmake ..
cmake --build . --config Debug

# Option B: Provide an explicit hint if discovery fails
# cmake -DSFML_DIR="$(brew --prefix sfml)/lib/cmake/SFML" ..
# cmake --build . --config Debug
```

Run the app:

```bash
./SFMLApp
```

## Notes

- You can switch to Release:

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

- If you use a non-standard install location, either set `SFML_DIR` to the `lib/cmake/SFML` folder or add the prefix path:

```bash
cmake -DSFML_DIR="/path/to/sfml/lib/cmake/SFML" ..
# or
cmake -DCMAKE_PREFIX_PATH="/path/to/sfml" ..
```

