# Dossier Técnico e Arquitetura do Gaia OS 1.0

> **Distribuição:** Gaia OS 1.0 (Debian GNU/Linux 12 Bookworm Customizado)  
> **Plataforma:** Gateway NV55C — Intel Core i7 M 640 @ 2.80GHz (Westmere/Arrandale, 2C/4T)  
> **Memória:** 5.5 GiB DDR3 física + 6.0 GiB zRAM (compressão `zstd`)  
> **Interface Gráfica:** KDE Plasma (X11)  
> **Modelo Agêntico de Manutenção:** Antigravity / Gemini 3.8 Flash  
> **Data de Atualização:** Novembro de 2026  

---

## 1. Visão Geral e Filosofia do Sistema

O **Gaia OS** nasceu da necessidade de transformar um notebook legado (Gateway NV55C com processador Intel Core i7 de 1ª Geração) em uma estação de trabalho e gateway de desenvolvimento moderna, ultra-responsiva, segura e autossuficiente.

Diferente de distribuições convencionais que acumulam serviços em segundo plano e demandam constantes downloads de pacotes pesados pela rede ou pendrives para restauração, o Gaia OS foi concebido sob três princípios fundamentais:

1. **Eficiência Milimétrica em Hardware Antigo:** Eliminar completamente o desperdício de ciclos de CPU e páginas de memória, aproveitando recursos modernos do kernel Linux (como MGLRU e zRAM) afinados para a microarquitetura Westmere.
2. **Segurança Proativa e Defesa em Profundidade:** Mitigação de vulnerabilidades críticas de dia-zero (como o CVE-2026-98432 no TCP IPv4 e o CVE-2026-111497 'Node2Shell') na borda do sistema, antes mesmo que os pacotes ou códigos atinjam camadas vulneráveis.
3. **Autonomia e Autossuficiência Offline:** Capacidade do próprio sistema operacional construir seus próprios pacotes `.deb`, gerenciar seu repositório APT local offline (`/var/local/gaia-repo`) e criar snapshots incrementais com rollback imediato.

---

## 2. Auditoria de Atualizações do Sistema (Kernel 6.1.190-1 & Chrome 157)

Em Novembro de 2026, o sistema recebeu atualizações do Debian Bookworm e do Google Chrome. Uma auditoria detalhada foi realizada para verificar compatibilidade e necessidade de ajustes:

### 2.1. Kernel Linux 6.1.190-1 (`linux-image-6.1.0-54-amd64`)
* **Changelog e CVEs:** O pacote incorporou patches upstream para dezenas de vulnerabilidades de ALSA, temporizadores, `mm/vmscan` (CVE-2026-89753), `mm/migrate` (CVE-2026-89756), `memcg` (CVE-2026-89752), NFS/NFSD e MPTCP syncookies (CVE-2026-97568).
* **Diagnóstico de MGLRU:** A inspeção do `/boot/config-6.1.0-54-amd64` revelou que `CONFIG_LRU_GEN=y`, porém `# CONFIG_LRU_GEN_ENABLED is not set`. Isso significa que, sem a nossa configuração persistente, o novo kernel inicializaria com o MGLRU **desativado por padrão** (`0x0000`).
  * **Conclusão:** As configurações criadas em `/etc/tmpfiles.d/gaia-mglru.conf` e `/etc/sysctl.d/99-performance.conf` **NÃO se tornaram obsoletas**, sendo **fundamentais** para garantir que a ativação do MGLRU (`0x0007`) persista no reboot para o novo kernel.
* **Escudo CVE-2026-98432:** O drop antecipado na tabela `raw` do Netfilter (`gaia-cve-2026-98432-shield.nft`) permanece ativo como primeira linha de defesa, descartando pacotes maliciosos com opções IPv4 antes do processamento pela pilha IP/TCP.

