# Gaia OS 1.0 (Westmere Edition) — Repositório Central de Evolução Agêntica & Segurança

Este repositório centraliza a evolução autônoma, blindagem de segurança e otimizações de sistema do **Gaia OS 1.0 (Westmere Edition)** rodando na máquina **Gateway NV55C (Intel Core i7 M 640)**.

---

## Matriz de Prioridades e Vulnerabilidades Mitigadas (Novembro 2026)

| Prioridade | Alvo / Componente | CVEs Associados | Severidade | Risco Específico na Máquina (`francisco-nv55c`) | Status no Gaia OS |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **P1 (Crítica)** | **Linux Kernel IPv4 TCP / MPTCP** (`6.1.0-53-amd64`) | **CVE-2026-98432** | **CVSS 9.8 (Critical)** | *Use-After-Free (UAF)* por condição de corrida no handshake TCP (`tcp_v4_conn_request` / `ireq_opt` liberado via `kfree()` concorrente a leitor RCU) e listeners MPTCP. A máquina tinha portas TCP expostas em `0.0.0.0:5432` (Docker PostgreSQL) e `*:1716` (KDE Connect), com `net.mptcp.enabled = 1` e `kernel.kptr_restrict = 0`. | **Mitigado & Testado** |
| **P2 (Alta)** | **Node.js Runtimes** (`node:zlib`, `node:dns`, `node:tls`) | **CVE-2026-111497** (*Node2Shell*), **CVE-2026-105482**, **CVE-2026-105483**, **CVE-2026-106069**, **CVE-2026-106072** | **CVSS 9.2 (Critical)** | O Node global (`/usr/bin/node`) já estava em `v22.23.4`, mas o runtime embutido em `~/.cache/codex-runtimes/.../bin/node` estava na versão vulnerável **`v24.19.0`** (sujeita a escrita fora de limites no decodificador Brotli e bypass de TLS/DNS). | **Corrigido Localmente (`v24.22.1`) + Runtime Shield** |
| **P3 (Alta — Privacidade)** | **Ativos Críticos, Chaves & Credenciais Locais** | Exposição de Segredos / Escalada Local | **Alta (Privacidade)** | Chaves PEM/SSH (`~/chaves_e_pem`, `~/.ssh`), credenciais cloud (`~/.aws`, `~/.boto`, `~/.config/gcloud`), segredos OAuth (`~/Downloads/client_secret_*.json` com permissão `0644`), `.env` de projetos e banco PostgreSQL exposto em `0.0.0.0:5432`. | **Blindado & Isolado** |
| **P4 (Preventiva)** | **PHP Runtime** (*PHP2Shell*) | **CVE-2026-109618**, **CVE-2026-109619**, **CVE-2026-115560** | **CVSS 9.2 (Critical)** | Nenhum runtime PHP está instalado no host ou containers no momento. Política preventiva instalada para bloquear `session.upload_progress` e corrupção de estado caso PHP seja adicionado futuramente. | **Prevenção Ativa** |

---

## Estrutura do Projeto

* [`bin/gaia-sec-apply`](file:///home/francisco/Documentos/Projetos/GaiaOS/bin/gaia-sec-apply) — Instalador e aplicador idempotente zero-network de todas as correções no sistema operacional.
* [`tests/verify-security.sh`](file:///home/francisco/Documentos/Projetos/GaiaOS/tests/verify-security.sh) — Suíte automatizada de 16 testes de verificação (incluindo simulação real de pacotes com opções IPv4 `ihl > 5` bloqueados na camada `raw prerouting`).
* [`security/kernel/98-gaia-security-cve-2026-98432.conf`](file:///home/francisco/Documentos/Projetos/GaiaOS/security/kernel/98-gaia-security-cve-2026-98432.conf) — Hardening de Kernel via `sysctl` (`net.mptcp.enabled=0`, `kernel.kptr_restrict=2`, `kernel.yama.ptrace_scope=1`, `tcp_rfc1337=1`, `rp_filter=1`).
* [`security/network/gaia-cve-2026-98432-shield.nft`](file:///home/francisco/Documentos/Projetos/GaiaOS/security/network/gaia-cve-2026-98432-shield.nft) — Escudo `nftables` em `raw prerouting` (prioridade `-300`, executando antes do roteamento e antes de `tcp_v4_rcv`/`tcp_v4_conn_request`) que descarta pacotes com opções IPv4 (`ip hdrlength > 5`), opções TCP `mptcp`, flags TCP inválidas, rajadas de corrida SYN e acessos externos à porta `5432`.
* [`security/node/gaia-node-shield.cjs`](file:///home/francisco/Documentos/Projetos/GaiaOS/security/node/gaia-node-shield.cjs) — Módulo de defesa em profundidade para Node.js contra *Node2Shell* e bypasses de DNS/TLS.
* [`security/php/99-gaia-php2shell-shield.ini`](file:///home/francisco/Documentos/Projetos/GaiaOS/security/php/99-gaia-php2shell-shield.ini) — Blindagem preventiva contra *PHP2Shell*.
* [`systemd/gaia-security-shield.service`](file:///home/francisco/Documentos/Projetos/GaiaOS/systemd/gaia-security-shield.service) — Serviço `systemd` para persistência automática no boot.
