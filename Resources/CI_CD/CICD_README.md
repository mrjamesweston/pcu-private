# Example CI/CD Script

### Disclaimer
_Playflow Cloud for Unreal Plugin is NOT affiliated with Playflow Cloud, Playflow, LLC or Playflow Cloud the Platform and website. All information here is written from their Documentation and REST API for use in our Playflow Cloud for Unreal Plugin. Playflow Cloud is a copyright of Playflow, LLC. All Rights Reserved._

### Overview
- A set of helper scripts that can be added to a CI/CD pipeline in order to upload a build to PlayFlow Cloud via automation

### Requirements
- This script expects PowerShell to be installed on the host system. Install details can be found [here](https://github.com/PowerShell/PowerShell).
    + Note: PowerShell as of version 7 is cross-platform, so you can install it on Mac, Linux and Windows. Neat!
    + These scripts are fairly straightforward by design so transposing them to shell or batch is possible.
- This guide assumes you know your way around a build system and have access to a build machine or cloud-provided build systems

### Basic Setup
- Depending on your CI/CD method, you can either make a copy of the scripts in `./Resources/CI_CD/` or run the code directly on your chosen CI/CD platform.
    + [PowerShell GitHub Actions - adamdriscoll/pwsh-github-actions](https://github.com/adamdriscoll/pwsh-github-actions)
    + [PowerShell - TeamCity](https://www.jetbrains.com/help/teamcity/powershell.html)
    + [How to Use Jenkins to Run Parameterized PowerShell Scripts - Indeo Blog](https://blog.inedo.com/jenkins/run-parameterized-ps-scripts)

- It is highly recommended to use a System Environment variable on your build agent or a secret management system in your CI/CD platform rather than placing your key in plain text.
    + ⚠️ Under no circumstances should you submit files to source control (such as GitHub, Perforce, Unity Version Control, etc) that include your PlayFlow Cloud API key. If you do, please rotate your key via the dashboard _right away_ ⚠️

#### Examples
- Here's an example of my PowerShell build step for using the deploy script in TeamCity
```PowerShell
function UploadToPlayFlow {
  //// Example script code
}

function Replace-PlusWithUnderscore {
    param(
        [string]$inputString
    )

    return $inputString -replace '\+', '_'
}

// Key is encrypted and stored in TeamCity
$apiKey = "%PlayFlow.ApiKey%"

// Downloaded build artifact
$zipPath = "%teamcity.agent.work.dir%\LinuxServer.zip"

// Removing '+' in the naming convention I use with '_'
$buildName = Replace-PlusWithUnderscore -inputString %unrealBranchName%

$result = UploadToPlayFlow -ApiKey $apiKey -ZipFilePath $zipPath -BuildName $buildName

if ($result) {
    Write-Host "Upload completed successfully!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "Upload failed!" -ForegroundColor Red
    exit 1
}
```
- Using the create launcher script in TeamCity
```PowerShell
$ProjectName="%playFlow.projectName%"
$ServerBinary="%playFlow.serverBin%"


$Launcher = @"
#!/bin/sh
UE4_TRUE_SCRIPT_NAME=`$(echo "`$0`" | xargs readlink -f)`
UE4_PROJECT_ROOT=`$(dirname "`$UE4_TRUE_SCRIPT_NAME`")`

# Ensure the server binary is executable
chmod +x "`$UE4_PROJECT_ROOT`/$ProjectName/Binaries/Linux/$ServerBinary"


# If we're root, drop privileges and run as 'unreal'
if [ "`$(id -u)`" -eq 0 ]; then
    # Create 'unreal' user if it doesn't exist
    id -u unreal >/dev/null 2>&1 || useradd -m unreal

    # Make sure unreal owns the server files
    chown -R unreal:unreal "`$UE4_PROJECT_ROOT`"

    # Run server as 'unreal' - fixed argument passing
    exec su unreal -c "cd '`$UE4_PROJECT_ROOT`' && ./$ProjectName/Binaries/Linux/$ServerBinary $ProjectName `$`*"
else
    # Not root, just run normally
    exec "`$UE4_PROJECT_ROOT`/$ProjectName/Binaries/Linux/$ServerBinary" $ProjectName "`$`@"
fi
"@

# replace any accidental CRLF with LF, then emit without adding new ones
$Launcher -replace "`r`n","`n" |
        Set-Content -NoNewline -Encoding UTF8 ./LinuxServer/Server.x86_64
```
