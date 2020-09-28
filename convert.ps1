function uint8Array($var) {
    begin { $buf = @() }
    process { $buf += $_ }
    end {
        $str = [Convert]::ToBase64String($buf)
        "export const $var = new Uint8Array(window.atob('`\"
        for ($i = 0; $str.Length - $i -gt 120; $i += 120) {$str.Substring($i, 120) + '\'}
        $str.Substring($i) + '\'
        ''').split('''').map(c => c.charCodeAt(0)));'
        ''
    }
}

Get-Content $Args[0] -Encoding Byte -ReadCount 0 | uint8Array bufferSource | Set-Content $Args[1]
