param()
$ErrorActionPreference='Stop'
$privateRoot=Join-Path (Get-Location) 'qa-private'
$decrypted=Join-Path $privateRoot 'a1mini-original.3mf'
$passFile=Join-Path $privateRoot 'passphrase.txt'
$report=Join-Path $privateRoot 'private-a1mini-b-report.json'
$expectedEncrypted='1bdfc1bc7b9482d4f393a0701fda7f765f1ce81195ca203f693f9fb4850e4043'
$expected3MF='3649d011b82a52aceed643f7e82c22803a59001704f5e3e415e16a7ed590c365'
$expectedWine='321a29de1290a42edf785674e789b9d69d41c5ba979a69144faf081aa69372df'
$encrypted='filuna/FILUNA/qa/fixtures/a1mini-makerworld-original.3mf.gpg'
$regex='^(BambuStudio|bambu-studio)\.exe$'
function Sha([string]$p) { (Get-FileHash $p -Algorithm SHA256).Hash.ToLowerInvariant() }
New-Item -ItemType Directory -Force -Path $privateRoot | Out-Null
try {
  if (!(Test-Path $encrypted)) { throw 'Encrypted A1-mini fixture missing in GitHub branch' }
  if ((Sha $encrypted) -ne $expectedEncrypted) { throw 'Encrypted fixture SHA invalid' }
  if (!$env:FILUNA_A1MINI_PASSPHRASE) { throw 'GitHub Actions secret FILUNA_A1MINI_3MF_GPG_PASSPHRASE missing' }
  $gpg=(Get-Command gpg.exe -ErrorAction SilentlyContinue)
  if (!$gpg) { throw 'gpg.exe must be installed on the Windows runner' }
  # Do NOT put the secret in command args or process logs.
  [IO.File]::WriteAllText($passFile,$env:FILUNA_A1MINI_PASSPHRASE,[Text.Encoding]::ASCII)
  & $gpg.Source --batch --yes --pinentry-mode loopback --passphrase-file $passFile --output $decrypted --decrypt $encrypted
  if ($LASTEXITCODE -ne 0) { throw 'GPG decryption failed' }
  Remove-Item $passFile -Force
  if ((Sha $decrypted) -ne $expected3MF) { throw 'Original 3MF SHA invalid after decrypt' }
  Write-Host 'PRIVATE_A1MINI_3MF_SHA256 verified'
  $official=Get-ChildItem 'qa-public/official-release' -File -Recurse | Where-Object { $_.Name -match $regex } | Select-Object -First 1
  $compiled=Get-ChildItem 'official/install-dir' -File -Recurse | Where-Object { $_.Name -match $regex } | Select-Object -First 1
  if (!$official -or !$compiled) { throw 'Missing Windows official or compiled binaries from public B test' }
  foreach ($job in @([pscustomobject]@{Exe=$official.FullName;Key='official'},[pscustomobject]@{Exe=$compiled.FullName;Key='compiled'})) {
    $out=Join-Path $privateRoot ('slice-'+$job.Key)
    $datadir=Join-Path $out 'datadir'
    New-Item -ItemType Directory -Force -Path $datadir | Out-Null
    & $job.Exe --datadir $datadir --outputdir $out --slice 0 $decrypted
    if ($LASTEXITCODE -ne 0) { throw "Private slicing failed: $($job.Key), exit $LASTEXITCODE" }
  }
  $a=Get-ChildItem (Join-Path $privateRoot 'slice-official') -File -Filter '*.gcode' | Select-Object -First 1
  $b=Get-ChildItem (Join-Path $privateRoot 'slice-compiled') -File -Filter '*.gcode' | Select-Object -First 1
  if (!$a -or !$b) { throw 'Private A1-mini G-code missing; cannot compare' }
  $officialSha=Sha $a.FullName
  $compiledSha=Sha $b.FullName
  Write-Host "PRIVATE_OFFICIAL_WIN_SHA256=$officialSha"
  Write-Host "PRIVATE_COMPILED_WIN_SHA256=$compiledSha"
  Write-Host "WINE_REFERENCE_SHA256=$expectedWine"
  Write-Host "WINE_REFERENCE_IDENTICAL=$($officialSha -eq $expectedWine)"
  $semantic=Join-Path $privateRoot 'private-semantic.json'
  python filuna/FILUNA/qa/gcode/compare_oem_exact_sha.py --gate B --official $a.FullName --filuna $b.FullName --out $semantic
  $exitCode=$LASTEXITCODE
  $result=Get-Content $semantic -Raw | ConvertFrom-Json
  $summary=[ordered]@{
    source='encrypted-private-3mf'
    fixture_sha256=$expected3MF
    source_commit='56a57d00a591360cf41bf21383915ee5bf357be5'
    official_win_gcode_sha256=$officialSha
    compiled_win_gcode_sha256=$compiledSha
    wine_reference_gcode_sha256=$expectedWine
    wine_matches_official_windows=($officialSha -eq $expectedWine)
    raw_sha256_identical=($officialSha -eq $compiledSha)
    normalized_sha256_identical=$result.byte_equal_after_allowed_normalization
    gate_b_status=$result.e8_acceptance
    first_difference=$result.first_difference
  }
  $summary | ConvertTo-Json -Depth 6 | Set-Content -Path $report -Encoding utf8
  if ($exitCode -ne 0) { throw 'Private B G-code SHA differs; first difference recorded in SHA-only report' }
}
finally {
  if (Test-Path $passFile) { Remove-Item $passFile -Force }
  if (Test-Path $decrypted) { Remove-Item $decrypted -Force }
  foreach ($folder in @('slice-official','slice-compiled')) {
    $path=Join-Path $privateRoot $folder
    if (Test-Path $path) { Remove-Item $path -Recurse -Force }
  }
  $details=Join-Path $privateRoot 'private-semantic.json'
  if (Test-Path $details) { Remove-Item $details -Force }
}
