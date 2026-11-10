#!/usr/bin/env bash
# ==============================================================================
# Gaia OS 1.0 - Wrapper & Atalho 'agy --yolo'
# Traduz automaticamente '--yolo' para '--dangerously-skip-permissions'
# ==============================================================================

# 1. Função exportável para shells bash interativos:
agy() {
    local translated_args=()
    local arg
    for arg in "$@"; do
        if [ "$arg" = "--yolo" ]; then
            translated_args+=("--dangerously-skip-permissions")
        else
            translated_args+=("$arg")
        fi
    done
    command agy "${translated_args[@]}"
}
export -f agy 2>/dev/null || true

# 2. Quando executado diretamente como script autônomo (ex: /usr/local/bin/agy):
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    args=()
    for arg in "$@"; do
        if [ "$arg" = "--yolo" ]; then
            args+=("--dangerously-skip-permissions")
        else
            args+=("$arg")
        fi
    done

    REAL_AGY=""
    if [ -n "$HOME" ] && [ -x "$HOME/.local/bin/agy" ] && [ "$HOME/.local/bin/agy" != "$0" ]; then
        REAL_AGY="$HOME/.local/bin/agy"
    elif [ -x "/home/francisco/.local/bin/agy" ] && [ "/home/francisco/.local/bin/agy" != "$0" ]; then
        REAL_AGY="/home/francisco/.local/bin/agy"
    elif [ -x "/root/.local/bin/agy" ] && [ "/root/.local/bin/agy" != "$0" ]; then
        REAL_AGY="/root/.local/bin/agy"
    fi

    if [ -n "$REAL_AGY" ]; then
        exec "$REAL_AGY" "${args[@]}"
    else
        echo "Erro: Binário compilado do agy não encontrado em ~/.local/bin/agy." >&2
        exit 1
    fi
fi
