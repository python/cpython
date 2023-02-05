function Find-Tool {
    param([string]$toolname)

    $kitroot = (gp 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots\').KitsRoot10
    $tool = (gci -r "$kitroot\Bin\*\x64\$toolname" | sort FullName -Desc | select -First 1)
    if (-not $tool) {
        throw "$toolname is not available"
    }
    Write-Host "Found $toolname at $($tool.FullName)"
    return $tool.FullName
}

Set-Alias SignTool (Find-Tool "signtool.exe") -Scope Script

function Sign-File {
    param([string]$certname, [string]$certsha1, [string]$certfile, [string]$description, [string[]]$files)

    if (-not $description) {
        $description = $env:SigningDescription;
        if (-not $description) {
            $description = "Python";
        }
    }
    if (-not $certsha1) {
        $certsha1 = $env:SigningCertificateSha1;
    }
    if (-not $certname) {
        $certname = $env:SigningCertificate;
    }
    if (-not $certfile) {
        $certfile = $env:SigningCertificateFile;
    }

    if (-not ($certsha1 -or $certname -or $certfile)) {
        throw "No signing certificate specified"
    }

    foreach ($a in $files) {
        if ($certsha1) {
            SignTool sign /sha1 $certsha1 /fd sha256 /tr http://timestamp.digicert.com/ /td sha256 /d $description $a
        } elseif ($certname) {
            SignTool sign /a /n $certname /fd sha256 /tr http://timestamp.digicert.com/ /td sha256 /d $description $a
        } elseif ($certfile) {
            SignTool sign /f $certfile /fd sha256 /tr http://timestamp.digicert.com/ /td sha256 /d $description $a
        }
    }
}

