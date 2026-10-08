# Silicon LogiX / SLX SynthForge
# Copyright (c) 2026 Marco Pezzullo (Silicon LogiX).
# Author: Marco Pezzullo
# License: Silicon LogiX Evaluation License 1.0. See LICENSE.

# Check every file in a standalone source release, then write or verify its manifest.
param([Parameter(Mandatory=$true)][string]$Directory,[switch]$WriteManifest,[switch]$AllowGit)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$target=[IO.Path]::GetFullPath($Directory).TrimEnd('\','/')
if(-not (Test-Path -LiteralPath "$target/CMakeLists.txt")){throw 'Source release root not found'}
$entries=Get-ChildItem -LiteralPath $target -Recurse -Force
$files=@()
foreach($entry in $entries) {
    $relative=$entry.FullName.Substring($target.Length+1).Replace('\','/')
    if($AllowGit -and ($relative -eq '.git' -or $relative.StartsWith('.git/'))){continue}
    if($entry.Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Linked path in source release: $relative"}
    if($relative -match '(^|/)(private|workspace|build|dist|\.git|__pycache__)(/|$)'){throw "Excluded path in source release: $relative"}
    if($entry.PSIsContainer){continue}
    if($relative -notmatch '^(src/|qml/|core/include/|sdk/windows-mingw/|assets/(brand|screenshots|models)/|docs/|licenses/|examples/(inspector-dataset|inspector-cycle)/|tests/|tools/|scripts/|SLX_SynthForge_PowerBoard/(scene/|README\.md$)|CMakeLists\.txt$|resources\.qrc$|README\.md$|LICENSE$|THIRD_PARTY_(NOTICES|SOURCE)\.md$|PUBLIC-MANIFEST\.json$|\.(gitignore|gitattributes|clang-format)$)') {
        throw "File outside the source release allowlist: $relative"
    }
    if($entry.Extension -in '.pdb','.pem','.key','.env','.log','.bak' -or $entry.Length -ge 100MB){throw "Unexpected or oversized release file: $relative"}
    $files+=@{entry=$entry;relative=$relative}
    if($entry.Extension -in '.cpp','.h','.qml','.py','.md','.ps1','.json','.txt','.svg','.rc','.qrc' -or $entry.Name -eq 'LICENSE') {
        $content=[IO.File]::ReadAllText($entry.FullName,[Text.UTF8Encoding]::new($false,$true))
        $sensitive=('[A-Z]:[\\/]+Users[\\/]+|internal'+' review bundle|initial package is for internal'+' review|BEGIN (RSA |OPENSSH |EC )?PRIVATE KEY|gh[pousr]_[A-Za-z0-9]{30}|github_pat_[A-Za-z0-9_]{40}|AKIA[0-9A-Z]{16}')
        if($content -match $sensitive){throw "Confidential note, personal path or credential pattern in $relative"}
        if($entry.Extension -in '.cpp','.h','.qml','.py','.ps1' -and
            ($content -notmatch 'Copyright \(c\) 2026 Marco Pezzullo' -or $content -notmatch 'License: Silicon LogiX Evaluation License')) {
            throw "Missing ownership/license header in $relative"
        }
        if($entry.Extension -eq '.json') {$null=$content.TrimStart([char]0xFEFF) | ConvertFrom-Json}
    }
}
$dll=Join-Path $target 'sdk/windows-mingw/SiliconLogiX.SynthForge.Core.dll'
$metadata=[Diagnostics.FileVersionInfo]::GetVersionInfo($dll)
if($metadata.CompanyName -ne 'Silicon LogiX' -or $metadata.ProductVersion -ne '0.4.1'){throw 'Core vendor/version metadata mismatch'}
$hashes=[ordered]@{}
foreach($item in ($files | Sort-Object relative)) {
    if($item.relative -eq 'PUBLIC-MANIFEST.json'){continue}
    $hashes[$item.relative]=(Get-FileHash -LiteralPath $item.entry.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
}
$manifestPath="$target/PUBLIC-MANIFEST.json"
if($WriteManifest) {
    $manifest=[ordered]@{schema='siliconlogix.public-export.v1';version='0.4.1';core_abi=4;company='Silicon LogiX';files=$hashes}
    [IO.File]::WriteAllText($manifestPath,($manifest | ConvertTo-Json -Depth 5)+"`n",[Text.UTF8Encoding]::new($false))
} else {
    $saved=Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if($saved.version -ne '0.4.1' -or @($saved.files.PSObject.Properties).Count -ne $hashes.Count){throw 'Source manifest identity/file count mismatch'}
    foreach($name in $hashes.Keys) {
        $property=$saved.files.PSObject.Properties[$name]
        if($null -eq $property -or $property.Value -ne $hashes[$name]){throw "Source manifest checksum mismatch: $name"}
    }
}
Write-Output "Public source audit passed: $($hashes.Count) files; ownership headers, integrity and confidentiality checks passed."
