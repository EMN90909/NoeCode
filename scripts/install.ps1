$ErrorActionPreference = 'Stop'
$build = if ($env:BUILD_DIR) { $env:BUILD_DIR } else { 'build' }
cmake -S . -B $build -DCMAKE_BUILD_TYPE=Release
cmake --build $build --config Release --parallel
if ($env:PREFIX) {
  cmake --install $build --config Release --prefix $env:PREFIX
} else {
  cmake --install $build --config Release
}
