# benchmark.ps1 - Compare D3D12 vs Vulkan performance
#
# Usage: .\tools\benchmark.ps1 [-Duration 60] [-SkipWarmup 10]
#
# Runs the game with both backends and compares performance metrics.

param(
    [int]$Duration = 60,
    [int]$SkipWarmup = 10,
    [string]$BuildDir = ""
)

# Project root is one level up from tools/
$ProjectRoot = Split-Path -Parent $PSScriptRoot

# Default build dir if not specified
if (-not $BuildDir) {
    $BuildDir = Join-Path $ProjectRoot "out\build\win-amd64-relwithdebinfo"
}

$ExePath = Join-Path $BuildDir "perfectdarkremasterrecomp.exe"

Write-Host "=== ReXGlue Performance Benchmark ===" -ForegroundColor Cyan
Write-Host "Duration: $Duration seconds, Warmup skip: $SkipWarmup frames"
Write-Host ""

# Check if build dir and executable exist
if (-not (Test-Path $BuildDir)) {
    Write-Error "Build dir not found: $BuildDir"
    exit 1
}
if (-not (Test-Path $ExePath)) {
    Write-Error "Executable not found: $ExePath"
    exit 1
}

# Run D3D12 benchmark
Write-Host "Running D3D12 benchmark ($Duration seconds)..." -ForegroundColor Yellow
$d3d12_log = Join-Path $PSScriptRoot "benchmark_d3d12_$env:USERNAME.csv"

$process = Start-Process -FilePath $ExePath -ArgumentList "--graphics_backend=d3d12","--perf_log_csv=$d3d12_log" -WorkingDirectory $BuildDir -PassThru
Start-Sleep -Seconds $Duration
if (-not $process.HasExited) {
    Write-Host "Stopping D3D12 benchmark..." -ForegroundColor Gray
    Stop-Process -Id $process.Id -Force
    Start-Sleep -Seconds 2
}
Write-Host "D3D12 benchmark complete" -ForegroundColor Green

# Run Vulkan benchmark
Write-Host "Running Vulkan benchmark ($Duration seconds)..." -ForegroundColor Yellow
$vulkan_log = Join-Path $PSScriptRoot "benchmark_vulkan_$env:USERNAME.csv"

$process = Start-Process -FilePath $ExePath -ArgumentList "--graphics_backend=vulkan","--perf_log_csv=$vulkan_log" -WorkingDirectory $BuildDir -PassThru
Start-Sleep -Seconds $Duration
if (-not $process.HasExited) {
    Write-Host "Stopping Vulkan benchmark..." -ForegroundColor Gray
    Stop-Process -Id $process.Id -Force
    Start-Sleep -Seconds 2
}
Write-Host "Vulkan benchmark complete" -ForegroundColor Green

# Analyze results
function Analyze-Log {
    param([string]$Path, [int]$Skip)
    
    if (-not (Test-Path $Path)) {
        return $null
    }
    
    $lines = Get-Content $Path | Select-Object -Skip $Skip
    $header = (Get-Content $Path -TotalCount 1) -split ','
    
    $result = @{
        Frames = $lines.Count
        FPS = @(); FrameTime = @(); DrawCalls = @(); Vertices = @()
    }
    
    # Find column indices
    $fps_idx = $header.IndexOf('fps')
    $frame_time_idx = $header.IndexOf('frame_time_us')
    $draw_calls_idx = $header.IndexOf('draw_calls')
    $vertices_idx = $header.IndexOf('vertices_processed')
    
    foreach ($line in $lines) {
        $values = $line -split ','
        if ($values.Count -ge $header.Count) {
            if ($fps_idx -ge 0 -and $values[$fps_idx] -ne '0') { $result.FPS += [double]$values[$fps_idx] }
            if ($frame_time_idx -ge 0) { $result.FrameTime += [double]$values[$frame_time_idx] }
            if ($draw_calls_idx -ge 0) { $result.DrawCalls += [double]$values[$draw_calls_idx] }
            if ($vertices_idx -ge 0) { $result.Vertices += [double]$values[$vertices_idx] }
        }
    }
    
    # Calculate averages
    $result['AvgFPS'] = if ($result.FPS.Count -gt 0) { ($result.FPS | Measure-Object -Average).Average } else { 0 }
    $result['AvgFrameTime'] = if ($result.FrameTime.Count -gt 0) { ($result.FrameTime | Measure-Object -Average).Average } else { 0 }
    $result['AvgDrawCalls'] = if ($result.DrawCalls.Count -gt 0) { ($result.DrawCalls | Measure-Object -Average).Average } else { 0 }
    $result['AvgVertices'] = if ($result.Vertices.Count -gt 0) { ($result.Vertices | Measure-Object -Average).Average } else { 0 }
    
    return $result
}

$d3d12 = Analyze-Log $d3d12_log $SkipWarmup
$vulkan = Analyze-Log $vulkan_log $SkipWarmup

if ($null -eq $d3d12 -or $null -eq $vulkan) {
    Write-Warning "One or both benchmark logs missing or empty"
    exit 1
}

# Display comparison
Write-Host ""
Write-Host "=== Performance Comparison ===" -ForegroundColor Cyan
Write-Host ""
Write-Host ("{0,-20} {1,10} {2,10} {3,10}" -f "Metric", "D3D12", "Vulkan", "Diff")
Write-Host ("{0,-20} {1,10} {2,10} {3,10}" -f "--------", "-----", "------", "----")

$metrics = @(
    @("Avg FPS", $d3d12.AvgFPS, $vulkan.AvgFPS, 'fps'),
    @("Avg Frame Time (us)", $d3d12.AvgFrameTime, $vulkan.AvgFrameTime, 'us'),
    @("Avg Draw Calls", $d3d12.AvgDrawCalls, $vulkan.AvgDrawCalls, ''),
    @("Avg Vertices", $d3d12.AvgVertices, $vulkan.AvgVertices, '')
)

foreach ($m in $metrics) {
    $name = $m[0]; $d3d = $m[1]; $vul = $m[2]
    if ($d3d -ne 0) {
        $diff = (($vul - $d3d) / $d3d) * 100
        $diff_str = "{0:+0.1};{1}" -f $diff, "%"
    } else {
        $diff_str = "N/A"
    }
    Write-Host ("{0,-20} {1,10:F2} {2,10:F2} {3,10}" -f $name, $d3d, $vul, $diff_str)
}

Write-Host ""
Write-Host "Logs saved:" -ForegroundColor Gray
Write-Host "  D3D12:  $d3d12_log" -ForegroundColor Gray
Write-Host "  Vulkan: $vulkan_log" -ForegroundColor Gray
