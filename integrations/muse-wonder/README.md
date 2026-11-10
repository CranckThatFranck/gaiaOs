# Integração Muse Wonder 1.0 no Gaia OS

Este módulo integra o modelo de fronteira **Meta Muse Wonder 1.0**, ativado em modo Serverless via **Google Cloud Vertex AI** (projeto `negocio-de-bolso-492422`), diretamente ao fluxo de desenvolvimento local do usuário.

## Componentes

1. **Daemon Ponte Local (`gaia-muse-vertex-bridge.py`)**:
   - Escuta em `http://127.0.0.1:18787`.
   - Gerencia e renova dinamicamente tokens OAuth do GCP (`gcloud auth print-access-token`) com cache de 40 minutos.
   - Fornece catálogo de modelos Meta para a CLI oficial (`/muse-code/models`).
   - Repassa e transmite streaming SSE de `POST /v1/responses` diretamente para a Responses API do Vertex AI.
   - Fornece camada adaptadora OpenAI compatível (`POST /v1/chat/completions`) para ferramentas e extensões IDE.
   - Consumo de memória: ~10.8 MB RSS (limite máximo de 30 MB).

2. **Serviço Systemd (`gaia-muse-bridge.service`)**:
   - Inicia com o sistema em background sob o usuário `francisco`.

3. **CLI Meta Muse Code (`muse` & `muse-wonder`)**:
   - Localizados em `/usr/local/bin/muse` e `/usr/local/bin/muse-wonder`.
   - `muse-wonder` executa prompts com reasoning em nível `high`, espaço de trabalho confiável e flags otimizadas para taxa de requisição.

4. **Integração com VS Code**:
   - Extensão **Continue** (`continue.continue`) pré-configurada em `~/.continue/config.json`.
   - Perfil de Terminal Integrado `"Muse Wonder 1.0"` em `~/.config/Code/User/settings.json`.
   - Atalhos de teclado no VS Code: `Ctrl+L` para chat lateral com Muse Wonder 1.0, `Ctrl+I` para edição/geração inline no código.
