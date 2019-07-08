Powershell tricks for the Linux guy
-----------------------------------

echo $PATH
$env:path -split ";"

which
(Get-Command cmd).Path

create new profile file
New-Item $profile -ItemType File -Force

test if path exist
Test-Path $profile

powershell editor
powershell_ise.exe $profile

create alias (only for session)
New-Item -Path Alias:python3 -Value C:\ProgramData\Anaconda3\python.exe



sloc count:
cloc --list-file=src-includes.lst --exclude-list-file=excluded.lst \
    --force-lang="C++",c --exclude-ext=csv