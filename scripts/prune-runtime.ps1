# Silicon LogiX / SLX SynthForge
# Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
# Author: Marco Pezzullo
# License: Silicon LogiX Evaluation License 1.0. See LICENSE.

# Remove unused Qt styles from a generated bundle. The UI selects Basic.
param([Parameter(Mandatory=$true)][string]$BundleDirectory)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$bundleRoot=[IO.Path]::GetFullPath($BundleDirectory)
$workspacePrefix=$projectRoot.TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
if(-not $bundleRoot.StartsWith($workspacePrefix,[StringComparison]::OrdinalIgnoreCase)){throw 'Runtime pruning is limited to a generated bundle in this workspace'}
if(-not (Test-Path -LiteralPath "$bundleRoot/slx-synthforge.exe")){throw 'SynthForge bundle executable not found'}
$bundlePrefix=$bundleRoot.TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
function AssertBundleTarget([string]$Candidate) {
    $resolved=[IO.Path]::GetFullPath($Candidate)
    if(-not $resolved.StartsWith($bundlePrefix,[StringComparison]::OrdinalIgnoreCase)){throw "Invalid bundle target: $resolved"}
    $segment=$resolved
    while($segment.Length -ge $bundleRoot.Length) {
        if((Test-Path -LiteralPath $segment) -and ((Get-Item -LiteralPath $segment).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw "Linked runtime path is not allowed: $segment"}
        if($segment -eq $bundleRoot){break}
        $segment=Split-Path -Parent $segment
    }
    if((Test-Path -LiteralPath $resolved -PathType Container) -and (Get-ChildItem -LiteralPath $resolved -Recurse -Force | Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint})){throw 'Runtime target contains linked entries'}
    return $resolved
}
foreach($style in @('FluentWinUI3','Fusion','Imagine','Material','Universal','Windows')) {
    $directory=AssertBundleTarget (Join-Path $bundleRoot "qml/QtQuick/Controls/$style")
    if(Test-Path -LiteralPath $directory){Remove-Item -LiteralPath $directory -Recurse -Force}
    foreach($library in Get-ChildItem -LiteralPath $bundleRoot -Filter "Qt6QuickControls2${style}*.dll" -File){Remove-Item -LiteralPath (AssertBundleTarget $library.FullName) -Force}
}
# Use native OpenGL 3.3 for the editor, or Qt's software scene graph when unavailable.
$openGlFallback=AssertBundleTarget (Join-Path $bundleRoot 'opengl32sw.dll')
if(Test-Path -LiteralPath $openGlFallback){Remove-Item -LiteralPath $openGlFallback -Force}
