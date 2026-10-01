#!/usr/bin/env bash
# Sourced by the Tools/*.sh scripts: paths for the checkout these scripts live in, never a hardcoded clone.
#   ROOT    the checkout root          PROJ    its .uproject
#   ENGINE  the checkout's Engine link SAVED   the checkout's Saved dir

TOOLS_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd -- "$TOOLS_DIR/.." && pwd)"

shopt -s nullglob
_projects=("$ROOT"/*.uproject)
shopt -u nullglob
if [ "${#_projects[@]}" -ne 1 ]; then
  echo "Expected exactly one .uproject in $ROOT" >&2
  exit 1
fi
PROJ="$(cygpath -m "${_projects[0]}")"
SAVED="$(cygpath -m "$ROOT")/Saved"

# Every checkout has an Engine symlink to the installed engine
ENGINE="$(cygpath -m "$ROOT")/Engine"
if [ ! -f "$ENGINE/Binaries/Win64/UnrealEditor-Cmd.exe" ]; then
  echo "No engine at $ENGINE: link the checkout's Engine folder to the installed engine" >&2
  exit 1
fi

# Number of editors open on this checkout's project
checkout_editor_count() {
  (cd "$ROOT" && python -c 'import sys; sys.path.insert(0, "Tools")
import editor_gate
print(len(editor_gate.project_editors(sys.argv[1])))' "$PROJ")
}
