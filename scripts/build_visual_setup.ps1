param([Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference = 'Stop'
$compiler = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if (-not (Test-Path -LiteralPath $compiler)) { throw '.NET Framework 4.x C# compiler is required.' }
$source = Join-Path $PSScriptRoot '..\tools\VisualSetup.cs'
$icon = Join-Path $PSScriptRoot '..\soh\yousei-helper.ico'
& $compiler /nologo /target:winexe /platform:x64 /optimize+ "/out:$Output" "/win32icon:$icon" "/resource:$icon,YouseiFairy" /r:System.Windows.Forms.dll /r:System.Drawing.dll /r:System.Net.Http.dll /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll /r:System.Web.Extensions.dll $source
if ($LASTEXITCODE -ne 0) { throw 'Visual setup compilation failed.' }
