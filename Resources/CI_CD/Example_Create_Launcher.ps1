$ProjectName="Test"
$ServerBinary="TestServer"

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