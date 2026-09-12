$ErrorActionPreference = 'Stop'

$Fox = @'
        /\       /\
       /  \_____/  \
      /   / </> \   \
     /   /   ^   \   \
    (    \  ___  /    )
     \    '.___.'    /
      '._         _.'
         '-.___.-'
           NOQERI
'@
Write-Host $Fox

$BuildDir = if ($env:BUILD_DIR) { $env:BUILD_DIR } else { 'build' }
$BuildType = if ($env:BUILD_TYPE) { $env:BUILD_TYPE } else { 'Release' }
Write-Host "building Noqeri bootstrap ($BuildType)"
cmake -S . -B $BuildDir -DCMAKE_BUILD_TYPE=$BuildType
cmake --build $BuildDir --config $BuildType --parallel
Write-Host "noqeri bootstrap built in $BuildDir"
