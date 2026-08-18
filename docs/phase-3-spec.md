# Netin — especificação da Fase 3

## Objetivo

Transformar o Netin pareado em um dispositivo social organizado por grupos: pessoas escolhem os grupos aos quais se inscrevem e enviam ou recebem interações nesses espaços. Nesta fase, a placa é uma superfície de recebimento; a PWA concentra escrita e administração.

Esta fase depende da conclusão do canal MQTT autenticado, da fila offline e da sincronização de status da Fase 2. Não começa a distribuir mídia nem atualizações OTA.

## Resultado esperado — concluído

Uma pessoa consegue:

1. Escolher grupos disponíveis para inscrição na PWA; criação de grupos é exclusiva de administradores.
2. Inscrever-se ou sair de um grupo.
3. Enviar uma reação ou mensagem curta para um grupo inscrito.
4. Receber a interação no Netin.
5. Silenciar globalmente as interações recebidas quando quiser ignorá-las.
6. Continuar recebendo eventos após reconectar, sem duplicá-los.

O escopo acima foi implementado. A cutucada também permite escolher uma pessoa
que esteja no mesmo grupo do remetente; a entrega é restrita aos dispositivos
pareados dessa pessoa.

## Escopo

### Grupos

- Grupos são criados e administrados somente na PWA por contas com permissão global de administrador.
- Uma pessoa pode escolher quais grupos disponíveis quer seguir. Inscrição e saída são ações da PWA; o silenciamento é uma preferência geral, também ajustada na PWA.
- Todos os grupos ativos aparecem em uma lista autenticada da PWA; não há busca pública por pessoas.
- A inscrição a partir da lista é imediata e não exige aprovação enquanto o grupo estiver com inscrições abertas.
- Administradores podem criar, renomear, arquivar/excluir grupos, remover membros e abrir/fechar novas inscrições.
- Usuários normais não criam nem editam grupos; podem apenas se inscrever, sair e interagir nos grupos em que estão inscritos.
- Não há limite funcional de membros por grupo nesta fase; limites operacionais só poderão ser introduzidos depois de medirmos a capacidade do backend.
- Fechar inscrições impede novas entradas, mas não remove nem silencia membros existentes.
- Um grupo tem membros `inscritos`.
- Sair de um grupo remove a inscrição.
- A placa não administra nem envia para grupos nesta fase; ela recebe eventos dos grupos aos quais a pessoa está inscrita.

### Interações

| Tipo | Destino | Criação | Conteúdo inicial |
| --- | --- | --- | --- |
| Reação | grupo inscrito | PWA | conjunto fixo de até 8 emojis/reações |
| Mensagem curta | grupo inscrito | PWA | texto de 1 a 160 caracteres |
| Cutucada | pessoa em grupo em comum ou grupo inscrito | PWA | evento sem texto, com alerta visual |

- Uma reação ou mensagem de grupo é expandida pelo backend para cada membro inscrito; o firmware nunca recebe a lista inteira de destinatários.
- Cada evento tem `eventId`, `senderUserId`, `groupId`, `targetUserId` opcional,
  `type`, `payload`, `createdAt` e `protocolVersion`. Para reações e mensagens,
  o destino é o grupo; para uma cutucada individual, `targetUserId` identifica a
  pessoa e `groupId` é o contexto de autorização compartilhado.
- A entrega é ao menos uma vez via MQTT QoS 1; backend e firmware deduplicam por `eventId`.
- “Entregue” significa que o dispositivo recebeu e confirmou o evento; confirmação de leitura fica fora desta fase.
- Para cada destinatário, o backend entrega o evento a todos os seus dispositivos pareados e ativos. Dispositivos despareados nunca recebem eventos.
- O backend pode limitar reação/mensagem por remetente e grupo para proteger o fan-out; os valores operacionais serão configuráveis, sem limitar o tamanho do grupo. Cutucadas não têm limite de frequência nesta fase.

### Cutucada

- Uma cutucada direcionada a uma pessoa só é permitida quando remetente e destinatário compartilham ao menos um grupo.
- Uma cutucada enviada a um grupo é entregue a todos os seus membros inscritos.
- Não há limite de frequência nesta fase.
- O destinatário pode silenciar globalmente as interações recebidas pela PWA.
- Na placa, uma cutucada gera destaque visual curto. Som/vibração só entra se o hardware for validado separadamente.

### Silenciamento e experiência da placa

- Não existe histórico de conversas ou interações nesta fase, nem na PWA nem na placa.
- O backend persiste eventos somente enquanto necessários para entrega: até confirmação de todos os dispositivos destinatários ou por no máximo 7 dias.
- A PWA oferece uma preferência geral para silenciar interações recebidas. Enquanto ativa, o backend não cria entregas para os dispositivos da pessoa e descarta o evento para ela.
- A placa não mantém histórico social, não responde e não inicia interações; ela apenas apresenta a interação entregue. Reações e cutucadas ocupam a tela e
  voltam ao painel com um toque; mensagens usam texto ampliado e botão `Voltar`.
