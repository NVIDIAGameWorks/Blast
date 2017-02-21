param(
[Parameter(Mandatory=$true)][string]$sourceGUID=$null,
[string]$output="out.exe"
)
$source = "http://nvgtl/download/" + $sourceGUID
$filename = $output
$key64 = 'QHV0ME1AdDNHVEwkY3IxcHQk'
$key = [System.Text.Encoding]::GetEncoding(1252).GetString([Convert]::FromBase64String($key64))
$key = $key | ConvertTo-SecureString -asPlainText -Force
$credential = New-Object System.Management.Automation.PSCredential('svcgtlautomate', $key)
$cache = New-Object System.Net.CredentialCache
$cache.Add( "http://sso.nvidia.com", "NTLM", $credential)

$req = [System.Net.httpwebrequest]::Create($source)
$req.cookiecontainer = New-Object System.net.CookieContainer
$req.Credentials = $cache
Write-Host "Connecting to NVGTL ..."
$res = $req.GetResponse()

if($res.StatusCode -eq "OK") {
  Write-Host "Downloading ..."
  [int]$goal = $res.ContentLength
  $reader = $res.GetResponseStream()
  $writer = new-object System.IO.FileStream $fileName, "Create"
  [byte[]]$buffer = new-object byte[] 4096
  [int]$total = [int]$count = 0
  do
  {
    $count = $reader.Read($buffer, 0, $buffer.Length);
    $writer.Write($buffer, 0, $count);
    $total += $count
    if($goal -gt 0) {
        Write-Progress "Downloading $url" "Saving $total of $goal" -id 0 -percentComplete (($total/$goal)*100)
    } else {
        Write-Progress "Downloading $url" "Saving $total bytes..." -id 0
    }
  } while ($count -gt 0)
 
  $reader.Close()
  $writer.Flush()
  $writer.Close()
}

