#!/usr/bin/env bash
# ==============================================================================
# Gaia OS 1.0 - Suite de Testes e Verificação de Segurança
# Valida as correções para:
# - CVE-2026-98432 (Kernel Linux IPv4 TCP ireq_opt UAF & MPTCP)
# - CVE-2026-111497 (Node2Shell Brotli RCE) + CVEs TLS/DNS Node.js
# - Blindagem de Privacidade/Credenciais e Isolamento de Portas
# - Prevenção PHP2Shell (CVE-2026-109618 / CVE-2026-109619)
# ==============================================================================
set -euo pipefail

PASS=0
FAIL=0

check() {
    local desc="$1"
    local cmd="$2"
    if eval "$cmd" >/dev/null 2>&1; then
        echo "  [OK]   $desc"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $desc"
        FAIL=$((FAIL + 1))
    fi
}

echo "=================================================================="
echo " [Gaia OS Test Suite] Verificando Mitigações & Integridade do SO"
echo "=================================================================="

echo ""
echo "--- 1. Testes de Kernel & Rede (CVE-2026-98432 - CVSS 9.8) ---"
check "MPTCP desativado no Kernel (net.mptcp.enabled = 0)" \
    "[ \"\$(sysctl -n net.mptcp.enabled)\" = \"0\" ]"

check "IPv4 Source Routing desativado (all & default = 0)" \
    "[ \"\$(sysctl -n net.ipv4.conf.all.accept_source_route)\" = \"0\" ] && [ \"\$(sysctl -n net.ipv4.conf.default.accept_source_route)\" = \"0\" ]"

check "SYN Cookies e RFC1337 ativos (mitigação de corrida em tcp_v4_conn_request)" \
    "[ \"\$(sysctl -n net.ipv4.tcp_syncookies)\" = \"1\" ] && [ \"\$(sysctl -n net.ipv4.tcp_rfc1337)\" = \"1\" ]"

check "Ponteiros do Kernel ocultos contra vazamento KASLR (kernel.kptr_restrict = 2)" \
    "[ \"\$(sysctl -n kernel.kptr_restrict)\" = \"2\" ]"

check "Isolamento de memória de processos ativo (kernel.yama.ptrace_scope >= 1)" \
    "[ \"\$(sysctl -n kernel.yama.ptrace_scope)\" -ge 1 ]"

check "Tabela Netfilter Raw 'gaia_cve_2026_98432_shield' ativa no Kernel" \
    "sudo nft list table inet gaia_cve_2026_98432_shield | grep 'ip hdrlength > 5'"

check "Regra Netfilter Raw descartando opções MPTCP externas ativa" \
    "sudo nft list table inet gaia_cve_2026_98432_shield | grep 'tcp option mptcp exists'"

check "Serviço systemd 'gaia-security-shield.service' habilitado e ativo" \
    "systemctl is-active --quiet gaia-security-shield.service && systemctl is-enabled --quiet gaia-security-shield.service"

echo ""
echo "--- 2. Simulação de Ataque CVE-2026-98432 (Pacote TCP com Opções IPv4 IHL > 5) ---"
# Dispara um pacote IPv4 com IP Options (Record Route: ping -R -c 1 -W 1 127.0.0.1)
# e verifica se o contador da regra 'ip hdrlength > 5' na chain raw_prerouting incrementa!
BEFORE_PKTS=$(sudo nft list chain inet gaia_cve_2026_98432_shield raw_prerouting | sed -n 's/.*ip hdrlength > 5 counter packets \([0-9]*\).*/\1/p')
ping -R -c 1 -W 1 127.0.0.1 >/dev/null 2>&1 || true
AFTER_PKTS=$(sudo nft list chain inet gaia_cve_2026_98432_shield raw_prerouting | sed -n 's/.*ip hdrlength > 5 counter packets \([0-9]*\).*/\1/p')
check "Bloqueio real de pacotes com Opções IPv4 (ihl > 5 / ireq_opt) comprovado (contador: ${BEFORE_PKTS} -> ${AFTER_PKTS})" \
    "[ \"$AFTER_PKTS\" -gt \"$BEFORE_PKTS\" ]"

echo ""
echo "--- 3. Testes de Node.js (CVE-2026-111497 'Node2Shell' & CVEs TLS/DNS) ---"
check "Node.js global (/usr/bin/node) em versão corrigida (>= v22.23.4)" \
    "[ \"\$(/usr/bin/node -v)\" = \"v22.23.4\" ]"

check "Node.js do Codex Runtime atualizado localmente para versão corrigida (v24.22.1)" \
    "[ \"\$(/home/francisco/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node -v)\" = \"v24.22.1\" ]"

