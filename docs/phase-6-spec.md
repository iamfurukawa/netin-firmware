# Netin — especificação da Fase 6: melhorias e estabilização

## Objetivo

Consolidar as Fases 1, 2, 3 e 5 em uma experiência confiável para uso diário.
Esta fase prioriza qualidade, segurança operacional, manutenção e clareza de
uso; não introduz uma nova família de recursos no firmware nem inclui OTA.

## Contexto

O firmware atual ocupa aproximadamente 87% da partição de aplicação da placa
ESP32 de 4 MB. Por isso, novos recursos permanentes no dispositivo devem ser
evitados nesta fase. Melhorias de produto que exigem interface rica devem ser
preferencialmente feitas na PWA e no servidor, preservando o protocolo e a UI
local já estáveis.

## Resultado esperado

Após a fase, uma pessoa consegue usar status, pareamento, grupos, interações e
mídia sem precisar interpretar estados internos do sistema. Falhas comuns têm
feedback acionável, os dados temporários são removidos com segurança e o
operador consegue diagnosticar entregas e conexões sem expor credenciais.

## Escopo

### PWA

- Reorganizar a navegação para que `Interações` seja a página inicial e separar
  `Mídia`, `Status`, `Grupos` e `Perfil`. Perfil concentra nome, cor,
  dispositivos, pareamento e preferências.
- Reordenar as fontes de mídia para privilegiar ações do celular: `Gravar
  vídeo`, `Buscar GIF` e, por último, `Arquivo`.
- Mostrar progresso real durante upload de mídia e um estado separado para
  "enviando", "processando" e "entregando".
- Exibir o resultado da entrega de mídia por dispositivo: pendente, confirmada
  ou falha normalizada, sem criar histórico social navegável.
- Melhorar mensagens de erro para permissão de câmera, vídeo incompatível,
  upload acima do limite, grupo sem destinatários e falha temporária de rede.
- Preservar rascunhos de mensagem e a seleção de grupo durante uma falha de
  rede, quando isso puder ser feito sem armazenar conteúdo sensível além da
  sessão atual.
- Melhorar acessibilidade: foco visível, rótulos, contraste, navegação por
  teclado e comportamento responsivo no celular.

### Catálogo global de reações e painel administrativo

- Criar um catálogo global, compartilhado por todos os grupos. Reações não são
  configuradas por grupo nem por pessoa nesta fase.
- Administradores podem criar, editar, ordenar, ativar ou desativar reações pela
  PWA. Cada reação possui identificador estável, nome, asset processado e
  versão de catálogo.
- O asset pode ser imagem ou GIF e passa pelo mesmo processamento de mídia para
  a tela do Netin. O servidor é a fonte de verdade e mantém os arquivos em
  armazenamento privado.
- Dispositivos sincronizam o catálogo e guardam os assets ativos no SD em
  `/netin/reactions/`. Quando uma reação recebida ainda não estiver no cache, o
  dispositivo a baixa pelo canal autenticado antes de exibir.
- Se SD ou asset estiverem indisponíveis, a placa usa um fallback visual simples
  e continua confirmando a interação normalmente.
- A área administrativa da PWA reúne grupos e membros, catálogo de reações,
  uso de mídia, entregas pendentes/falhas e execução auditável do expurgo.

### Backend e operação

- Criar tarefa periódica idempotente para remover variantes de mídia e entregas
  expiradas, respeitando os sete dias atuais e preservando apenas metadados
  operacionais mínimos.
- Explicitar estados de entrega de mídia e disponibilizá-los à PWA do remetente.
- Manter o processamento de vídeo síncrono enquanto o volume for baixo, com
  limite de tempo e logs. Uma fila persistente de processamento só será
  planejada se métricas demonstrarem necessidade.
- Adicionar logs estruturados e métricas simples para autenticação, pareamento,
  MQTT, downloads de mídia, confirmações e falhas normalizadas.
- Aplicar limites de taxa para login, geração/uso de código de pareamento,
  uploads e criação de eventos sociais, sem alterar a regra de que cutucadas
  não possuem limite funcional de produto.
- Documentar backup do PostgreSQL, restauração, rotação de segredos e operação
  do Mosquitto Dynamic Security.
