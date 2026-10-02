#!/usr/bin/env bash
# Headless puzzle generate/solve via the Puzzle commandlet. Prints only the result file.
#   Tools/puzzle.sh -seeds=1-500 -size=6 -diff=2          (batch, failures + summary)
#   Tools/puzzle.sh -seed=42 -mode=show|trace             (full detail for one puzzle)
#   Tools/puzzle.sh --build ...                           (rebuild HappinessEditor first)
# Works on the checkout this script is in. --build needs that checkout's editor closed (Tools/editor.sh stop).
source "$(dirname -- "${BASH_SOURCE[0]}")/checkout_env.sh"
OUT="$SAVED/PuzzleCLI.txt"
BUILD_LOG="$SAVED/PuzzleBuild.log"

if [ "$1" = "--build" ]; then
  shift
  if [ "$(checkout_editor_count)" != "0" ]; then
    echo "This checkout's editor is open and locks the module DLL. Save, then Tools/editor.sh stop, and build again." >&2
    exit 1
  fi
  # UnrealBuildTool keeps one log for every checkout (%LOCALAPPDATA%/UnrealBuildTool/Log.txt), so a build
  # started while another checkout is building fails at once. Wait for it.
  ubt_running() {
    powershell.exe -NoProfile -NonInteractive -Command \
      "@(Get-CimInstance Win32_Process -Filter \"Name = 'dotnet.exe'\" | Where-Object { \$_.CommandLine -match 'UnrealBuildTool' }).Count" \
      2>/dev/null | tr -d '\r'
  }
  waited=0
  while [ "$(ubt_running)" != "0" ]; do
    [ "$waited" = 0 ] && echo "Waiting for another checkout's build to finish..." >&2
    waited=$((waited + 10))
    [ "$waited" -ge 1800 ] && { echo "Another build is still running after 30 minutes" >&2; exit 1; }
    sleep 10
  done
  mkdir -p "$SAVED"
  "$ENGINE/Build/BatchFiles/Build.bat" HappinessEditor Win64 Development -Project="$PROJ" -WaitMutex > "$BUILD_LOG" 2>&1 \
    || { grep -E "error|Error|Unable to build" "$BUILD_LOG" | head -20; exit 1; }
fi

rm -f "$OUT"
"$ENGINE/Binaries/Win64/UnrealEditor-Cmd.exe" "$PROJ" -run=Puzzle -out="$OUT" "$@" \
  -unattended -nopause -nosplash -nullrhi -nosound -NoLogTimes > /dev/null 2>&1
[ -f "$OUT" ] && cat "$OUT" || echo "no output (exit $?); see $SAVED/Logs/Happiness.log"