### 2.2. Google Chrome 157 (`157.0.8103.50-1`)
* **Políticas Corporativas de Desempenho:** A política legado `HighEfficiencyModeEnabled` continua válida. No entanto, o Chrome 157 introduziu a política `MemorySaverModeSavings` para configurar o nível de agressividade no descarte de abas inativas.
* **Correção/Aprimoramento Aplicado:** Atualizamos `/etc/opt/chrome/policies/managed/gaia-performance.json` para incluir `"MemorySaverModeSavings": 2` (modo de economia máximo), garantindo que abas em segundo plano sejam descartadas rapidamente para poupar RAM na máquina de 5.5 GiB.

---

## 3. Arquitetura dos Módulos do Gaia OS

```
                     +---------------------------------------+
                     |              Gaia OS 1.0              |
                     |  Gateway NV55C (Intel Core i7 M 640)  |
                     +-------------------+-------------------+
                                         |
         +-------------------------------+-------------------------------+
         |                               |                               |
+--------v--------+             +--------v--------+             +--------v--------+
|   Segurança     |             |   Performance   |             | Autossuficiência|
|   & Escudos     |             |   & Memória     |             |    & Repositório|
+--------+--------+             +--------+--------+             +--------+--------+
| Netfilter Raw   |             | MGLRU (0x0007)  |             | Repo APT Local  |
| CVE-2026-98432  |             | zRAM zstd (6GB) |             | gaia-pkg        |
| Node2Shell      |             | Chrome Max Saver|             | gaia-snapshot   |
| Hardening Ativos|             | Socket Docker   |             | Zero-Network    |
+--------+--------+             +--------+--------+             +-----------------+
         |                               |
         +---------------+---------------+
                         |
           +-------------v-------------+
           |     Gaia Guardian AI      |
           +-------------+-------------+
           | C Daemon (< 1 MB RSS)     |
           | Telemetria /sys hwmon     |
           | Anti-Freeze & PSI Watchdog|
           | Gemini 3.8 Flash Advisor  |
           | PyQt5 UI (Sob Demanda)    |
           +---------------------------+
```

---

### Módulo 1: Segurança Proativa & Mitigações de Vulnerabilidades

1. **Kernel IPv4 TCP (CVE-2026-98432 - CVSS 9.8):**
   * **Causa Raiz:** Corrupção de memória e corrida no manuseio de opções IPv4 e cabeçalhos estendidos em conexões TCP de alta taxa.
   * **Mitigação:** Regra Netfilter Raw descartando pacotes com `ip hdrlength > 5` direcionados a portas TCP locais, drop de pacotes com opção TCP MPTCP (30), desativação de `net.mptcp.enabled = 0`, desativação de IPv4 Source Routing e ativação de RFC 1337 (`net.ipv4.tcp_rfc1337 = 1`).
   * **Persistência:** Serviço systemd `/etc/systemd/system/gaia-security-shield.service`.

2. **Node.js (CVE-2026-111497 'Node2Shell', CVE-2026-105482, CVE-2026-105483, CVE-2026-106072):**
   * **Mitigação:** Atualização do runtime local do Codex para `v24.22.1` e implantação do hook de runtime `/etc/gaiaos/security/gaia-node-shield.cjs` injetado via `NODE_OPTIONS=--require`, neutralizando NUL bytes em resolução DNS, domínios TLS com caracteres Unicode full-width e endereços IPv4 octais/hexadecimais não-canônicos.

3. **PHP2Shell (CVE-2026-109618 / 109619):**
   * **Mitigação:** Criação de `/etc/gaiaos/security/99-gaia-php2shell-shield.ini` desativando funções de execução arbitrária (`system`, `exec`, `passthru`, `proc_open`, etc.) e desabilitando upload de arquivos em instâncias PHP locais de desenvolvimento.

4. **Blindagem de Ativos Locais e Credenciais:**
   * Diretórios sensíveis (`~/.ssh`, `~/chaves_e_pem`, `~/.aws`, `~/.config/gcloud`) restritos para permissão octal `0700`.
   * Arquivos de credenciais (`client_secret` OAuth, `.env`, chaves `.pem`, `known_hosts`) restritos para permissão octal `0600`.
   * Remoção de containers Docker órfãos que mantinham portas como `0.0.0.0:5432` expostas desnecessariamente.

---