- A Home pode indicar uma interação recebida de forma discreta. Não haverá indicador de Wi-Fi na Home.
- A tela de interação mostra remetente, tipo e conteúdo curto, com a ação de voltar.

### PWA

Telas mínimas:

| Tela | Responsabilidade |
| --- | --- |
| Explorar grupos | listar grupos ativos e permitir inscrição imediata. |
| Meus grupos | mostrar inscrições e saída. |
| Administração de grupos | área exclusiva de administradores para CRUD e gestão de membros. |
| Interações | enviar reação, mensagem curta ou cutucada. |
| Preferências sociais | silenciamento geral de interações. |

### Fora do escopo

- Busca pública por pessoas, feed público e recomendações de pessoas.
- Avatar e upload de mídia.
- Foto, GIF, vídeo, cache de mídia e streaming de câmera: pertencem à Fase 5.
- Mensagens longas, anexos, chamadas, áudio e criptografia ponta a ponta.
- Histórico de conversas/interações.
- Confirmação de leitura, presença de digitação e notificações push do navegador.
- OTA, administração remota de firmware e diagnóstico avançado.

## Dados e contratos

### PostgreSQL

Tabelas iniciais:

- `groups`: criador para auditoria, nome de 1 a 40 caracteres, estado ativo/arquivado, inscrições abertas/fechadas e datas.
- `group_members`: inscrição e datas.
- `social_events`: evento idempotente, remetente, destino, tipo, payload, criação e expiração operacional.
- `event_deliveries`: destinatário/dispositivo, estado de entrega, tentativas e confirmação.
- `social_preferences`: silenciamento geral e datas por usuário.
- `users.is_admin`: permissão global que controla acesso ao CRUD e à gestão de membros.

Índices e restrições devem impedir inscrição duplicada, auto-cutucada e acesso a grupos sem inscrição válida.

### MQTT

Os tópicos da Fase 2 são estendidos, mantendo o dispositivo inscrito apenas em seu próprio namespace:

```text
netin/v1/devices/{deviceId}/events
netin/v1/devices/{deviceId}/ack
netin/v1/devices/{deviceId}/state
```

O servidor valida a credencial revogável, autoriza o remetente, persiste o evento antes da publicação e remove a pendência somente após `ack` do dispositivo. O broker não decide permissões sociais.

## Segurança e retenção

- PWA exige sessão autenticada para qualquer operação social.
- O backend verifica a inscrição e a associação ao grupo antes de aceitar cada evento; também interrompe pendências de entrega quando uma inscrição é removida.
- Payloads são limitados e normalizados no backend; HTML não é aceito.
- Eventos e entregas expiram após confirmação de todos os destinatários ou em até 7 dias. Não há retenção para consulta posterior.

## Implementação concluída

1. Grupos, inscrições e administração foram criados no PostgreSQL, API e PWA.
2. Eventos sociais possuem fan-out persistente por dispositivo em `event_deliveries`.
3. O servidor publica pendências por MQTT QoS 1, repete a publicação em reconexão
   ou heartbeat e remove a pendência após `social_ack`.
4. O firmware deduplica `eventId` em NVS e apresenta reação, mensagem ou
   cutucada recebida.
5. A PWA permite silenciamento global, reações, mensagens, cutucada ao grupo e
   cutucada a um membro do mesmo grupo.
6. As migrações sociais `0007` a `0009` estão incluídas no servidor.

## Critérios de aceite — atendidos

- [x] Uma pessoa não inscrita não envia nem recebe eventos do grupo; remover um membro interrompe novas entregas.
- [x] Um grupo com inscrições fechadas não aceita novas inscrições, mas continua entregando eventos aos membros existentes.
- [x] Com silenciamento geral ativo, novas interações não são entregues à placa nem ficam disponíveis para consulta posterior.
- [x] Eventos duplicados não aparecem duas vezes na placa, por deduplicação de `eventId`.
- [x] Uma placa offline recebe as interações pendentes ao reconectar e confirma cada uma uma vez.
- [x] Cutucadas direcionadas e de grupo são suspensas pelo silenciamento geral.
- [x] A placa continua responsiva ao receber interações e exibe apenas o evento atual.

## Decisões posteriores (fora do encerramento da fase)

- Definir como a primeira conta administradora será provisionada e como esse papel será concedido/revogado.
- Escolher o conjunto inicial de reações e a representação visual de cada uma na placa.
- Definir o comportamento quando outra interação chegar enquanto a placa ainda estiver exibindo a atual; hoje a mais recente substitui a exibida.
- Definir a política de exclusão de conta e limpeza imediata de eventos pendentes.
