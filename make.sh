#!/usr/bin/env bash
set -euo pipefail

diretorio_projeto="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

if [[ $# -eq 0 ]]; then
    set -- all
fi

exec make -C "$diretorio_projeto" "$@"
