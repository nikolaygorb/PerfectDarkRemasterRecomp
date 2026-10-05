# Prints the busiest threads of a running process over a sampling window, as a share of one core.
# Thread ids match the [tNNNN] tags in the game log.
param(
    [string]$ProcessName = "perfectdarkremasterrecomp",
    [int]$Seconds = 10,
    [int]$Top = 12
)

$process = Get-Process -Name $ProcessName -ErrorAction Stop | Select-Object -First 1

function Get-ThreadTimes($process) {
    $process.Refresh()
    $times = @{}
    foreach ($thread in $process.Threads) {
        $times[$thread.Id] = $thread.TotalProcessorTime.TotalMilliseconds
    }
    $times
}

$before = Get-ThreadTimes $process
Start-Sleep -Seconds $Seconds
$after = Get-ThreadTimes $process

$after.GetEnumerator() |
    ForEach-Object {
        $start = if ($before.ContainsKey($_.Key)) { $before[$_.Key] } else { 0 }
        [pscustomobject]@{ Thread = $_.Key; CpuPercent = [math]::Round(($_.Value - $start) / ($Seconds * 10), 1) }
    } |
    Sort-Object CpuPercent -Descending |
    Select-Object -First $Top |
    Format-Table -AutoSize
