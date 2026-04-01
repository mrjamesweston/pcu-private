
function UploadToPlayFlow {
    param(
        [Parameter(Mandatory=$true)]
        [string]$ApiKey,

        [Parameter(Mandatory=$true)]
        [string]$ZipFilePath,

        [Parameter(Mandatory=$true)]
        [string]$BuildName
    )

    Write-Host "Playflow: Starting upload to Playflow Cloud..."

    # Check if file exists
    if (-not (Test-Path $ZipFilePath)) {
        Write-Error "Playflow: Zip file not found: $ZipFilePath"
        return $false
    }

    try {
        # Read file data
        $FileData = [System.IO.File]::ReadAllBytes($ZipFilePath)
        $FileSize = $FileData.Length
        $FileSizeMB = [math]::Round($FileSize / (1024.0 * 1024.0), 2)
        Write-Host "Playflow: File loaded: $FileSizeMB MB"

        Write-Host "Playflow: Step 1 - Requesting upload URL..."

        # Step 1: Get upload URL
        $headers = @{
            "api-key" = $ApiKey
        }

        $url = "https://api.computeflow.cloud/v2/builds/builds/upload-url?name=" + $BuildName

        # Use Invoke-WebRequest to get full response details
        $getUrlResponse = Invoke-WebRequest -Uri $url `
                                           -Method POST `
                                           -Headers $headers `
                                           -TimeoutSec 300

        Write-Host "Playflow: Get URL Response Code: $($getUrlResponse.StatusCode)"
        Write-Host "Playflow: Get URL Response: $($getUrlResponse.Content)"

        if ($getUrlResponse.StatusCode -lt 200 -or $getUrlResponse.StatusCode -ge 300) {
            Write-Error "Playflow: Failed to get upload URL - HTTP $($getUrlResponse.StatusCode)"
            return $false
        }

        # Parse JSON response
        $responseJson = $getUrlResponse.Content | ConvertFrom-Json
        $UploadUrl = $responseJson.upload_url

        if ([string]::IsNullOrEmpty($UploadUrl)) {
            Write-Error "Playflow: No upload_url in response"
            return $false
        }

        Write-Host "Playflow: Got upload URL: $UploadUrl"

        Write-Host "Playflow: Step 2 - Uploading file ($FileSizeMB MB)..."

        # Step 2: Upload file - SIMPLE PUT with raw file data
        try {
            # Upload the raw file bytes directly with Content-Type: application/zip
            $uploadResponse = Invoke-WebRequest -Uri $UploadUrl `
                                               -Method PUT `
                                               -ContentType "application/zip" `
                                               -Body $FileData `
                                               -TimeoutSec 900

            Write-Host "Playflow: Upload Response Code: $($uploadResponse.StatusCode)"

            # S3 upload might not return content, so check if it exists
            if ($uploadResponse.Content) {
                Write-Host "Playflow: Upload Response: $($uploadResponse.Content)"
            } else {
                Write-Host "Playflow: Upload Response: (empty - typical for successful S3 upload)"
            }

            # For S3 presigned URLs, success is typically 200 OK
            if ($uploadResponse.StatusCode -eq 200) {
                Write-Host "Playflow: Upload successful!"
                return $true
            }
            else {
                Write-Error "Playflow: File upload failed - HTTP $($uploadResponse.StatusCode)"
                return $false
            }
        }
        catch {
            # Handle web request exceptions which contain response details
            if ($_.Exception.Response) {
                $statusCode = $_.Exception.Response.StatusCode.value__
                $statusDescription = $_.Exception.Response.StatusDescription
                Write-Error "Playflow: File upload failed - HTTP $statusCode : $statusDescription"

                # Try to get response content if available
                try {
                    $stream = $_.Exception.Response.GetResponseStream()
                    $reader = New-Object System.IO.StreamReader($stream)
                    $errorContent = $reader.ReadToEnd()
                    Write-Host "Playflow: Upload Error Details: $errorContent"
                } catch {
                    Write-Host "Playflow: Could not read error response content"
                }
            } else {
                Write-Error "Playflow: File upload failed - Network error: $($_.Exception.Message)"
            }
            return $false
        }
    }
    catch {
        # Handle web request exceptions for the first request
        if ($_.Exception.Response) {
            $statusCode = $_.Exception.Response.StatusCode.value__
            $statusDescription = $_.Exception.Response.StatusDescription
            Write-Error "Playflow: Failed to get upload URL - HTTP $statusCode : $statusDescription"
        } else {
            Write-Error "Playflow: Failed to get upload URL - Network error: $($_.Exception.Message)"
        }
        return $false
    }
}

# Example usage
$apiKey = "YOUR KEY HERE"
$zipPath = ".\LinuxServer.zip"
$buildName = "__exampleProject_v1"

$result = UploadToPlayFlow -ApiKey $apiKey -ZipFilePath $zipPath -BuildName $buildName

if ($result) {
    Write-Host "Upload completed successfully!" -ForegroundColor Green
} else {
    Write-Host "Upload failed!" -ForegroundColor Red
}