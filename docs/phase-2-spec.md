# Netin — especificação da Fase 2

## Objetivo

Conectar o Netin à internet de forma configurável pelo celular, associá-lo a uma conta e sincronizar o status entre a placa e uma aplicação web mobile (PWA). A placa continua funcional sem rede: a alteração de status acontece localmente e é sincronizada assim que houver conexão.

Esta fase estabelece a base de identidade, pareamento, conectividade e eventos necessária para as funcionalidades sociais posteriores. Ela não tenta colocar no touch da placa fluxos que são melhores no celular.

## Resultado esperado

Uma pessoa consegue:

1. Abrir a PWA no celular e criar ou acessar sua conta.
2. Colocar um Netin novo em modo de configuração.
3. Informar a rede Wi-Fi e parear a placa com sua conta.
4. Definir nome e status padrão no celular.
5. Alterar o status tanto pela placa quanto pela PWA.
6. Ver a mesma presença refletida nas duas superfícies após a sincronização.
7. Continuar mudando o status na placa quando estiver offline, com indicação de pendência e sincronização posterior.

## Escopo

### Conta e perfil mínimo

- A PWA permite criar conta, entrar e encerrar sessão com e-mail e senha.
- O perfil possui `id`, nome de exibição e cor opcional; não há avatar nesta fase.
- O nome tem de 1 a 24 caracteres visíveis; a PWA valida e o firmware recebe uma versão já sanitizada.
- Não existe descoberta pública de usuários nesta fase.
- Uma conta pode ter mais de um dispositivo, mas cada dispositivo pertence a apenas uma conta por vez.

### Provisionamento Wi-Fi

- Um dispositivo sem credenciais Wi-Fi inicia em **modo de configuração**.
- O modo de configuração é iniciado também por uma ação explícita em Ajustes; deve haver confirmação para evitar apagar a conexão por toque acidental.
- O provisionamento é feito exclusivamente por portal Wi-Fi local; BLE não entra nesta fase.
- A placa expõe uma rede temporária protegida, por exemplo `Netin-7K3P`, e um portal em `http://192.168.4.1`. A senha temporária possui oito dígitos numéricos e é exibida na tela. QR fica como melhoria futura; a URL cativa/direta é suficiente nesta fase.
- O portal lista redes visíveis, permite selecionar/informar SSID e senha e mostra o resultado da tentativa de conexão.
- O usuário conecta o celular temporariamente à rede do Netin. A abertura automática do portal é desejável, mas a URL direta deve funcionar quando o sistema operacional não a oferecer.
- As credenciais ficam somente na placa, em armazenamento não exposto pela PWA nem enviado ao backend.
- A placa testa a conexão antes de encerrar o portal. Em caso de falha, explica o erro e permite tentar novamente.
- A sessão do portal expira em até 10 minutos e encerra após uma conexão bem-sucedida. Ela só pode ser iniciada por ação local na placa.

#### Redes salvas e retentativa

- O dispositivo mantém até cinco perfis de rede, cada um com SSID, credencial, data/contador do último sucesso e prioridade de uso.
- Uma rede só é adicionada ou atualizada depois que a conexão for validada; inserir senha incorreta não altera os perfis já salvos.
- No boot e após perder a rede, o firmware tenta primeiro o último perfil conectado com sucesso. Se ele não estiver visível ou falhar, faz uma varredura e tenta os demais perfis salvos visíveis, do mais recentemente bem-sucedido para o menos recente.
- As tentativas ocorrem em segundo plano, com espera progressiva, sem bloquear touch ou renderização. O portal não abre automaticamente apenas porque uma rede está indisponível.
- Ao adicionar uma sexta rede, o perfil com sucesso mais antigo é removido. A interface do portal deve informar essa substituição antes da confirmação final.
- Ajustes oferece as ações `Configurar Wi-Fi` e `Esquecer redes`; esta última exige confirmação e não remove o vínculo da conta.