### Módulo 2: Otimização de Memória & Kernel para Westmere

1. **Multi-Gen LRU (MGLRU):**
   * O algoritmo tradicional de substituição de páginas do Linux (active/inactive lists) sofre de "thrashing" em multitarefa pesada quando a memória disponível cai abaixo de 20%.
   * O MGLRU organiza as páginas em múltiplas gerações cronológicas e protege o working set da interface gráfica.
   * Ativação persistente via `/etc/tmpfiles.d/gaia-mglru.conf`:
     * `/sys/kernel/mm/lru_gen/enabled = 7` (0x0007: Pagecache + Memória Anônima + Shmem).
     * `/sys/kernel/mm/lru_gen/min_ttl_ms = 1000` (Protege páginas ativas por 1 segundo).

2. **Ajuste Fino do zRAM:**
   * Configurado para `6.0 GiB` com compressão `zstd` (taxa de compressão média de 3.6x a 4.1x).
   * `vm.page-cluster = 0` configurado em `/etc/sysctl.d/99-performance.conf`: desativa a leitura antecipada de 32 KB de swap convencional, otimizando o acesso aleatório de baixa latência da memória comprimida na CPU Westmere.

3. **Desativação de Serviços Ociosos:**
   * Docker configurado em **Socket Activation** (`systemctl disable docker && systemctl enable docker.socket`). O daemon do Docker só é iniciado na memória quando o comando `docker` for invocado, liberando ~150 MB de RAM.
   * `ModemManager` e `fwupd` desativados e mascarados, pois a máquina não possui modem celular nem BIOS atualizável via fwupd.

---

### Módulo 3: Autossuficiência Offline (Pacotes & Snapshots)

1. **Repositório APT Local (`/var/local/gaia-repo`):**
   * Configurado na lista de fontes do sistema (`/etc/apt/sources.list.d/gaia-local.list`).
   * Permite instalar, atualizar e reconstruir componentes do Gaia OS sem conexão de rede.

2. **Construtor de Pacotes (`/usr/local/bin/gaia-pkg`):**
   * Constrói pacotes `.deb` a partir de estruturas padronizadas de diretório com arquivo `DEBIAN/control`.
   * Gera somas criptográficas `Packages.gz` e `Release` atualizando o repositório local instantaneamente.

3. **Gerenciador de Snapshots (`/usr/local/bin/gaia-snapshot`):**
   * Cria backups de estado das configurações críticas (`/etc/sysctl.d`, `/etc/gaiaos`, `/etc/tmpfiles.d`, systemd units) e da lista de pacotes do sistema.
   * Suporta criação com `gaia-snapshot create "nome"` e restauração automática com `gaia-snapshot rollback <id>`.

---

### Módulo 4: Gaia Guardian AI (C Daemon & Gemini 3.8 Flash)

Para monitorar e otimizar a máquina em tempo real sem comprometer os escassos 5.5 GiB de RAM, desenvolvemos uma arquitetura híbrida inteligente:

1. **Daemon Nativo em C (`gaia-guardian`):**
   * Código em C padrão sem dependências de frameworks pesados (`glibc` pura).
   * Consumo residente de memória: **~924 KB de RAM (RSS)** (menos de 1 MB!).
   * Lê diretamente as métricas de hardware:
     * Sensores de temperatura `coretemp` via `/sys/class/hwmon/hwmon*/temp*_input`.
     * Memória física e zRAM via `/proc/meminfo`.
     * Pressão de stalls de memória via `/proc/pressure/memory` (PSI).
     * Estado do MGLRU via `/sys/kernel/mm/lru_gen/enabled`.
   * Registra telemetria a cada 10 segundos em `/var/log/gaia-guardian/status.json`.
   * **Watchdog Anti-Congelamento:** Se o PSI `full avg10` exceder 25%, aciona proativamente `posix_fadvise` e sincronização de páginas limpas para evitar que o sistema trave.

