#!/usr/bin/env bash
# Run from this checkout (or a subdirectory). Requires HAPPINESS_AGENT_ID.
# start acquires the shared gate; stop retains it for rebuilding; release hands it back.
set -euo pipefail
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
exec python "$SCRIPT_DIR/editor.py" "$@"