### Pareamento dispositivo–conta

- Após estar conectada, a placa mostra um código de pareamento temporário e um identificador curto do dispositivo.
- Na PWA autenticada, a pessoa informa ou escaneia esse código e confirma o pareamento.
- O código é de uso único, expira em até 10 minutos e não revela credenciais de Wi-Fi nem um token permanente.
- Após a confirmação, o backend entrega ao dispositivo uma credencial revogável, específica daquele dispositivo.
- O dispositivo exibe confirmação de pareamento e passa a mostrar o nome do perfil.
- Remover um dispositivo na PWA revoga sua credencial. A placa volta a estado não pareado, preservando a configuração de Wi-Fi até que o usuário a remova localmente.

### Status sincronizado

Os estados da Fase 1 continuam disponíveis e são a única forma de presença nesta entrega. Texto personalizado, expiração e agendamento ficam para uma fase futura.

- Mudar status na placa atualiza a tela imediatamente e persiste o valor local.
- Se conectada, a placa envia a alteração ao backend.
- Mudar status na PWA atualiza o backend e o dispositivo conectado recebe a alteração.
- A placa mostra uma indicação discreta de conexão e uma indicação de status pendente quando houver alteração ainda não confirmada.
- A Fase 2 não exige mostrar “hora da última alteração”.

#### Regra de conflito

Cada alteração tem `eventId`, `deviceId`, `createdAt` e versão monotônica por dispositivo. O backend atribui uma versão global ao aceitar um evento.

- Enquanto o dispositivo está conectado, o primeiro evento aceito torna-se o status atual.
- Para eventos criados offline, a reconexão envia a fila na ordem local de criação.
- Se uma alteração remota já foi aceita depois da última sincronização local, vence o evento aceito por último no backend. A placa recebe o estado resultante e o persiste.
- A UI não deve prometer sucesso antes da confirmação: usa estados visuais `sincronizado`, `pendente` e `erro`.

### Fila offline

- A placa mantém uma fila persistente de alterações de status ainda não confirmadas.
- A fila suporta no mínimo 20 eventos sem sobrescrever eventos não entregues.
- O firmware tenta reenvio com espera progressiva, sem usar `delay()` nem bloquear touch/renderização.
- Um `PUBACK` do MQTT confirma apenas que o broker recebeu a publicação. O item só é removido da fila após receber um `ack` do backend contendo o mesmo `eventId`, confirmando que a alteração foi aplicada.
- Eventos confirmados pelo backend são removidos da fila.
- Se a fila estiver cheia, a UI exibe erro recuperável; o estado local atual permanece utilizável.
- Reiniciar ou cortar energia não pode descartar eventos ainda pendentes.

### PWA

A PWA é a superfície para configuração e gestão, não uma cópia completa da UI da placa.

- Implementação: React, TypeScript e Vite, com layout mobile-first, manifest e service worker.
- É hospedada como arquivos estáticos pelo proxy web da Raspberry em `https://glados.13997906387.xyz`; a API Fastify é servida separadamente em `https://glados-server.13997906387.xyz`.
- Chamadas à API usam CORS com credenciais somente entre esses dois subdomínios conhecidos.
- A sessão de autenticação usa cookie `HttpOnly`, `Secure` e `SameSite`, em vez de token armazenado em `localStorage`.
- Nesta fase, a sessão tem validade fixa de 30 dias. Ao expirar, a API responde
  `401`, limpa o cookie quando ele ainda estiver presente e a PWA retorna ao
  login. Renovação deslizante de sessão fica para a Fase 3.
- A PWA apresenta estados explícitos de dispositivo: conectado, desconectado e aguardando a primeira conexão. A pendência de uma alteração criada offline é indicada pela própria placa, que é a única superfície que conhece a fila local antes da reconexão.
- O estado online é derivado do último heartbeat MQTT recebido: até 90 segundos é
  conectado; depois disso é desconectado. Antes do primeiro heartbeat, a PWA
  informa que o dispositivo ainda aguarda conexão.