- Modelar catálogo de reações, suas versões e assets; publicar atualização de
  catálogo para dispositivos conectados e servir download autenticado para
  sincronização posterior.

### Firmware

- Remover o seletor de status da placa. A PWA passa a ser a única superfície de
  alteração de status; o firmware exibe o estado sincronizado e continua útil
  como painel passivo.
- Dar precedência total a conteúdo recebido: mídia, reação e cutucada ocupam a
  tela sem cabeçalhos, rodapés ou controles decorativos. O remetente, quando
  necessário, aparece de forma discreta.
- Fechar automaticamente mídia, GIF, vídeo convertido, reação ou cutucada após
  três minutos. Um toque fecha antes do prazo. Ao fechar, abre a próxima entrega
  pendente; sem fila, volta para a Home com o status atual.
- Substituir emojis renderizados por assets de reação armazenados no SD, com
  fallback enxuto para cartão indisponível.
- Corrigir defeitos de apresentação, consumo de RAM, vazamentos de recursos e
  reconexão encontrados durante testes, sem aumentar dependências pesadas.
- Medir flash e RAM em toda release. Alterações que façam a imagem ultrapassar
  90% da partição exigem justificativa e plano de redução antes de merge.
- Melhorar somente diagnósticos já existentes no serial ou na tela de
  Dispositivo; nunca mostrar tokens, senha Wi-Fi, URL assinada ou dados privados.

### Qualidade e testes

- Criar testes automatizados para autorização de download de mídia, limpeza de
  expiração, revogação/despareamento e mudanças de grupo com entregas pendentes.
- Cobrir no PWA as transições de upload, gravação de câmera, erro e reenvio.
- Manter um roteiro manual reproduzível para Wi-Fi, portal, pareamento, MQTT,
  SD ausente, cartão cheio, JPEG, GIF, vídeo curto e dispositivo offline.
- Executar uma janela de observação em produção antes de considerar a fase
  encerrada, registrando falhas reais e sua causa.

## Fora do escopo

- OTA, tabela de partições com dois slots e rollback; continuam na
  [Fase 4](phase-4-spec.md), atualmente adiada.
- Amigos, convites, bloqueios, histórico de conversas, status agendado e Não
  Perturbe por horário.
- Galeria de mídia, favoritos, áudio, buzzer, streaming de câmera e vídeo
  decodificado diretamente no ESP32.
- Criptografia ponta a ponta de mídia.
- Reescrita do firmware ou troca do hardware atual.

## Ordem recomendada

1. Reorganizar a PWA em áreas de Interações, Mídia, Status, Grupos e Perfil.
2. Criar o catálogo global administrável de reações e sua sincronização no SD.
3. Simplificar a UI do firmware, remover o seletor de status e implementar
   fechamento automático/avanço da fila após três minutos.
4. Expor estado de entrega de mídia e progresso de upload na PWA.
5. Implementar limpeza de mídia expirada, painel administrativo e testes do
   servidor.
6. Adicionar rate limits, observabilidade e runbooks de operação.
7. Rodar testes de resiliência e corrigir falhas de firmware sem criar novos
   recursos.
8. Avaliar métricas e encerrar a fase com uma versão estável.

## Critérios de aceite

- A PWA informa claramente se uma mídia está enviando, processando, pendente,
  confirmada ou falhou para cada dispositivo destinatário.
- Um administrador consegue manter o catálogo global de reações pela PWA, e
  uma reação ativa é exibida pela placa usando asset no SD ou fallback seguro.
- A placa não oferece seletor de status; fecha conteúdo recebido após três
  minutos e abre a próxima entrega ou retorna ao status atual.
- Mídias e entregas expiradas não se acumulam indefinidamente no armazenamento
  privado ou PostgreSQL.
- Remover/desparear um dispositivo e remover alguém de um grupo impede novas
  entregas e elimina pendências autorizadas de forma verificável.
- Operador consegue diagnosticar uma falha por logs e métricas sem acesso a
  credenciais ou conteúdo de mídia.
- Os roteiros manuais de rede, MQTT, SD e mídia passam sem travar a placa.
- O firmware permanece abaixo de 90% da partição de aplicação.
