#!/usr/bin/env bash
# Run from this checkout (or a subdirectory). Requires HAPPINESS_AGENT_ID.
# start takes this checkout's editor (launching it on the checkout's MCP port if needed); stop closes it and
# keeps ownership for rebuilding; release hands it back and leaves it running. Other checkouts are unaffected.
set -euo pipefail
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
exec python "$SCRIPT_DIR/editor.py" "$@"
