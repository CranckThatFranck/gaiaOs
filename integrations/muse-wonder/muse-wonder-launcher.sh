#!/usr/bin/env bash
# Gaia OS - Muse Wonder 1.0 Dedicated Agent Launcher
# Meta Frontier Coding Agent connected to Google Cloud Vertex AI Serverless

export META_API_KEY="${META_API_KEY:-gcp-vertex-proxy}"

if [ "$#" -eq 0 ]; then
    # Interactive TUI mode
    exec /usr/local/bin/muse --model muse-wonder-1.0 --reasoning-effort high --trust-workspace
else
    # Subcommand or prompt execution
    if [[ "$1" == "exec" || "$1" == "resume" || "$1" == "review" || "$1" == "mcp" || "$1" == "skills" ]]; then
        exec /usr/local/bin/muse "$@"
    else
        exec /usr/local/bin/muse exec --model muse-wonder-1.0 --reasoning-effort high --trust-workspace --disable-reminders "$@"
    fi
fi
