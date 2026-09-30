@echo off
cd /d "%~dp0"
setlocal EnableDelayedExpansion

set TOKENFILE=D:\wbuddy\.ghtoken
set REPO=mmxn1218/pra
set BRANCH=main

:: read token (first non-empty line of token file)
set TOKEN=
for /f "usebackq delims=" %%t in ("%TOKENFILE%") do ( if not defined TOKEN set "TOKEN=%%t" )
if not defined TOKEN ( echo [ERROR] token not found: %TOKENFILE% & pause & exit /b 1 )

:: if python is on PATH, regenerate version.json + payload first
where python >nul 2>&1
if not errorlevel 1 ( python "%~dp0make_release.py" )

:: init / configure local git repo
if not exist .git ( git init -q )
git config user.email tongdexuetong@gmail.com
git config user.name mmxn1218

:: set remote with token (re-set each run so an expired/changed token does not linger)
git remote remove origin >nul 2>&1
git remote add origin https://%TOKEN%@github.com/%REPO%.git

:: stage and commit (only if there are changes)
git add -A
git diff --cached --quiet
if errorlevel 1 ( git commit -q -m "release PRA" ) else ( echo [INFO] nothing new to commit )

:: push (requires: 1) GitHub repo %REPO% created empty; 2) machine can reach github.com; 3) token valid)
git branch -M %BRANCH%
git push -u origin %BRANCH% --force
if errorlevel 1 ( echo [FAIL] push failed. Check: 1) GitHub repo %REPO% exists (empty); 2) network to github.com; 3) token valid. & pause & exit /b 1 )
echo [OK] pushed to https://github.com/%REPO%
pause
