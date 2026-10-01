#!/usr/bin/env bash
# Render through the gated editor for this checkout; leave it running afterwards.
set -euo pipefail
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
WIDGET="${1:-}"; W="${2:-1920}"; H="${3:-1080}"
if [ -z "$WIDGET" ]; then
  echo "usage: Tools/render_ui.sh /Game/Path/WBP_Name [width] [height]" >&2
  exit 1
fi
"$SCRIPT_DIR/editor.sh" start
ARGS="$(python -c 'import json,sys; print(json.dumps(dict(widgetBlueprintPath=sys.argv[1],width=int(sys.argv[2]),height=int(sys.argv[3]),outputFile="")))' "$WIDGET" "$W" "$H")"
python "$SCRIPT_DIR/mcp_call.py" HappinessMCPLookup.HappinessUIToolset RenderWidget "$ARGS" \
  | python -c 'import json,sys; d=json.load(sys.stdin); r=d["result"]; t=r["content"][0]["text"]; sys.exit(t) if r.get("isError") else print(json.loads(t).get("returnValue",t) if t.startswith("{") else t)'
