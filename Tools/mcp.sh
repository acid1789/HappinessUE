#!/usr/bin/env bash
# Look up Unreal MCP functions without the MCP server or its large schemas. Prints only the result.
#   Tools/mcp.sh -toolsets
#   Tools/mcp.sh -find=widget [-max=40]
#   Tools/mcp.sh -list=BlueprintTools
#   Tools/mcp.sh -describe=BlueprintTools.AddNode
# Works on the checkout this script is in; read-only, runs alongside an open editor.
source "$(dirname -- "${BASH_SOURCE[0]}")/checkout_env.sh"
OUT="$SAVED/MCPLookup.txt"

rm -f "$OUT"
"$ENGINE/Binaries/Win64/UnrealEditor-Cmd.exe" "$PROJ" -run=HappinessMCPLookup -out="$OUT" "$@" \
  -unattended -nopause -nosplash -nullrhi -nosound -NoLogTimes > /dev/null 2>&1
[ -f "$OUT" ] && cat "$OUT" || echo "no output (exit $?); see $SAVED/Logs/Happiness.log"