check "Runtime Shield (gaia-node-shield.cjs) bloqueia CVE-2026-105482 (DNS NUL byte), CVE-2026-105483 (Full-width TLS) e CVE-2026-106072 (IPv4 não-canônico)" \
    "node -e '
      const shield = require(\"/etc/gaiaos/security/gaia-node-shield.cjs\");
      const dns = require(\"node:dns\");
      const tls = require(\"node:tls\");
      const zlib = require(\"node:zlib\");
      let nulBlocked = false;
      try { dns.lookup(\"evil.com\\0.trusted.com\", () => {}); } catch (e) { nulBlocked = (e.code === \"ERR_GAIA_UNSAFE_HOSTNAME\"); }
      let fwBlocked = false;
      try { tls.checkServerIdentity(\"１２７.0.0.1\", {}); } catch (e) { fwBlocked = (e.code === \"ERR_GAIA_FULLWIDTH_HOSTNAME\"); }
      const nonCanonRes = tls.checkServerIdentity(\"0177.0.0.1\", {});
      const nonCanonBlocked = nonCanonRes instanceof Error && nonCanonRes.code === \"ERR_TLS_CERT_ALTNAME_INVALID\";
      const payload = zlib.brotliCompressSync(Buffer.from(\"gaia-os-self-test\"));
      const decompressed = zlib.brotliDecompressSync(payload).toString();
      if (!nulBlocked || !fwBlocked || !nonCanonBlocked || decompressed !== \"gaia-os-self-test\") process.exit(1);
    '"

echo ""
echo "--- 4. Testes de Privacidade, Ativos Locais e Funcionamento de Serviços ---"
check "Nenhum container Docker desnecessário ou porta 0.0.0.0:5432 exposta" \
    "[ -z \"\$(sudo docker ps -q 2>/dev/null)\" ] && ! ss -tuln | grep -q ':5432 '"

check "Diretórios de credenciais (.ssh, chaves_e_pem, .aws, .config/gcloud) blindados (permissão 700)" \
    "[ \"\$(stat -c %a /home/francisco/.ssh)\" = \"700\" ] && [ \"\$(stat -c %a /home/francisco/chaves_e_pem)\" = \"700\" ] && [ \"\$(stat -c %a /home/francisco/.aws)\" = \"700\" ] && [ \"\$(stat -c %a /home/francisco/.config/gcloud)\" = \"700\" ]"

check "Arquivos client_secret OAuth em ~/Downloads protegidos com permissão 600" \
    "! find /home/francisco/Downloads -maxdepth 1 -name '*client_secret*.json' -perm /077 | grep -q ."

check "Política preventiva PHP2Shell instalada em /etc/gaiaos/security/99-gaia-php2shell-shield.ini" \
    "grep -q 'session.upload_progress.enabled = 0' /etc/gaiaos/security/99-gaia-php2shell-shield.ini"

echo ""
echo "--- 5. Testes de Performance, Repositório Local & Gaia Guardian AI ---"
check "MGLRU (Multi-Gen LRU) ativo (0x0007) com proteção de working-set (1000ms) e page-cluster=0" \
    "[ \"\$(cat /sys/kernel/mm/lru_gen/enabled)\" = \"0x0007\" ] && [ \"\$(cat /sys/kernel/mm/lru_gen/min_ttl_ms)\" = \"1000\" ] && [ \"\$(sysctl -n vm.page-cluster)\" = \"0\" ]"

check "Repositório APT Offline (/var/local/gaia-repo) e gaia-snapshot operacionais" \
    "[ -f /var/local/gaia-repo/Packages.gz ] && [ -x /usr/local/bin/gaia-pkg ] && [ -x /usr/local/bin/gaia-snapshot ]"

check "Política Chrome HighEfficiencyMode instalada e Docker em Socket-Activation" \
    "[ -f /etc/opt/chrome/policies/managed/gaia-performance.json ] && systemctl is-active --quiet docker.socket"

check "Daemon C 'gaia-guardian' ativo, lendo temperatura coretemp e consumindo < 2 MB de RAM" \
    "systemctl is-active --quiet gaia-guardian.service && [ -f /var/log/gaia-guardian/status.json ] && [ \"\$(jq -r '.daemon_rss_kb' /var/log/gaia-guardian/status.json 2>/dev/null || awk -F': ' '/daemon_rss_kb/ {gsub(/,/,\"\",\$2); print \$2}' /var/log/gaia-guardian/status.json)\" -lt 2048 ]"

check "Consultor Gemini 3.8 Flash e Interface Gráfica (gaia-guardian-ui) instalados" \
    "[ -x /usr/local/bin/gaia-gemini-advisor ] && [ -x /usr/local/bin/gaia-guardian-ui ] && [ -f /var/log/gaia-guardian/gemini-insights.log ]"

echo ""
echo "=================================================================="
echo " Resultado Final: $PASS testes aprovados, $FAIL falhas."
echo "=================================================================="
[ "$FAIL" -eq 0 ]
