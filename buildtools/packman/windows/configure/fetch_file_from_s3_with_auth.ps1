param(
[Parameter(Mandatory=$true)][string]$sourceName=$null,
[string]$output=$sourceName
)
$bucket = "packman"
# use packman-ro user (can only read known files, cannot discover)
$aws_key_id = "AKIAJHPSPBMWMTZS6TJA"
$aws_secret_key = "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"

$verb = "GET"
$date = (Get-Date).ToUniversalTime()
$httpDate = $date.ToString("r")
$bucket_and_file = "/" + $bucket + "/" + $sourceName
$stringToSign = $verb + "`n`n`n" + $httpDate + "`n" + $bucket_and_file

$hmac = New-Object System.Security.Cryptography.HMACSHA1
$hmac.Key = [Text.Encoding]::ASCII.GetBytes($aws_secret_key)
$signatureHMACSHA1 = $hmac.ComputeHash([Text.Encoding]::ASCII.GetBytes($stringToSign))
$signatureHMACSHA1Base64 = [Convert]::ToBase64String($signatureHMACSHA1)

$authorizationValue = "AWS " + $aws_key_id + ":" + $signatureHMACSHA1Base64
$source = "http://" + $bucket + ".s3.amazonaws.com/" + $sourceName
$filename = $output

$req = [System.Net.httpwebrequest]::Create($source)
$req.cookiecontainer = New-Object System.net.CookieContainer
#$req.date = $date
$req.Headers.Add("Authorization", $authorizationValue)
Write-Host "Connecting to S3 ..."
$res = $req.GetResponse()

if($res.StatusCode -eq "OK") {
  Write-Host "Initiating download ..."
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

