$ErrorActionPreference = "Stop"
$Noe = $env:NOE_BIN
if (-not $Noe) { $Noe = ".\build\noe.exe" }
New-Item -ItemType Directory -Force -Path build | Out-Null

& $Noe --version
if ($LASTEXITCODE -ne 0) { throw "version failed" }

& $Noe check examples/native_hello.noe
if ($LASTEXITCODE -ne 0) { throw "check failed" }

& $Noe nir examples/native_hello.noe | Set-Content build/native_hello.nir
if ($LASTEXITCODE -ne 0) { throw "nir failed" }

& $Noe run examples/native_hello.noe | Set-Content build/interpreter.out
if ($LASTEXITCODE -ne 0) { throw "run failed" }
$expected = Get-Content examples/native_hello.expected -Raw
$actual = Get-Content build/interpreter.out -Raw
if ($expected -ne $actual) { throw "interpreter output did not match expected output" }

Set-Content build/type_error.noe 'let value: int = "wrong"'
& $Noe check build/type_error.noe *> build/type_error.out
if ($LASTEXITCODE -eq 0) { throw "expected type checker to reject invalid program" }
Select-String -Path build/type_error.out -Pattern 'NOE-T3001' | Out-Null

Set-Content build/const_error.noe "const value = 1`nvalue = 2`n"
& $Noe check build/const_error.noe *> build/const_error.out
if ($LASTEXITCODE -eq 0) { throw "expected type checker to reject const reassignment" }
Select-String -Path build/const_error.out -Pattern 'NOE-T3014' | Out-Null

& $Noe test tests
if ($LASTEXITCODE -ne 0) { throw "test runner failed" }

Set-Content build/format.noe "let   x=1+2;`nprint(x);`n"
& $Noe format build/format.noe
if ($LASTEXITCODE -ne 0) { throw "format failed" }
Select-String -Path build/format.noe -Pattern 'let x = 1 + 2;' | Out-Null

Remove-Item -Recurse -Force build/smoke_project -ErrorAction SilentlyContinue
& $Noe new smoke build/smoke_project
if ($LASTEXITCODE -ne 0) { throw "new project failed" }
& $Noe manifest build/smoke_project/project.noe | Set-Content build/manifest.out
if ($LASTEXITCODE -ne 0) { throw "manifest failed" }
& $Noe lock build/smoke_project/project.noe
if ($LASTEXITCODE -ne 0) { throw "lock failed" }
if (-not (Test-Path build/smoke_project/noe.lock)) { throw "noe.lock missing" }
Select-String -Path build/smoke_project/noe.lock -Pattern '^noe-lock 1$' | Out-Null

& $Noe doctor .
if ($LASTEXITCODE -ne 0) { throw "doctor failed" }

Write-Host "Noe Windows/macOS core-compatible tests passed"