- O portal em `192.168.4.1` não é parte da PWA: é uma página local e temporária hospedada pela placa, usada somente para configurar Wi-Fi. Após a placa entrar na internet, o usuário retorna à PWA para parear a conta.

Telas mínimas:

| Tela | Responsabilidade |
| --- | --- |
| Autenticação | entrar/criar acesso e encerrar sessão. |
| Meus dispositivos | listar dispositivos, conexão, nome e ação de remover. |
| Adicionar dispositivo | explicar o modo de configuração e vincular código/QR. |
| Perfil | editar nome e cor opcional. |
| Status | consultar e alterar o status sincronizado. |

- A PWA deve funcionar bem em tela pequena e informar erros de rede, código expirado e dispositivo offline.
- A PWA não recebe, guarda ou exibe a senha Wi-Fi do usuário.
- Upload de foto, vídeo e GIF não entra nesta fase.

### Interface na placa

A Home preserva o status abaixo do menu como ação principal, sem cabeçalho, rodapé ou indicador de rede. O estado de rede é apresentado textualmente em Ajustes.

O menu abre Ajustes com itens suficientes para a parte conectada:

- Tema claro/escuro (recurso existente).
- Rede: mostra o estado textual da conectividade e oferece `Configurar Wi-Fi`, `Tentar novamente` quando aplicável e `Esquecer redes`.
- Dispositivo: identificador curto e estado `não pareado`, `gerando código` ou `pareado`; abre a tela de código de pareamento quando aplicável.

As mensagens devem ser curtas, legíveis em 240×320 e não esconder as ações de voltar. A configuração detalhada de Wi-Fi e a edição de perfil ocorrem no celular.

## Comunicação e segurança

### Backend

O backend é a fonte de verdade para conta, perfil, vínculo de dispositivos e status sincronizado.

- A implementação inicial roda em uma Raspberry Pi e é composta por um serviço Node.js/TypeScript com Fastify, PostgreSQL e um broker Mosquitto.
- O Fastify expõe a API HTTPS consumida pela PWA: autenticação por e-mail e senha, perfil, dispositivos e pareamento. Ele também executa o consumidor que aplica os eventos recebidos por MQTT.
- PostgreSQL armazena contas, hashes de senha, dispositivos, vínculos, códigos de pareamento, estado atual e deduplicação de eventos. Mosquitto é responsável somente pelo transporte MQTT, não por regras de negócio.
- API HTTPS para PWA e provisionamento/pareamento.
- O dispositivo usa MQTT sobre WebSocket seguro: `wss://glados-mqtt.13997906387.xyz/mqtt`. Isso permite atravessar o Cloudflare Tunnel e o CGNAT pela porta HTTPS padrão, sem expor portas no roteador.
- O Cloudflare Tunnel recebe o hostname curinga e o encaminha ao Nginx; o Nginx faz o upgrade WebSocket e encaminha somente `glados-mqtt.13997906387.xyz` ao listener WebSocket interno do Mosquitto. A PWA usa HTTPS para suas operações e não se conecta ao broker diretamente.
- O serviço Node/Fastify conecta-se ao Mosquitto pela rede Docker interna.
- Eventos, comandos e confirmações usam MQTT QoS 1. QoS 1 pode entregar duplicatas, portanto o backend confirma eventos de status de forma idempotente por `eventId`.
- Enquanto estiver conectado, o firmware publica um heartbeat leve a cada 60
  segundos. O backend usa apenas esse sinal para atualizar a disponibilidade do
  dispositivo; ele não altera o status da pessoa.
- Mensagens usam versão de protocolo explícita e payloads pequenos em JSON ou formato binário documentado.
- O estado atual é publicado como mensagem MQTT retida para que o dispositivo receba a versão vigente ao reconectar.
- Tópicos iniciais: `netin/v1/devices/{deviceId}/events`, `commands`, `ack` e `state`.
- Reconexões e mensagens repetidas não podem duplicar alterações nem corromper a fila local.

