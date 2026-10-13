$ErrorActionPreference = 'Stop'
$upgradeRoot = Join-Path $env:RUNNER_TEMP 'NotyUpgradeTest'
$legacySetup = Join-Path $env:RUNNER_TEMP 'Noty-legacy-setup.exe'
$release = Invoke-RestMethod 'https://api.github.com/repos/Lyvt-Dev/Noty/releases/tags/v0.4.0'
$asset = $release.assets | Where-Object name -eq 'Noty-windows-x64-setup.exe'
if (-not $asset -or -not $asset.digest.StartsWith('sha256:')) { throw 'Missing legacy installer digest' }
Invoke-WebRequest $asset.browser_download_url -OutFile $legacySetup
if ((Get-FileHash -LiteralPath $legacySetup -Algorithm SHA256).Hash.ToLowerInvariant() -ne $asset.digest.Substring(7)) { throw 'Legacy installer checksum mismatch' }
$legacy = Start-Process -FilePath $legacySetup -ArgumentList '/S',"/D=$upgradeRoot" -Wait -PassThru -WindowStyle Hidden
if ($legacy.ExitCode -ne 0) { throw 'Legacy installer failed' }
$installed = Join-Path $upgradeRoot 'bin\Noty.exe'
if (-not (Test-Path -LiteralPath $installed)) { throw 'Legacy installation missing' }
$marker = Join-Path $upgradeRoot 'user-settings.marker'
Set-Content -LiteralPath $marker -Value 'preserve-existing-user-data'
$expected = (Get-FileHash -LiteralPath 'build\Release\Noty.exe' -Algorithm SHA256).Hash
# Deliberately no /D: the current installer must find the old installation.
$update = Start-Process -FilePath (Resolve-Path 'build\Noty-windows-x64-setup.exe').Path -ArgumentList '/S' -Wait -PassThru -WindowStyle Hidden
if ($update.ExitCode -ne 0) { throw 'Update installer failed' }
if ((Get-FileHash -LiteralPath $installed -Algorithm SHA256).Hash -ne $expected) { throw 'Existing installation was not updated in place' }
if ((Get-Content -LiteralPath $marker) -ne 'preserve-existing-user-data') { throw 'Update removed existing user data' }
$smoke = Start-Process -FilePath $installed -ArgumentList '--smoke-test' -Wait -PassThru -WindowStyle Hidden
if ($smoke.ExitCode -ne 0) { throw 'Updated application smoke test failed' }
Get-Content -LiteralPath (Join-Path $upgradeRoot 'bin\smoke-test.txt')
Write-Output 'PASS: 0.4 -> current installer update, existing directory preserved'
