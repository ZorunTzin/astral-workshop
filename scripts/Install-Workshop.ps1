param([string]$GameRoot='B:\D2R\Diablo II Resurrected')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot
if(!(Test-Path -LiteralPath (Join-Path $GameRoot 'D2RLoader.exe'))){throw 'D2RLoader.exe missing'}
$mods=(Resolve-Path -LiteralPath (Join-Path $GameRoot 'Mods')).Path
$name='ReimaginedWorkshop'
$target=Join-Path $mods $name
if(Test-Path -LiteralPath $target){throw "Mod already installed: $target"}
$temp=Join-Path $mods ($name+'-staging-'+[guid]::NewGuid().ToString('N'))
$mpq=New-Item -ItemType Directory -Path (Join-Path $temp "$name.mpq") -Force
Copy-Item -LiteralPath (Join-Path $repo 'data') -Destination $mpq.FullName -Recurse
Copy-Item -LiteralPath (Join-Path $repo 'modinfo.json') -Destination $mpq.FullName
Copy-Item -LiteralPath (Join-Path $repo 'd2rloader') -Destination $temp -Recurse
$src=Join-Path $repo 'd2rloader\plugins\d2rl-astral-workshop.dll'
if((Get-FileHash $src).Hash -ne (Get-FileHash (Join-Path $temp 'd2rloader\plugins\d2rl-astral-workshop.dll')).Hash){throw 'Plugin mismatch'}
if([IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($temp)) -ne $mods -or [IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($target)) -ne $mods){throw 'Unexpected staging path'}
[IO.Directory]::Move($temp,$target)
Write-Output "Installed $target"
