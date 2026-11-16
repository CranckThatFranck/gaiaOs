# Relatório Completo de Profiling de Hardware — Gaia OS

Este relatório apresenta o perfil técnico detalhado obtido diretamente do hardware para subsidiar a customização, otimização e compilação do **Gaia OS**.

---

## 1. CPU & ARQUITETURA DE SILÍCIO
* **Modelo da CPU:** Intel(R) Core(TM) i7 CPU       M 640  @ 2.80GHz
* **Arquitetura:** x86_64
* **Sufixo/Geração:** Intel Westmere Mobile (Arrandale 32nm)
* **Núcleos / Sockets:** 1 Socket | 2 Cores Físicos | 4 Threads (Tecnologia Hyper-Threading ativa)
* **Microarquitetura Detectada:** `-march=westmere` / `-mtune=westmere`
* **Instruções Vetoriais e Criptográficas Suportadas:**
  * **SSE4.2:** ✅ Sim
  * **AES-NI:** ✅ Sim
  * **AVX:** ❌ Não (Não suportado em CPUs Westmere)
  * **AVX2:** ❌ Não
  * **AVX512:** ❌ Não
  * **FMA:** ❌ Não
  * **SHA_NI:** ❌ Não
  * **BMI1 / BMI2:** ❌ Não
* **Topologia de Cache:**
  * **Cache L1d:** 64 KiB (2 instances)
  * **Cache L1i:** 64 KiB (2 instances)
  * **Cache L2:** 512 KiB (2 instances)
  * **Cache L3:** 4 MiB (1 instance) (Compartilhado)
* **Arquitetura Híbrida (P-Cores vs E-Cores):** Não (Arquitetura homogênea pura)
* **Driver de Escalonamento de Frequência:** `acpi-cpufreq` (Gerenciador clássico ACPI cpufreq em uso)

---

## 2. GPU, DISPLAY & ACELERAÇÃO GRÁFICA
* **GPU Detectada:** `00:02.0 VGA compatible controller [0300]: Intel Corporation Core Processor Integrated Graphics Controller [8086:0046] (rev 02)`
* **Driver de Vídeo Ativo:** `i915`
* **Suporte a Vulkan:** `Sem suporte (GPU Ironlake/GMA HD legado não suporta Vulkan API)`
* **Aceleração Gráfica por Hardware (VA-API):** `Suporte legado via i965-va-driver (VA-API H.264 parcial / MPEG2 total)`
  * *Observação:* A GPU Ironlake de 1ª geração Core possui recursos de decodificação parcial via VA-API (`i965-va-driver`). Não possui suporte a formatos modernos como VP9 ou AV1.

---

## 3. MEMÓRIA & ARMAZENAMENTO
* **Memória RAM Total:** `5,5Gi` (Instalada: ~6.0 GB)
* **Slots de Memória Ocupados (dmidecode):**
  * **Slot 0 (DIMM0):** 4 GiB N/A @ N/A
  * **Slot 1 (DIMM1):** 2 GiB DIMM @ 1067 MT/s
* **Dispositivos de Armazenamento:**
  * `sda    447,1G disk                   SSD 480GB`
  * `├─sda1     1M part`
  * `├─sda2     2G part ext4   /boot`
  * `└─sda3 445,1G part btrfs  /home`
  * `sdb        0B disk                   Multi-Card`
  * `sr0     1024M rom                    HL-DT-STDVDRAM GT30N`
  * `zram0    5,5G disk swap   [SWAP]`
* **Suporte a TRIM (SSD):** Sim (Suportado pelo SSD sda - discard_granularity = 512 bytes)
* **Configuração Atual do zRAM:** `Ativo - /dev/zram0 lzo-rle       5,5G  6,6M  3,8M  5,8M         [SWAP]`

---

## 4. REDE, ÁUDIO E PERIFÉRICOS
* **Placa Wi-Fi:** `02:00.0 Network controller: Qualcomm Atheros AR9485 Wireless Network Adapter (rev 01)` (Módulo do Kernel: `ath9k`)
* **Placa Ethernet:** `01:00.0 Ethernet controller: Broadcom Inc. and subsidiaries NetLink BCM57780 Gigabit Ethernet PCIe (rev 01)` (Módulo do Kernel: `tg3`)
* **Bluetooth:** `Nenhum adaptador Bluetooth USB detectado`
* **Servidor de Áudio:** `PipeWire (com pipewire-pulse compatibilidade)`
* **Modo de Boot do Firmware:** `BIOS Legado / CSM`
* **Status do Secure Boot:** `Não suportado (Sistema em modo BIOS Legado)`

---

## 5. SÍNTESE E RECOMENDAÇÃO DO GAIA OS

Com base no perfil detalhado da máquina host **Gateway NV55C** (Intel i7 M640 Westmere / 6GB RAM / SSD 480GB), apresentamos as especificações de compilação e otimização recomendadas para a distribuição customizada **Gaia OS**:

### A. Flags Recomendadas de Compilação do Kernel e Userland
* **MARCH_FLAG exato:** `-march=westmere`
* **MTUNE_FLAG exato:** `-mtune=westmere`
* **Flags Recomendadas:** `-O2 -pipe -march=westmere -mtune=westmere -fno-plt`
* *Raciocínio:* A microarquitetura Westmere é beneficiada por otimizações específicas para criptografia rápida AES-NI e operações vetorizadas SSE4.2. Usar `-march=westmere` em vez de genérico garante o uso dessas instruções e de `pclmulqdq`, o que otimiza significativamente o desempenho do kernel em redes e hashing sem incorrer no overhead de instruções não suportadas (como AVX).

### B. Módulos de GPU e Wi-Fi Embutidos no `initramfs`
Para garantir um boot rápido e "flicker-free" (KMS nativo desde o início do carregamento):
* **Módulos Críticos:** `i915 ath9k tg3`
* *Configuração do Dracut / Mkinitcpio:*
  * No Dracut: `force_drivers+=" i915 ath9k tg3 "`
  * No Mkinitcpio: `MODULES=(i915 ath9k tg3)`

### C. Dimensionamento e Otimização do zRAM
* **Algoritmo Recomendado:** `zstd` (Nível de compressão: `1`)
* **Tamanho Ideal:** `6GB (100% da RAM física)`
* *Raciocínio:*
  1. A máquina conta com 6GB de memória física, o que pode ser um gargalo em tarefas concorrentes.
  2. O algoritmo `zstd` oferece taxas de compressão significativamente superiores ao `lz4` ou `lzo-rle` clássico (frequentemente atingindo 3:1 ou superior em dados de RAM).
  3. Para evitar que o processador dual-core (4 threads) sofra com overhead durante a compressão de memória, recomenda-se configurar o **nível de compressão 1**. Isso garante velocidade semelhante ao `lz4` com taxas de compressão muito superiores, poupando a CPU antiga e maximizando o aproveitamento dos 6GB de RAM.
  4. Adicionalmente, deve ser mantida uma prioridade alta para o zRAM (`priority=100`) em conjunto com o swapfile secundário do SSD para transbordo seguro de memória.

---
*Relatório gerado automaticamente pelo Engenheiro de Kernel e Sistemas Operacionais da Gaia Works.*
