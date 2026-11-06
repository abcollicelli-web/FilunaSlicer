param(
  [string]$Fixture='official/tests/data/20mm_cube.obj'
)
$ErrorActionPreference='Stop'
$dir=Join-Path (Get-Location) 'qa-public'
New-Item -ItemType Directory -Force -Path $dir | Out-Null
$releaseUrl='https://github.com/bambulab/BambuStudio/releases/download/v02.09.01.70/Bambu_Studio_win-v02.09.01.70-20261103183404.zip'
$zip=Join-Path $dir 'bambu-official-win.zip'
curl.exe --fail --show-error --location --retry 3 --output $zip $releaseUrl
if ($LASTEXITCODE -ne 0) { throw 'Cannot fetch official Bambu Studio ZIP from GitHub release' }
$releaseDir=Join-Path $dir 'official-release'
Expand-Archive -Path $zip -DestinationPath $releaseDir -Force
$regex='^(BambuStudio|bambu-studio)\.exe$'
$official=Get-ChildItem $releaseDir -Recurse -File | Where-Object { $_.Name -match $regex } | Select-Object -First 1
$compiled=Get-ChildItem 'official/install-dir' -Recurse -File | Where-Object { $_.Name -match $regex } | Select-Object -First 1
if (!$official -or !$compiled) { throw 'Missing official/compiled Windows CLI executable' }
function Hash([string]$p) { (Get-FileHash $p -Algorithm SHA256).Hash.ToLowerInvariant() }
Write-Host "OFFICIAL_BINARY $($official.FullName) SHA256 $(Hash $official.FullName)"
Write-Host "COMPILED_BINARY $($compiled.FullName) SHA256 $(Hash $compiled.FullName)"
$fixtureDir=Join-Path $dir 'generate'
$fixtureData=Join-Path $fixtureDir 'datadir'
New-Item -ItemType Directory -Force -Path $fixtureData | Out-Null
$machine=(Resolve-Path 'official/resources/profiles/BBL/machine/Bambu Lab A1 mini 0.4 nozzle.json').Path
$process=(Resolve-Path 'official/resources/profiles/BBL/process/0.20mm Standard @BBL A1M.json').Path
$filament=(Resolve-Path 'official/resources/profiles/BBL/filament/Bambu PLA Basic @BBL A1M.json').Path
$model=(Resolve-Path $Fixture).Path
# Generate a Bambu-native 3MF with an official CLI and its A1-mini manufacturer profiles.
& $official.FullName --datadir $fixtureData --load-settings "$machine;$process" --load-filaments $filament --outputdir $fixtureDir --export-3mf 'filuna_a1mini_cube.3mf' $model
if ($LASTEXITCODE -ne 0) { throw "Official A1-mini 3MF generator failed: $LASTEXITCODE" }
$project=Join-Path $fixtureDir 'filuna_a1mini_cube.3mf'
if (!(Test-Path $project)) { throw 'Official 3MF was not generated' }
Write-Host "PUBLIC_TEST_3MF_SHA256 $(Hash $project)"
# Validate the manufacturer identity from Bambu-native 3MF, not by looking at its name.
$checkScript = "import json,sys,zipfile; z=zipfile.ZipFile(sys.argv[1]); d=json.loads(z.read('Metadata/project_settings.config')); assert d['printer_model']=='Bambu Lab A1 mini',d.get('printer_model'); assert d['printer_settings_id']=='Bambu Lab A1 mini 0.4 nozzle'; assert d['print_settings_id']=='0.20mm Standard @BBL A1M'; print('OFFICIAL_A1MINI_PROJECT_VALID')"
python -c $checkScript $project
if ($LASTEXITCODE -ne 0) { throw 'Generated 3MF does not include correct A1-mini preset' }
$cases=@(
  [pscustomobject]@{Exe=$official.FullName;Name='official'},
  [pscustomobject]@{Exe=$compiled.FullName;Name='compiled'}
)
foreach ($case in $cases) {
  $output=Join-Path $dir ('slice-'+$case.Name)
  $datadir=Join-Path $output 'datadir'
  New-Item -ItemType Directory -Force -Path $datadir | Out-Null
  Write-Host "SLICING $($case.Name): exe=$($case.Exe)"
  & $case.Exe --datadir $datadir --outputdir $output --slice 0 $project
  if ($LASTEXITCODE -ne 0) { throw "Bambu CLI failure in $($case.Name): $LASTEXITCODE" }
}
$a=Get-ChildItem (Join-Path $dir 'slice-official') -Filter '*.gcode' -File | Select-Object -First 1
$b=Get-ChildItem (Join-Path $dir 'slice-compiled') -Filter '*.gcode' -File | Select-Object -First 1
if (!$a -or !$b) { throw 'Missing a Windows G-code; impossible to compare' }
Write-Host "OFFICIAL_WIN_GCODE_SHA256 $(Hash $a.FullName)"
Write-Host "COMPILED_WIN_GCODE_SHA256 $(Hash $b.FullName)"
$rawEqual = ((Hash $a.FullName) -eq (Hash $b.FullName))
Write-Host "EXACT_RAW_SHA256_MATCH $rawEqual"
Write-Host "WINE_DIORAMA_SHA256 321a29de1290a42edf785674e789b9d69d41c5ba979a69144faf081aa69372df (not comparable with own cube)"
$report=Join-Path $dir 'b-public-cube-report.json'
python filuna/FILUNA/qa/gcode/compare_oem_exact_sha.py --gate B --official $a.FullName --filuna $b.FullName --out $report
$comparisonExit=$LASTEXITCODE
Write-Host "B_PUBLIC_TEST_RESULT_EXIT $comparisonExit"
# Only the small report may be uploaded, never either G-code, 3MF or packaged executable.
if (!(Test-Path $report)) { throw 'Missing B test report' }
if ($comparisonExit -ne 0 -or !$rawEqual) {
  Get-Content $report
  throw 'B FAILED: exact raw SHA256 mismatch or normalized mismatch; see first divergent line in report'
}
Write-Host "B_PUBLIC_TEST_PASS"
