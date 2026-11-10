# Gaia OS 1.0 (Westmere Edition) — Repositório Central de Evolução Agêntica & Segurança

Este repositório centraliza a evolução autônoma, blindagem de segurança, otimizações de sistema e ferramentas de autossuficiência do **Gaia OS 1.0 (Westmere Edition)** rodando na máquina **Gateway NV55C (Intel Core i7 M 640 @ 2.80GHz)**.

> 📖 **Dossiê Completo de Arquitetura e Histórico:** Veja [`docs/DOSSIE_SISTEMA_GAIAOS.md`](file:///home/francisco/Documentos/Projetos/GaiaOS/docs/DOSSIE_SISTEMA_GAIAOS.md) para a documentação técnica aprofundada de todos os subsistemas.

---

## Matriz de Prioridades e Vulnerabilidades Mitigadas

| Prioridade | Alvo / Componente | CVEs Associados | Severidade | Risco Específico na Máquina (`francisco-nv55c`) | Status no Gaia OS |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **P1 (Crítica)** | **Linux Kernel IPv4 TCP / MPTCP** (`6.1.0-53` e `6.1.0-54`) | **CVE-2026-98432** | **CVSS 9.8 (Critical)** | *Use-After-Free (UAF)* por condição de corrida no handshake TCP (`tcp_v4_conn_request` / `ireq_opt` liberado via `kfree()` concorrente a leitor RCU) e listeners MPTCP. A máquina tinha portas expostas e `net.mptcp.enabled = 1`. | **Mitigado & Testado na camada Raw Prerouting** |
| **P2 (Alta)** | **Node.js Runtimes** (`node:zlib`, `node:dns`, `node:tls`) | **CVE-2026-111497** (*Node2Shell*), **CVE-2026-105482**, **CVE-2026-105483**, **CVE-2026-106069**, **CVE-2026-106072** | **CVSS 9.2 (Critical)** | O runtime embutido em `~/.cache/codex-runtimes/.../bin/node` estava na versão vulnerável **`v24.19.0`** (sujeita a escrita fora de limites no decodificador Brotli e bypass de TLS/DNS). | **Corrigido Localmente (`v24.22.1`) + Runtime Shield** |
| **P3 (Alta — Privacidade)** | **Ativos Críticos, Chaves & Credenciais Locais** | Exposição de Segredos / Escalada Local | **Alta (Privacidade)** | Chaves PEM/SSH (`~/chaves_e_pem`, `~/.ssh`), credenciais cloud (`~/.aws`, `~/.config/gcloud`), segredos OAuth (`~/Downloads/client_secret_*.json` com permissão `0644`), `.env` de projetos e banco PostgreSQL exposto em `0.0.0.0:5432`. | **Blindado & Containers Órfãos Eliminados** |
| **P4 (Preventiva)** | **PHP Runtime** (*PHP2Shell*) | **CVE-2026-109618**, **CVE-2026-109619**, **CVE-2026-115560** | **CVSS 9.2 (Critical)** | Nenhum runtime PHP está instalado no host. Política preventiva instalada em `/etc/gaiaos/security/99-gaia-php2shell-shield.ini` para bloquear `session.upload_progress` e execução arbitrária caso PHP seja adicionado futuramente. | **Prevenção Ativa** |

---

## Módulos e Melhorias do Sistema (Etapas de Evolução)

1. **Otimização Extrema de Memória (Etapa 1):**
   * **Multi-Gen LRU (MGLRU):** Habilitado via `/etc/tmpfiles.d/gaia-mglru.conf` (`0x0007`, `min_ttl_ms=1000`). Previne thrashing da interface gráfica.
   * **zRAM zstd + Page Cluster:** zRAM de 6.0 GiB configurada com `vm.page-cluster = 0` para eliminar I/O de swap desnecessário.
2. **Autonomia e Repositório Offline (Etapa 4):**
   * **Repositório APT Local:** Localizado em `/var/local/gaia-repo`, permitindo instalar e atualizar softwares do sistema sem dependência de rede externa.
   * **`gaia-pkg`:** Construtor offline de pacotes `.deb`.
   * **`gaia-snapshot`:** Gerenciador de pontos de restauração e rollback instantâneo do sistema.
3. **Serviços Enxutos e Políticas Chrome (Etapa 3):**
   * **Chrome 157:** Políticas corporativas de descarte agressivo de abas (`HighEfficiencyModeEnabled: true` e `MemorySaverModeSavings: 2`).
   * **Docker:** Configurado em **Socket Activation** (`docker.socket`), liberando ~150 MB de RAM quando inativo.
   * **Serviços Desativados:** `ModemManager` e `fwupd`.
4. **Gaia Guardian AI (Etapa 2):**
   * **C Daemon (`gaia-guardian`):** Monitor residente de baixo consumo (**~924 KB de RAM RSS**), monitorando temperatura `coretemp`, PSI e estado do MGLRU.
   * **Consultor Gemini 3.8 Flash (`gaia-gemini-advisor`):** Análise sob demanda ou por gatilho com consumo zero de RAM residente.
   * **Interface Gráfica (`gaia-guardian-ui`):** Dashboard PyQt5 sob demanda para visualização de métricas e tomada de decisões.
5. **Limpeza e Reivindicação de Disco (Etapa 5):**
   * Remoção de imagens antigas de máquinas virtuais e instaladores não mais úteis, liberando mais de 10 GB de armazenamento.
6. **Atalho de Produtividade `agy --yolo`:**
   * Wrapper universal em `/usr/local/bin/agy` e funções de shell que traduzem `--yolo` para `--dangerously-skip-permissions` no Antigravity CLI.

---

## Estrutura do Repositório

* [`bin/gaia-sec-apply`](file:///home/francisco/Documentos/Projetos/GaiaOS/bin/gaia-sec-apply) — Instalador e aplicador idempotente zero-network de todas as correções no sistema operacional.
* [`bin/gaia-pkg`](file:///home/francisco/Documentos/Projetos/GaiaOS/bin/gaia-pkg) — Construtor de pacotes `.deb` offline.
* [`bin/gaia-snapshot`](file:///home/francisco/Documentos/Projetos/GaiaOS/bin/gaia-snapshot) — Utilitário de snapshots e rollback de configurações.
* [`bin/gaia-lean-services`](file:///home/francisco/Documentos/Projetos/GaiaOS/bin/gaia-lean-services) — Gerenciador de serviços enxutos (Docker socket, ModemManager).
* [`bin/gaia-agy-yolo.sh`](file:///home/francisco/Documentos/Projetos/GaiaOS/bin/gaia-agy-yolo.sh) — Definição do wrapper e atalho `agy --yolo`.
* [`docs/DOSSIE_SISTEMA_GAIAOS.md`](file:///home/francisco/Documentos/Projetos/GaiaOS/docs/DOSSIE_SISTEMA_GAIAOS.md) — Dossiê técnico completo da arquitetura do Gaia OS.
* [`guardian/src/gaia-guardian.c`](file:///home/francisco/Documentos/Projetos/GaiaOS/guardian/src/gaia-guardian.c) — Código-fonte em C nativo do daemon de monitoramento e anti-congelamento.
* [`guardian/src/gaia-gemini-advisor`](file:///home/francisco/Documentos/Projetos/GaiaOS/guardian/src/gaia-gemini-advisor) — Consultor inteligente efêmero utilizando a API Gemini 3.8 Flash.
* [`guardian/ui/gaia-guardian-ui`](file:///home/francisco/Documentos/Projetos/GaiaOS/guardian/ui/gaia-guardian-ui) — Interface gráfica em PyQt5 sob demanda.
* [`performance/`](file:///home/francisco/Documentos/Projetos/GaiaOS/performance/) — Configurações de MGLRU, Chrome e `sysctl` de alta performance.
* [`security/`](file:///home/francisco/Documentos/Projetos/GaiaOS/security/) — Regras Netfilter, Node shield, PHP shield e sysctls de blindagem.
* [`tests/verify-security.sh`](file:///home/francisco/Documentos/Projetos/GaiaOS/tests/verify-security.sh) — Suíte completa de 21 testes automatizados do sistema.
