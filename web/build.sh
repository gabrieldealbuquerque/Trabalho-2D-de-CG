#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
output_dir="${1:-${project_dir}/web/dist}"
mkdir -p -- "$output_dir"
output_dir="$(cd -- "$output_dir" && pwd)"

if [[ -n "${EMSDK:-}" && -f "${EMSDK}/emsdk_env.sh" ]]; then
    source "${EMSDK}/emsdk_env.sh" >/dev/null 2>&1
fi
if ! command -v em++ >/dev/null 2>&1; then
    printf '%s\n' 'em++ nao encontrado. Ative o Emscripten SDK (emsdk_env.sh) ou defina EMSDK.' >&2
    exit 1
fi

cd -- "$project_dir"
em++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -fexceptions \
    -Isrc -Ithird_party/tinyxml2 \
    src/configuracao.cpp src/geometria.cpp src/jogo.cpp src/renderizador.cpp \
    third_party/tinyxml2/tinyxml2.cpp web/main.cpp \
    --embed-file configuracoes.xml@/configuracoes.xml \
    -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createCGGame \
    -sENVIRONMENT=web -sLEGACY_GL_EMULATION=1 -sGL_FFP_ONLY=1 \
    -sMAX_WEBGL_VERSION=1 -sALLOW_MEMORY_GROWTH=1 -sNO_EXIT_RUNTIME=1 \
    '-sEXPORTED_FUNCTIONS=["_main","_cg_key","_cg_aim","_cg_fire","_cg_pause","_cg_reset","_cg_release","_cg_state"]' \
    '-sEXPORTED_RUNTIME_METHODS=["UTF8ToString"]' \
    -o "$output_dir/game.js"
printf 'WebAssembly gerado em %s\n' "$output_dir"
