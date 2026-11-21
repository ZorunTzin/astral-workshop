param([string]$OutputDirectory)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot
if(!$OutputDirectory){$OutputDirectory=Join-Path $repo 'build/launcher'}
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
dotnet run --project (Join-Path $repo 'launcher-tests/Launcher.Tests.csproj') -c Release
if($LASTEXITCODE -ne 0){throw 'Launcher tests failed'}
dotnet publish (Join-Path $repo 'launcher/AstralWorkshop.csproj') -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -o (Join-Path $OutputDirectory 'app')
if($LASTEXITCODE -ne 0){throw 'Launcher build failed'}
Compress-Archive -Path (Join-Path $OutputDirectory 'app/*') -DestinationPath (Join-Path $OutputDirectory 'AstralWorkshop.zip') -Force
dotnet publish (Join-Path $repo 'launcher-setup/AstralWorkshopSetup.csproj') -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -o (Join-Path $OutputDirectory 'setup')
if($LASTEXITCODE -ne 0){throw 'Installer build failed'}
Get-FileHash (Join-Path $OutputDirectory 'setup/AstralWorkshopSetup.exe') -Algorithm SHA256
