# Silicon LogiX / SLX SynthForge
# Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
# Author: Marco Pezzullo
# License: Silicon LogiX Evaluation License 1.0. See LICENSE.

# Build the portable demonstration with replaceable runtimes and corresponding sources.
param(
    [string]$QtRoot='C:/Qt/6.12.0/mingw_64',
    [string]$ToolchainBin='C:/Qt/Tools/mingw1310_64/bin',
    [string]$BuildDirectory='build/windows-mingw',
    [string]$OutputDirectory='dist/SLX-SynthForge-0.4.1',
    [string]$DatasetDirectory,
    [string]$RuntimeSourceDirectory='dist/runtime-sources-0.4',
    [switch]$CreateArchive
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$projectRoot=[IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
function ProjectPath([string]$Path) {
    if([IO.Path]::IsPathRooted($Path)){return [IO.Path]::GetFullPath($Path)}
    return [IO.Path]::GetFullPath((Join-Path $projectRoot $Path))
}
function WorkspaceTarget([string]$Path) {
    $resolved=ProjectPath $Path
    $prefix=$projectRoot.TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
    if(-not $resolved.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)){throw "Package output must stay inside the workspace: $resolved"}
    return $resolved
}
$buildPath=ProjectPath $BuildDirectory
$outputPath=WorkspaceTarget $OutputDirectory
if(Test-Path -LiteralPath $outputPath){throw "Package already exists. Choose a new OutputDirectory: $outputPath"}
$previousPath=$env:PATH
try {
    $env:PATH="$QtRoot/bin;$ToolchainBin;$previousPath"
    New-Item -ItemType Directory -Path $outputPath -Force | Out-Null
    foreach($name in @('slx-synthforge.exe','SiliconLogiX.SynthForge.Core.dll')) {
        $source=Join-Path $buildPath $name
        if(-not (Test-Path -LiteralPath $source)){throw "Missing build artifact: $source"}
        Copy-Item -LiteralPath $source -Destination $outputPath
        & "$ToolchainBin/strip.exe" --strip-unneeded (Join-Path $outputPath $name)
        if($LASTEXITCODE -ne 0){throw 'Binary stripping failed'}
    }
    & "$QtRoot/bin/windeployqt.exe" --release --compiler-runtime --no-translations --no-system-d3d-compiler --no-opengl-sw --skip-plugin-types qmltooling,generic --include-plugins qsvg,qsvgicon --verbose 0 --qmldir "$projectRoot/qml" --dir $outputPath "$outputPath/slx-synthforge.exe"
    if($LASTEXITCODE -ne 0){throw 'Qt deployment failed'}
    & "$PSScriptRoot/prune-runtime.ps1" -BundleDirectory $outputPath
    New-Item -ItemType Directory -Path "$outputPath/tools","$outputPath/licenses","$outputPath/docs","$outputPath/assets/screenshots","$outputPath/assets/models","$outputPath/SLX_SynthForge_PowerBoard/scene" -Force | Out-Null
    foreach($name in @('train.py','train-inspector.py','reporting.py','requirements.txt')){Copy-Item -LiteralPath "$projectRoot/tools/$name" -Destination "$outputPath/tools"}
    foreach($name in @('README.md','LICENSE','THIRD_PARTY_NOTICES.md','THIRD_PARTY_SOURCE.md')){Copy-Item -LiteralPath "$projectRoot/$name" -Destination $outputPath}
    Get-ChildItem "$projectRoot/docs" -Filter '*.md' | Copy-Item -Destination "$outputPath/docs"
    Copy-Item -LiteralPath "$projectRoot/licenses/Qt-Open-Source-LICENSE.txt" -Destination "$outputPath/licenses"
    $runtimeLicenses=@{'GCC-GPLv3.txt'='gcc/COPYING3';'GCC-runtime-exception.txt'='gcc/COPYING.RUNTIME';'Winpthreads-LICENSE.txt'='winpthreads/COPYING';'MinGW-w64-runtime-LICENSE.txt'='mingw-w64/COPYING.MinGW-w64-runtime.txt'}
    $toolchainRoot=Split-Path -Parent $ToolchainBin
    foreach($name in $runtimeLicenses.Keys){Copy-Item -LiteralPath "$toolchainRoot/licenses/$($runtimeLicenses[$name])" -Destination "$outputPath/licenses/$name"}
    foreach($sbom in Get-ChildItem "$QtRoot/sbom" -Filter '*.spdx' -ErrorAction SilentlyContinue | Where-Object Name -Match '^(qtbase|qtdeclarative|qtsvg|qtshadertools)-'){Copy-Item -LiteralPath $sbom.FullName -Destination "$outputPath/licenses"}
    Copy-Item -LiteralPath "$projectRoot/assets/models/assembly-inspector.json" -Destination "$outputPath/assets/models"
    foreach($name in @('editor.png','inspection.png','assembly-ai.png','about.png')){Copy-Item -LiteralPath "$projectRoot/assets/screenshots/$name" -Destination "$outputPath/assets/screenshots"}
    Copy-Item -LiteralPath "$projectRoot/SLX_SynthForge_PowerBoard/README.md" -Destination "$outputPath/SLX_SynthForge_PowerBoard"
    foreach($name in @('nominal','missing_C1','misaligned_U4','reversed_C1','solder_bridge_U4')){Copy-Item -LiteralPath "$projectRoot/SLX_SynthForge_PowerBoard/scene/SLX_PowerBoard_$name.glb" -Destination "$outputPath/SLX_SynthForge_PowerBoard/scene"}
    Copy-Item -LiteralPath "$projectRoot/SLX_SynthForge_PowerBoard/scene/instances.json" -Destination "$outputPath/SLX_SynthForge_PowerBoard/scene"
    if($DatasetDirectory) {
        $datasetPath=ProjectPath $DatasetDirectory
        if(-not (Test-Path -LiteralPath "$datasetPath/dataset.json") -or (Test-Path -LiteralPath "$datasetPath/incomplete.json")){throw 'Demo dataset is missing or incomplete'}
        New-Item -ItemType Directory -Path "$outputPath/examples" -Force | Out-Null
        Copy-Item -LiteralPath $datasetPath -Destination "$outputPath/examples/demo-dataset" -Recurse
        # Preserve numeric evidence while making report provenance portable.
        foreach($file in Get-ChildItem -LiteralPath "$outputPath/examples/demo-dataset/models" -Filter 'comparison.json' -Recurse -File -ErrorAction SilentlyContinue) {
            $content=[IO.File]::ReadAllText($file.FullName)
            $report=$content | ConvertFrom-Json
            if([IO.Path]::IsPathRooted($report.benchmark_path)) {
                $benchmark=[IO.Path]::GetFullPath($report.benchmark_path)
                $datasetPrefix=$datasetPath.TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
                if(-not $benchmark.StartsWith($datasetPrefix,[StringComparison]::OrdinalIgnoreCase)){throw 'Comparison provenance points outside the packaged dataset'}
                $relative=$benchmark.Substring($datasetPrefix.Length).Replace('\','/')
                $encoded=ConvertTo-Json -InputObject $relative -Compress
                $content=[regex]::Replace($content,'"benchmark_path"\s*:\s*"(?:\\.|[^"\\])*"','"benchmark_path": '+$encoded)
                [IO.File]::WriteAllText($file.FullName,$content,[Text.UTF8Encoding]::new($false))
            }
        }
    }
    Set-Content -LiteralPath "$outputPath/START-HERE.txt" -Encoding UTF8 -Value @'
SLX SynthForge / Silicon LogiX
Open slx-synthforge.exe. Select a component, remove/move/rotate it, then click
Start inspection. Click results to inspect the captured RGB and linked CAD.
Click Stop to unlock edits and try again. The editable-board model is included.
The example dataset loads automatically when included. Assembly AI lab uses its own inspector examples.
Undo/Redo, Guided trial, Save experiment and Open explain the editable-board flow.
Rendering and inference require no Python. To train new models, install Python
and run: python -m pip install -r tools/requirements.txt
Select that Python executable in Dataset forge. See docs/quick-start.md.
This is an evaluation demonstration. See LICENSE and docs/demo-v04.md.
'@
    $runtimeSources=ProjectPath $RuntimeSourceDirectory
    if(-not (Test-Path -LiteralPath "$runtimeSources/manifest.json")){throw 'Prepare corresponding sources with tools/prepare-runtime-sources.py before packaging'}
    $sourceManifest=Get-Content -LiteralPath "$runtimeSources/manifest.json" -Raw | ConvertFrom-Json
    foreach($record in $sourceManifest.source_archives) {
        $archive=Get-Item -LiteralPath (Join-Path $runtimeSources $record.file)
        if($archive.Length -ne $record.bytes -or (Get-FileHash -LiteralPath $archive.FullName -Algorithm SHA256).Hash.ToLowerInvariant() -ne $record.sha256) {
            throw "Corresponding source archive is missing or changed: $($record.file)"
        }
    }
    foreach($property in $sourceManifest.runtime_files.PSObject.Properties) {
        if((Get-FileHash -LiteralPath "$outputPath/$($property.Name)" -Algorithm SHA256).Hash.ToLowerInvariant() -ne $property.Value) {
            throw "Deployed runtime differs from corresponding source record: $($property.Name)"
        }
    }
    Copy-Item -LiteralPath $runtimeSources -Destination "$outputPath/runtime-sources" -Recurse
    New-Item -ItemType Directory -Path "$outputPath/examples" -Force | Out-Null
    Copy-Item -LiteralPath "$projectRoot/examples/inspector-dataset" -Destination "$outputPath/examples/inspector-dataset" -Recurse
    if(Test-Path -LiteralPath "$projectRoot/examples/inspector-cycle/cycle.json"){Copy-Item -LiteralPath "$projectRoot/examples/inspector-cycle" -Destination "$outputPath/examples/inspector-cycle" -Recurse}
    # Verify with the deployed runtime, no developer Qt kit on PATH.
    $env:PATH="$env:SystemRoot/System32;$env:SystemRoot"
    $probe=Start-Process -FilePath "$outputPath/slx-synthforge.exe" -ArgumentList '--smoke-test' -WorkingDirectory $outputPath -WindowStyle Hidden -PassThru -Wait
    if($probe.ExitCode -ne 0){throw "Portable startup failed: $($probe.ExitCode)"}
    $release=[ordered]@{schema='siliconlogix.portable-release.v1';version='0.4.1';core_abi=4;files=[ordered]@{}}
    foreach($file in Get-ChildItem -LiteralPath $outputPath -Recurse -File | Sort-Object FullName) {
        $relative=$file.FullName.Substring($outputPath.TrimEnd('\','/').Length+1).Replace('\','/')
        $release.files[$relative]=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    [IO.File]::WriteAllText("$outputPath/PORTABLE-MANIFEST.json",($release | ConvertTo-Json -Depth 5)+"`n",[Text.UTF8Encoding]::new($false))
    if($CreateArchive) {
        $zipPath=WorkspaceTarget ($OutputDirectory+'.zip')
        if(Test-Path -LiteralPath $zipPath){throw 'Archive already exists'}
        Compress-Archive -LiteralPath $outputPath -DestinationPath $zipPath -CompressionLevel Optimal
    }
    Write-Output "Portable application: $outputPath/slx-synthforge.exe"
} finally {$env:PATH=$previousPath}