### Regras mínimas de segurança

- Toda comunicação fora da rede local usa TLS com validação de certificado. Para o dispositivo, o certificado público apresentado pelo Cloudflare para `glados-mqtt.13997906387.xyz` é validado contra a CA incluída no firmware.
- Senha Wi-Fi, tokens e segredos não aparecem em logs, tela, QR nem respostas da PWA.
- A credencial do dispositivo é revogável e tem escopo apenas daquele dispositivo.
- Códigos de pareamento são aleatórios, de uso único e expiram.
- A API exige autenticação para consultar ou alterar perfil e dispositivos.
- Telemetria é opcional e deve excluir conteúdo de credenciais.

## Dados persistidos na placa

Além das preferências existentes da Fase 1, o firmware precisa versionar estes dados. Nomes finais de chaves podem variar, mas o esquema e a migração precisam ser documentados.

| Dado | Finalidade |
| --- | --- |
| esquema de armazenamento | migrar dados com segurança. |
| até cinco perfis Wi-Fi | SSID, credencial, prioridade e último sucesso para reconexão automática. |
| identificador do dispositivo | identificar a placa durante pareamento e API. |
| credencial revogável do dispositivo | autenticar o canal com backend. |
| perfil mínimo em cache | nome e cor para uso local básico. |
| último status confirmado | restaurar a presença conhecida. |
| fila de eventos pendentes | reenviar alterações offline. |
| configuração do backend | ambiente/URL e versão de protocolo, preferencialmente compilados ou provisionados com segurança. |

Falha de leitura, esquema incompatível ou dado inválido não pode travar o boot. O firmware deve restaurar um estado seguro e expor uma ação de recuperação apropriada, sem apagar Wi-Fi ou identidade silenciosamente.

## Preparação para OTA da Fase 4

OTA não é entregue nesta fase, mas a Fase 2 deve preservar essa possibilidade:

- validar uma tabela de partições com dois slots OTA compatíveis com o tamanho do
  firmware, NVS e `otadata`;
- persistir a versão do firmware e a variante/revisão de hardware no modelo local
  e enviá-las no futuro heartbeat do dispositivo;
- preparar a validação TLS, tempo confiável e contrato versionado por dispositivo;
- fazer a CI gerar e reter o artefato `.bin` associado ao commit/versão.

O download, assinatura, escrita da partição inativa, rollback e interface de
atualização pertencem à [Fase 4](phase-4-spec.md).

## Arquitetura proposta

O código da Fase 1 continua como base e recebe módulos de conectividade sem misturar rede com desenho de tela.

| Módulo | Responsabilidade |
| --- | --- |
| `src/app` | tipos de domínio: status, perfil, conectividade e eventos. |
| `src/display` | renderização e ícones; não realiza chamadas de rede. |
| `src/input` | touch e eventos de interação. |
| `src/storage` | NVS, fila persistente e migração de esquema. |
| `src/ui` | telas, estados visuais e encaminhamento de ações. |
| `src/network` | Wi-Fi, reconexão, relógio/tempo seguro e estado de conectividade. |
| `src/provisioning` | access point temporário, portal Wi-Fi local e pareamento. |
| `src/sync` | protocolo, autenticação do dispositivo, fila e reconciliação de status. |

Dependências permitidas:

```text
ui → app, display, input, storage, network, sync
sync → app, storage, network
provisioning → network, storage
display/input → sem dependência de rede
```

### Repositórios

As superfícies são mantidas em repositórios independentes, clonados lado a lado durante o desenvolvimento:

| Repositório | Responsabilidade |
| --- | --- |
| `netin-firmware` | firmware ESP32, portal local e cliente MQTT. |
| `netin-server` | API Fastify, consumidor MQTT, migrações PostgreSQL e infraestrutura do backend. |
| `netin-web` | PWA para autenticação, perfil, pareamento e status. |