2. **Consultor Efêmero Gemini 3.8 Flash (`gaia-gemini-advisor`):**
   * Invocado apenas quando necessário (análise sob demanda ou anomalia grave).
   * Utiliza a chave `GEMINI_API_KEY` do ambiente para consultar o modelo `gemini-3.8-flash`.
   * Recebe as métricas de telemetria e retorna uma recomendação estruturada em JSON (`action`, `reason`, `advice`).
   * Consumo residente em segundo plano: **Zero bytes de RAM** (o processo nasce, consulta a API e se encerra imediatamente).

3. **Interface Gráfica Sob Demanda (`gaia-guardian-ui`):**
   * Aplicação em PyQt5 acessível via menu de aplicativos ou pelo comando `gaia-guardian-ui`.
   * Apresenta medidores visuais de RAM, zRAM, temperatura da CPU, alertas recentes e conselhos do Gemini.
   * O usuário pode abrir para inspecionar e fechar a qualquer momento; nenhuma janela ou processo gráfico permanece ocupando memória quando fechado.

---

### Módulo 5: Atalho `agy --yolo`

Com a migração do legado `gemini-cli` para o **Antigravity CLI (`agy`)**, o usuário estava habituado à conveniência da flag `--yolo` para aprovação automática de comandos e ferramentas.

* **Implementação:**
  * Wrapper executável universal em `/usr/local/bin/agy`.
  * Função exportada em `/home/francisco/.bashrc`, `/root/.bashrc` e `/etc/bash.bashrc`.
* **Comportamento:**
  * Qualquer invocação de `agy --yolo <comandos>` intercepta o argumento `--yolo` e o traduz transparentemente para `--dangerously-skip-permissions`.
  * O binário oficial do `agy` é preservado e todas as demais flags e subcomandos continuam funcionando perfeitamente.

---

## 4. Guia Rápido de Comandos do Gaia OS

| Comando | Descrição |
| :--- | :--- |
| `gaia-sec-verify` | Executa a suíte de testes de integridade do Gaia OS (21 testes de segurança, performance e rede). |
| `gaia-guardian-ui` | Abre a interface gráfica de telemetria, saúde do sistema e conselhos da IA. |
| `gaia-gemini-advisor` | Executa uma consulta imediata ao Gemini 3.8 Flash via terminal. |
| `gaia-snapshot list` | Lista todos os pontos de restauração e snapshots salvos. |
| `gaia-snapshot create "motivo"` | Cria um snapshot instantâneo do estado do sistema. |
| `gaia-pkg build <diretorio>` | Constrói um pacote `.deb` e o indexa no repositório APT local. |
| `agy --yolo` | Inicia o Antigravity CLI com aprovação automática de permissões. |

---

## 5. Rastreabilidade de Arquivos do Sistema

* **Repositório Git do Projeto:** [`/home/francisco/Documentos/Projetos/GaiaOS`](file:///home/francisco/Documentos/Projetos/GaiaOS)
* **Regras Netfilter Raw:** [`/etc/gaiaos/security/gaia-cve-2026-98432-shield.nft`](file:///etc/gaiaos/security/gaia-cve-2026-98432-shield.nft)
* **Configuração Sysctl de Segurança:** [`/etc/sysctl.d/98-gaia-security-cve-2026-98432.conf`](file:///etc/sysctl.d/98-gaia-security-cve-2026-98432.conf)
* **Ativação Persistente MGLRU:** [`/etc/tmpfiles.d/gaia-mglru.conf`](file:///etc/tmpfiles.d/gaia-mglru.conf)
* **Políticas de Desempenho do Chrome:** [`/etc/opt/chrome/policies/managed/gaia-performance.json`](file:///etc/opt/chrome/policies/managed/gaia-performance.json)
* **Daemon Guardian (Código Fonte C):** [`/home/francisco/Documentos/Projetos/GaiaOS/guardian/src/gaia-guardian.c`](file:///home/francisco/Documentos/Projetos/GaiaOS/guardian/src/gaia-guardian.c)
* **Logs e Telemetria em Tempo Real:** [`/var/log/gaia-guardian/`](file:///var/log/gaia-guardian/)
* **Wrapper Universal agy:** [`/usr/local/bin/agy`](file:///usr/local/bin/agy)
