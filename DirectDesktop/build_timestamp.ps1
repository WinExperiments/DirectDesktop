$compiledate = (Get-Date).ToUniversalTime().ToString('yyyy-MM-dd')
$compiletime = (Get-Date).ToUniversalTime().ToString('yyyyMMdd-HHmm')

@"
#define BUILD_DATE L`"$compiledate`"
#define BUILD_TIMESTAMP L`"$compiletime`"
"@ | Out-File -FilePath "$PSScriptRoot\build_timestamp.h" -Encoding Unicode