Os contratos HTTP e MQTT devem ser versionados e compartilhados entre os repositórios por OpenAPI/JSON Schema e documentação de tópicos; não há dependência de imports diretos entre eles.

## Fora do escopo

- Amigos, convites, bloqueios, descoberta e grupos.
- Mensagens curtas, reações, broadcast e histórico.
- Cutucar/chamar atenção.
- GIFs, fotos, vídeos, upload de mídia e cache de mídia.
- Streaming ou captura de câmera.
- OTA e gerenciamento remoto de firmware.
- Renovação deslizante e limpeza automática de sessões expiradas.
- Status personalizado, expiração, agendamento e Não Perturbe agendado.
- Bateria, buzzer ou outras mudanças de hardware.

## Decisões necessárias antes de implementar

- [x] Backend inicial: Node.js/TypeScript com Fastify, PostgreSQL e Mosquitto na Raspberry Pi.
- [x] Portal Wi-Fi cativo com fallback em `192.168.4.1`; QR foi adiado.
- [x] Endpoints públicos: `glados.13997906387.xyz` (PWA), `glados-server.13997906387.xyz` (API) e `glados-mqtt.13997906387.xyz` (MQTT/WSS). Cloudflare Tunnel termina TLS; o firmware valida a CA do certificado público.
- [x] Fila persistente definida: até 20 eventos, FIFO, reenvio a cada 5 segundos e remoção apenas após `ack` idempotente do backend. Quando cheia, o status local continua utilizável e a tela informa o erro recuperável.
- [x] Validado Wi-Fi, portal, NTP, HTTPS de pareamento, MQTT/WSS e TLS: firmware ocupa aproximadamente 82,3% da partição padrão de aplicação. OTA permanece fora desta entrega.

### Infraestrutura já validada

- Raspberry Pi atrás de CGNAT com Cloudflare Tunnel apontando o curinga de domínio ao Nginx na rede Docker `nginxnet`.
- Nginx roteia `glados-server.13997906387.xyz` para `netin-server:3000`, `glados.13997906387.xyz` para `netin-web:80` e `glados-mqtt.13997906387.xyz` para `mosquitto:9001` com upgrade WebSocket e HTTP/1.1. O nome Docker do container do broker é `netin-mosquitto`, mas o nome de serviço resolvido na rede é `mosquitto`.
- API e PWA possuem runners GitHub Actions ARM64 próprios na Raspberry; somente pushes na `main` executam deploy. O upload físico do firmware permanece manual.
- API e PWA publicadas com autenticação por e-mail/senha, sessão por cookie,
  edição de perfil, listagem/remoção de dispositivos, presença por heartbeat,
  pareamento por código e consulta/alteração de status. O firmware MQTT/WSS usa
  a credencial revogável do dispositivo e mantém fila offline persistente.

> Os nomes públicos são planos de propósito. O certificado Universal SSL e o curinga `*.13997906387.xyz` não cobrem subdomínios com dois níveis, como `netin.server.13997906387.xyz`.

## Estado de implementação e próximos passos

Esta seção é o acompanhamento técnico da Fase 2. O escopo e os critérios de
aceite continuam sendo as seções anteriores; itens concluídos aqui não substituem
os testes de aceite finais.

### Concluído

- [x] Firmware da Fase 1 estável: status local, touch, persistência e Ajustes.
- [x] Infraestrutura pública na Raspberry: Cloudflare Tunnel, Nginx, PostgreSQL,
  Mosquitto com autenticação/ACL e rede Docker `nginxnet`.
- [x] Repositórios `netin-server` e `netin-web` com CI e deploy por runners
  GitHub Actions ARM64; firmware permanece com upload físico manual.
- [x] API inicial publicada com `GET /health` e PWA inicial publicada, que
  verifica a conexão com a API.
