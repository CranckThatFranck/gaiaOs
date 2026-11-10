#!/usr/bin/env bash
# Gaia OS - Muse Code CLI Wrapper
# Routes requests via local Vertex AI bridge (http://127.0.0.1:18787)
# Powered by Meta Muse Wonder 1.0 (negocio-de-bolso-492422)

export META_API_KEY="${META_API_KEY:-gcp-vertex-proxy}"

# Ensure bridge daemon is active
if ! systemctl is-active --quiet gaia-muse-bridge.service 2>/dev/null; then
    sudo systemctl start gaia-muse-bridge.service 2>/dev/null || true
fi

exec /opt/muse/bin/muse "$@"
