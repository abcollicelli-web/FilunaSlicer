param(
  [switch]$StageOnly,
  [switch]$CompareOnly
)
$ErrorActionPreference='Stop'
$fixtureDir=Join-Path (Get-Location) 'qa-fixtures'
New-Item -ItemType Directory -Force -Path $fixtureDir | Out-Null
$fixture=Join-Path $fixtureDir 'a1-mini-original.3mf'
$wine=Join-Path $fixtureDir 'wine_reference.gcode'
$knownFixture='3649d011b82a52aceed643f7e82c22803a59001704f5e3e415e16a7ed590c365'
$knownWine='321a29de1290a42edf785674e789b9d69d41c5ba979a69144faf081aa69372df'
function Sha([string]$path) { return (Get-FileHash -Algorithm SHA256 $path).Hash.ToLowerInvariant() }
if ($StageOnly) {
  if (!$env:FILUNA_FIXTURE_URL -or !$env:FILUNA_WINE_URL) { throw 'Both temporary input links are required' }
  # The SentinelX download endpoint requires POST and has one-use links.
  curl.exe --fail --show-error --location --retry 2 --request POST --output $fixture $env:FILUNA_FIXTURE_URL
  if ($LASTEXITCODE -ne 0) { throw "3MF transfer failed or expired" }
  curl.exe --fail --show-error --location --retry 2 --request POST --output $wine $env:FILUNA_WINE_URL
  if ($LASTEXITCODE -ne 0) { throw "Wine reference transfer failed or expired" }
  Write-Host "FIXTURE_SHA256 $(Sha $fixture)"
  Write-Host "WINE_REFERENCE_SHA256 $(Sha $wine)"
  if ((Sha $fixture) -ne $knownFixture) { throw 'Source 3MF corrupted or substituted' }
  if ((Sha $wine) -ne $knownWine) { throw 'Wine reference corrupted or substituted' }
  exit 0
}
if (!$CompareOnly) { throw 'Specify StageOnly or CompareOnly' }
if ((Sha $fixture) -ne $knownFixture) { throw 'Original 3MF SHA invalid before slicing' }
if ((Sha $wine) -ne $knownWine) { throw 'Original Wine G-code SHA invalid before slicing' }
$releaseZip=Join-Path (Get-Location) 'bambu-official-windows-02090170.zip'
$releaseUrl='https://github.com/bambulab/BambuStudio/releases/download/v02.09.01.70/Bambu_Studio_win-v02.09.01.70-20261103183404.zip'
curl.exe --fail --show-error --location --retry 3 -o $releaseZip $releaseUrl
if ($LASTEXITCODE -ne 0) { throw 'Failed to download official Bambu Windows portable release' }
$officialDir=Join-Path (Get-Location) 'official-win-release'
Expand-Archive -Path $releaseZip -DestinationPath $officialDir -Force
$exeName='^(bambu-studio|BambuStudio)\.exe$'
$official=Get-ChildItem -Path $officialDir -Recurse -File | Where-Object { $_.Name -match $exeName } | Select-Object -First 1
$compiled=Get-ChildItem -Path 'official/install-dir' -Recurse -File | Where-Object { $_.Name -match $exeName } | Select-Object -First 1
if (!$official -or !$compiled) { throw 'Official or compiled Bambu Studio Windows CLI executable missing' }
Write-Host "OFFICIAL_EXE $($official.FullName) SHA256 $(Sha $official.FullName)"
Write-Host "COMPILED_EXE $($compiled.FullName) SHA256 $(Sha $compiled.FullName)"
$runs = @(
  [pscustomobject]@{Exe=$official.FullName;Name='qa-out-official'},
  [pscustomobject]@{Exe=$compiled.FullName;Name='qa-out-compiled'}
)
foreach ($entry in $runs) {
  $out=Join-Path (Get-Location) $entry.Name
  $datadir=Join-Path $out 'datadir'
  New-Item -ItemType Directory -Path $datadir -Force | Out-Null
  # Same CLI flags and plate, with only the necessary output/data isolation.
  & $entry.Exe --datadir $datadir --outputdir $out --slice 0 $fixture
  if ($LASTEXITCODE -ne 0) { throw "CLI returned $LASTEXITCODE for $($entry.Exe)" }
}
$g1=Get-ChildItem 'qa-out-official' -Filter '*.gcode' -File | Select-Object -First 1
$g2=Get-ChildItem 'qa-out-compiled' -Filter '*.gcode' -File | Select-Object -First 1
if (!$g1 -or !$g2) { throw 'One or both Windows G-code outputs missing' }
$outOfficial=Join-Path $fixtureDir 'oficial_win.gcode'
$outCompiled=Join-Path $fixtureDir 'compilado_win.gcode'
Copy-Item $g1.FullName $outOfficial
Copy-Item $g2.FullName $outCompiled
Write-Host "OFFICIAL_WINDOWS_GCODE_SHA256 $(Sha $outOfficial)"
Write-Host "COMPILED_WINDOWS_GCODE_SHA256 $(Sha $outCompiled)"
Write-Host "WINE_GCODE_SHA256 $(Sha $wine)"
$compare='filuna/FILUNA/qa/gcode/compare_oem_exact_sha.py'
python $compare --gate B --official $outOfficial --filuna $outCompiled --out (Join-Path $fixtureDir 'b-test-compiled-vs-official.json')
$bExit=$LASTEXITCODE
python $compare --official $outOfficial --filuna $wine --out (Join-Path $fixtureDir 'wine-vs-windows.json')
$wineExit=$LASTEXITCODE
Write-Host "B_COMPARISON_EXIT $bExit"
Write-Host "WINE_COMPARISON_EXIT $wineExit (a mismatch invalidates the Wine reference)"
if ($bExit -ne 0) { throw 'B gate failed; see the comparison JSON for exact first differing line' }
