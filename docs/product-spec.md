# Netin — especificação inicial de produto

## Visão

Netin é um pequeno dispositivo social de mesa/bolso: mostra o estado atual de uma pessoa e permite interações rápidas entre amigos ou grupos. O dispositivo funciona como superfície de consulta e resposta imediata; o celular complementa tarefas que pedem texto, mídia, administração ou precisão de toque.

## Princípios

- Interações devem caber em poucos toques.
- O estado principal precisa ser compreensível à primeira vista.
- O dispositivo deve continuar útil quando estiver sem conexão.
- Privacidade e controle de quem pode interagir vêm antes de descoberta pública.
- O touch da placa não é a interface adequada para fluxos longos ou de alta precisão.

## Superfícies do produto

### Dispositivo Netin

Foco em informação passiva e ações curtas:

- Ver status próprio, conectividade e últimas interações.
- Mudar entre estados pré-definidos.
- Responder com reação, ação rápida ou mensagem curta.
- Receber cutucadas, GIFs e alertas.
- Escolher favoritos e usar ações simples de grupo.

### Web mobile / PWA

Foco em tarefas mais ricas. Uma PWA permite começar sem desenvolver e manter aplicativos nativos.

- Parear e configurar Wi-Fi do dispositivo.
- Editar perfil, status padrão, texto personalizado e preferências.
- Adicionar/remover amigos, aceitar convites, bloquear e administrar grupos.
- Escrever mensagens maiores e consultar histórico completo.
- Enviar foto, GIF ou vídeo curto; o servidor redimensiona/transcodifica para o formato aceito pelo Netin.
- Configurar Não Perturbe, privacidade, notificações e atualizações OTA.
- Mostrar diagnóstico do dispositivo: versão, bateria quando existir, sinal e última conexão.

### Backend

Fonte de verdade para identidade, relacionamentos e eventos.

- API autenticada para a PWA.
- Canal persistente MQTT sobre WebSocket seguro para os dispositivos.
- Fila de eventos, confirmação de entrega e sincronização após períodos offline.
- Processamento de mídia: validação, transcodificação, limite de tamanho e cache.

## Funcionalidades principais

### 1. Presença e status

O usuário escolhe um status padrão, por exemplo: disponível, ocupado, focado, ausente, em chamada ou invisível. Cada estado tem cor, ícone, texto curto e opcionalmente uma expiração.

- Alteração local pelo touch.
- Status personalizado curto, como “em reunião até 15h”.
- Agendamento/expiração: o estado volta para “disponível” após um período.
- Exibição de conectividade e última sincronização.

### 2. Perfil e contatos

Cada dispositivo tem um perfil simples: nome, avatar/ícone, cor e identificador.

- Adição de amigos por código/QR ou convite aprovado.
- Lista de favoritos para acesso rápido.
- Grupos privados, como família, time ou turma.
- Bloquear/remover contato.

### 3. Interações rápidas

O usuário pode enviar uma reação sem escrever texto.

- Emoji/reação.
- Atualização de status para amigos ou grupo.
- Mensagem curta pré-definida ou personalizada.
- Broadcast opcional para um grupo.
- Confirmação visual de envio, entrega e leitura quando aplicável.

### 4. Cutucar

Equivalente ao “nudge” do MSN: uma interação direcionada que chama atenção de um contato.

- Animação curta, vibração/buzzer se o hardware permitir e destaque na tela.
- Limite de frequência por remetente para evitar abuso.
- Ação rápida para silenciar ou responder com emoji/status.

### 5. GIFs e animações

Animações devem ser leves e previsíveis no ESP32.

- Primeiro suporte a GIFs pequenos, pré-redimensionados e com paleta limitada.
- Armazenamento local em LittleFS/SD e cache do último conteúdo recebido.
- Limite explícito de resolução, duração e tamanho do arquivo.
- Futuramente: pacotes temáticos de animações e reações.

### 6. Câmera em baixa resolução — fase futura

Streaming só deve entrar depois que a comunicação básica estiver robusta.

- Foto sob demanda antes de vídeo contínuo.
- Preview JPEG com resolução e taxa de quadros baixas.
- Consentimento explícito em cada sessão e indicador visível de câmera ativa.
- Avaliar hardware com câmera e PSRAM; a placa atual não deve ser tratada como alvo garantido dessa função.

## Mídia enviada pelo celular

O envio de mídia deve começar na PWA, não na placa.