- [x] Autenticação de e-mail/senha, perfil de sessão, dispositivos e pareamento por código na API/PWA.
- [x] Portal Wi-Fi protegido, expiração do portal, confirmação para esquecer redes e até cinco perfis priorizados pelo último sucesso, com retentativa progressiva.
- [x] Identidade persistente, registro HTTPS validado por CA, NTP, código temporário de pareamento e credencial revogável persistida no firmware.

### Pendente — sincronização do backend

- [x] Health check de deploy executado dentro do container `netin-server`.
- [x] Modelar status atual, eventos, deduplicação e contratos MQTT versionados no backend.
- [x] Configurar Dynamic Security/ACL do Mosquitto com credenciais revogáveis por dispositivo.
- [x] Conectar o servidor ao broker e aplicar eventos MQTT de modo idempotente.

### Pendente — PWA

- [x] Implementar perfil sem avatar no cadastro e edição posterior de nome e cor.
- [x] Implementar consulta e alteração de status no backend e PWA.
- [x] Adicionar manifest e service worker para o shell da aplicação.
- [x] Mostrar estado real aproximado do dispositivo por heartbeat MQTT (conectado, desconectado ou aguardando conexão).

### Pendente — firmware conectado

- [x] Informar no portal, antes da confirmação, quando uma sexta rede substituir a mais antiga.
- [x] Implementar MQTT/WSS com TLS, credencial revogável por dispositivo,
  tópicos versionados e detecção de revogação.
- [x] Implementar fila persistente, `eventId`, confirmação do backend e
  reconciliação de conflitos/offline.

### Validação de encerramento — concluída

- [x] Exercitar Wi-Fi inválido, redes salvas, reboot e retentativa.
- [x] Exercitar pareamento, revogação e remoção de dispositivo.
- [x] Confirmar status bidirecional entre PWA e placa conectada.
- [x] Exercitar API/broker indisponíveis, fila cheia, duplicatas e conflitos.
- [x] Medir flash/RAM com Wi-Fi, portal, TLS e MQTT.

## Plano de implementação

1. Criar contrato de API/protocolo e um backend de desenvolvimento autenticado.
2. Implementar estado de conectividade, ícone Wi-Fi na Home e tela de Rede sem remover a operação local.
3. Implementar access point e portal de Wi-Fi, com até cinco perfis persistentes e retentativa; testar falhas de senha, redes ausentes, troca de local e reinício.
4. Implementar registro e código de pareamento temporário.
5. Criar PWA com autenticação, lista de dispositivos, pareamento e edição de perfil/status.
6. Implementar canal autenticado, fila persistente e sincronização idempotente de status.
7. Exercitar cenários offline, conflito, revogação e recuperação antes de expandir para recursos sociais.

## Critérios de aceite

- Uma placa nova pode ser conectada ao Wi-Fi e pareada a uma conta usando somente celular e touch da placa.
- Credenciais Wi-Fi não aparecem fora da placa e o código de pareamento não pode ser reutilizado após expirar ou vincular o dispositivo.
- O dispositivo reconecta automaticamente a uma das redes salvas disponíveis, inclusive após reiniciar ou sair e voltar ao alcance de uma rede conhecida.
- Senha inválida, perfil sem sinal e remoção do sexto perfil recebem feedback recuperável no portal, sem apagar uma rede válida silenciosamente.
- O status alterado na placa aparece na PWA, e o alterado na PWA aparece na placa conectada.
- Sem internet, a placa continua permitindo mudar status e indica claramente que a atualização está pendente.
- Eventos pendentes sobrevivem a reinicialização e são sincronizados uma única vez após a reconexão.
- Remover um dispositivo na PWA revoga o acesso dele ao backend.
- Falha de Wi-Fi, backend indisponível, token inválido e fila cheia geram feedback recuperável, sem travar a interface local.
- A UI local continua estável e responsiva enquanto Wi-Fi e sincronização ocorrem em segundo plano.
