#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

if ! command -v make >/dev/null 2>&1 || ! command -v "${CXX:-g++}" >/dev/null 2>&1; then
    if [[ -f /tmp/cg-env.sh ]]; then
        source /tmp/cg-env.sh
    fi
fi

if ! command -v make >/dev/null 2>&1 || ! command -v "${CXX:-g++}" >/dev/null 2>&1; then
    printf '%s\n' 'Instale as dependencias: sudo apt install g++ make freeglut3-dev' >&2
    exit 1
fi

if [[ $# -eq 0 ]]; then
    set -- all
fi

exec make -C "$project_dir" "$@"
