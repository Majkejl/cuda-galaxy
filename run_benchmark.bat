@echo off
REM ============================================================================
REM  run_benchmark.bat
REM
REM  Locks the GPU clocks to a steady value, runs the headless N-body benchmark
REM  (cuda_nbody.exe benchmark), then ALWAYS releases the clocks again.
REM
REM  REQUIREMENTS / GOTCHAS (see .claude/current-step.md Part 2 C-D):
REM   * Run from an ADMINISTRATOR prompt - clock locking needs elevation,
REM     otherwise --lock-gpu-clocks fails silently and you measure boost/throttle
REM     noise instead of the algorithm.
REM   * PLUG THE LAPTOP IN. On battery the dGPU power-throttles hard and the
REM     numbers are meaningless.
REM   * Close other GPU apps (browser/compositor) so they don't steal the dGPU.
REM   * If you Ctrl+C out mid-run, the reset lines at the bottom are SKIPPED -
REM     run them by hand (nvidia-smi --reset-gpu-clocks / --reset-memory-clocks).
REM ============================================================================
setlocal

REM Run from the script's own folder. When launched "as administrator" Windows
REM sets the working dir to C:\Windows\System32, so the exe's relative output
REM (performance.csv) would land there instead of the repo root. /d also switches
REM drive if needed. %~dp0 ends in a backslash, so quote it as-is.
cd /d "%~dp0"

REM Executable lives next to this script under build\Debug\ (%~dp0 = script dir).
set "EXE=%~dp0build\Debug\cuda_nbody.exe"

REM --- require Administrator --------------------------------------------------
net session >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Not elevated. Right-click this .bat and "Run as administrator".
    echo         Clock locking needs admin; aborting so you don't record throttled numbers.
    pause
    exit /b 1
)

if not exist "%EXE%" (
    echo [ERROR] Executable not found: %EXE%
    echo         Build the Debug target first, then re-run.
    pause
    exit /b 1
)

echo === Supported SM/memory clock pairs (pick an SM clock the card sustains) ===
nvidia-smi -q -d SUPPORTED_CLOCKS | findstr /C:"Graphics" /C:"Memory"
echo.

echo === Locking clocks =========================================================
REM Tunable: 1500 MHz SM sits below the ~2100 boost so it should hold without
REM thermal sag; 6001 is this card's max memory clock. Laptop GPUs sometimes
REM REFUSE locking - if these error out, the warm-up loop in Benchmark::run is
REM the (weaker) fallback to steady state. Verify with the monitor command below.
nvidia-smi --lock-gpu-clocks=1500
nvidia-smi --lock-memory-clocks=6001
echo.

REM --- optional: in a SEPARATE terminal, watch for throttling during the run ---
REM nvidia-smi --query-gpu=clocks.sm,clocks.mem,temperature.gpu,power.draw,pstate,clocks_throttle_reasons.active --format=csv -l 1

echo === Running benchmark ======================================================
"%EXE%" 1500
echo.

echo === Releasing clocks (always, even if the run above failed) =================
nvidia-smi --reset-gpu-clocks
nvidia-smi --reset-memory-clocks

echo.
echo Done.
endlocal
pause