1. Usuário escolhe uma foto, GIF ou vídeo curto no celular.
2. A PWA envia o original ao backend.
3. O backend gera uma versão compatível com o dispositivo: resolução pequena, paleta/formato suportado, duração e tamanho limitados.
4. O Netin baixa ou recebe a versão processada, salva no cache e a exibe.

Para a primeira versão, o melhor recorte é **foto e GIF pequeno**. Vídeo deve ser tratado como uma sequência curta de frames ou GIF otimizado; streaming contínuo fica para uma etapa posterior.

## Recursos complementares recomendados

- **Modo Não Perturbe:** silencia cutucadas e alertas em horários definidos.
- **Histórico curto:** últimas interações recebidas/enviadas, com expiração local.
- **Sincronização offline:** fila local e reenvio quando o Wi‑Fi voltar.
- **Provisionamento Wi‑Fi:** tela ou portal temporário para configurar rede sem recompilar firmware.
- **Atualização OTA:** essencial antes de distribuir dispositivos a outras pessoas.
- **Indicadores de saúde:** Wi‑Fi, bateria (quando houver), última sincronização e versão do firmware.
- **Preferências de privacidade:** quem vê status, quem pode enviar mensagens/cutucadas e participação em grupos.
- **Segurança básica:** identidade de dispositivo, tokens revogáveis e comunicação TLS para o backend.

## Fluxos mínimos

### Alterar status

1. Usuário toca no cartão de status.
2. Escolhe um estado ou texto curto.
3. Netin atualiza a tela imediatamente.
4. O evento entra na fila local e sincroniza com o servidor.

### Enviar interação

1. Usuário abre favoritos ou grupo.
2. Escolhe reação, mensagem curta ou cutucada.
3. Confirma o destinatário quando a ação for direcionada.
4. O dispositivo mostra o resultado: pendente, enviado ou falhou.

### Receber interação

1. Dispositivo recebe o evento.
2. Mostra animação/aviso conforme preferências do usuário.
3. Salva no histórico curto.
4. Permite responder ou silenciar.

## Roadmap sugerido

Detalhamento das entregas: [Fase 1](phase-1-spec.md), [Fase 2](phase-2-spec.md)
[Fase 3](phase-3-spec.md) e [Fase 4 — OTA](phase-4-spec.md).

### Fase 1 — fundação local

- Tela estável, touch, LED e configurações de hardware.
- Estados locais e alteração de status.
- Persistência local de configurações.

### Fase 2 — conexão e identidade

- Configuração de Wi‑Fi.
- Registro do dispositivo e perfil mínimo, sem avatar.
- PWA de configuração, perfil e pareamento.
- Backend com autenticação e MQTT sobre WebSocket para eventos de status.
- Sincronização apenas do próprio status entre a placa e a PWA.
- Não inclui amigos, contatos, convites, grupos, mensagens, reações ou broadcast.

### Fase 3 — social

- Grupos com inscrição imediata, administração global e silenciamento geral.
- Reações, mensagem curta e cutucada para grupo ou pessoa que compartilhe grupo.
- Não inclui contatos, convites, histórico de interações nem limite de frequência para cutucadas.
- Renovação deslizante de sessão: ao usar a PWA perto do vencimento, a API emite
  um novo cookie/sessão e revoga o anterior. Incluir limpeza periódica de sessões
  vencidas no PostgreSQL, limite de sessões ativas por conta e opção de encerrar
  sessões em outros dispositivos.

### Fase 4 — distribuição

- OTA, diagnósticos e recuperação de rede.
- Privacidade, bloqueios e observabilidade do backend.
- Atualização remota com assinatura, rollback e liberação progressiva; ver
  [especificação de OTA](phase-4-spec.md).

### Fase 5 — mídia

- Foto e GIF enviados pela PWA, processamento no backend e cache local.
- Experimentos de streaming em hardware adequado.

## Decisões pendentes após o início da Fase 2

- O backend é próprio: Node.js/TypeScript, Fastify, PostgreSQL e Mosquitto na Raspberry Pi.
- As interações são privadas por padrão ou haverá descoberta pública?
- Os grupos serão administrados por convite, código temporário ou ambos?
- A configuração Wi‑Fi será feita por portal local no dispositivo; Bluetooth e serial não entram no fluxo principal.
- A placa atual terá bateria, buzzer ou outro mecanismo de alerta?
