@echo off
setlocal enabledelayedexpansion

:: Check if a zip file was dragged and dropped
if "%~1"=="" (
    echo Please drag and drop a zip folder onto this script.
    exit /b
)

:: Step 0: Move the zip to this folder
::move /y "%~1" "%~dp0"

:: Step 0.5: Change the active directory from the drag location to this file's dir.
::cd "%~dp0"

:: Step 1: Get the zip file path and name
:: %~1 is the first command, which happens to be the source dir in this case. Doesn't update. So, we need to use a dynamic var instead.
set "zipFile=%~1"
::set "zipFile=%CD%"
::set "zipFile=%~dp0%~nx1"
set "zipName=%~n1"
set "targetZipDir=%~dp0%zipName%"

set dataFileName=zzzapdata

:: Step 3: Extract the zip file
powershell -command "Add-Type -AssemblyName 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('!zipFile!', '!targetZipDir!')"

echo extracted main folder !zipFile! as !zipName! to !targetZipDir!
::pause 

:: Step 3.5: Extract the player's zip file that was in the primary zip file...
set "playerZip="
set "playerZipName="
:: I think %%f is acting as a kind of variable as well
::for /r ".\%zipName%" %%f in (*.zip) do (
for /r "%targetZipDir%" %%f in (*.zip) do (
    set "playerZip=%%f"
	set "playerZipName=%%~nf"
)

::set "playerDir=%zipName%\%playerZipName%"
set "playerDir=%targetZipDir%\%playerZipName%"

echo extracting player folder !playerZip! as !playerZipName! to !playerDir!
::pause

powershell -command "Add-Type -AssemblyName 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('!playerZip!', '!playerDir!')"

:: Step 4: Find the text file in the extracted folder
set "textFile="
echo searching %playerDir% for txt files - find %dataFileName%
for /r "%playerDir%" %%f in (*.txt) do (
	::set "textFile=%%f"
	set "textFileName=%%~nf"
	
	if /I "!textFileName!"=="%dataFileName%" (
		set "textFile=%%f"
	) else (
		:: move the file to root dir of game so the game can find the resource
		move /y "%%f" "%~dp0"
	)
)

echo text file found !textFile!

::pause 

:: Check if the text file was found
if "!textFile!"=="" (
    echo No text file found in the zip folder.
	pause
    exit /b
)

:: Step 5: Call the other executable to transform the text file
"%~dp0\EVNEW.exe" -torez "!textFile!" "!textFile:~0,-4!.rez"

::pause 

set "pluginDir=%~dp0Nova Plug-ins"

echo !pluginDir!

:: Step 2: Create a patch directory if it doesn't exist
if not exist "!pluginDir!" (
    mkdir "!pluginDir!"
)

::pause

:: Step 6: Move the transformed file to the plugin directory
if exist "!textFile:~0,-4!.rez" (
    move /y "!textFile:~0,-4!.rez" "!pluginDir!"
) else (
    echo Transformation failed or file does not exist.
)

::pause 

:: Step 7: Clean up the extracted folder
:: /S removes dirs and files, including the dir itself. /Q skips confirmation prompts.
if exist "%targetZipDir%" (
	RD /S /Q "%targetZipDir%"
)

echo Process completed!
::exit /b
pause
