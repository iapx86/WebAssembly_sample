function uint8Array($var) {
    begin { $buf = @() }
    process { $buf += $_ }
    end {
        $str = [Convert]::ToBase64String($buf)
        "export const $var = Uint8Array.from(window.atob('`\"
        for ($i = 0; $str.Length - $i -gt 120; $i += 120) {$str.Substring($i, 120) + '\'}
        $str.Substring($i) + '\'
        ''').split(''''), c => c.charCodeAt(0));'
        ''
    }
}

$temp = New-TemporaryFile
Compress-Archive $Args[0] $temp -Force
Get-Content $temp -AsByteStream -ReadCount 0 | uint8Array archive | Set-Content $Args[1]
Remove-Item -Path $temp

