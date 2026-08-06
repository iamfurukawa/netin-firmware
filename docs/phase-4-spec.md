# Netin — especificação da Fase 4: OTA e operação remota

## Objetivo

Permitir atualizar o firmware dos dispositivos Netin remotamente, de modo seguro,
controlado e recuperável. Uma atualização nunca deve transformar uma placa
funcional em uma placa inacessível.

## Resultado esperado

Uma pessoa autenticada consegue publicar uma versão de firmware, disponibilizá-la
para dispositivos elegíveis e acompanhar se cada dispositivo instalou, adiou ou
falhou. O dispositivo baixa a atualização apenas por HTTPS, valida sua origem,
mantém a versão anterior para rollback e informa o resultado quando reconectar.

## Escopo da Fase 4

### Publicação e elegibilidade

- O backend mantém releases com versão semântica, data, notas, alvo de hardware,
  URL HTTPS do artefato, tamanho, SHA-256 e assinatura.
- Apenas uma interface administrativa autenticada pode publicar ou liberar uma
  release. A PWA comum pode consultar e solicitar atualização apenas dos próprios
  dispositivos.
- O backend determina se uma versão é aplicável pela variante de hardware, versão
  mínima, canal de release (`stable` inicialmente) e capacidade de flash.
- A liberação suporta ondas: teste interno, grupo pequeno e liberação geral. Uma
  release pode ser pausada ou retirada sem apagar a versão já instalada.

### Fluxo no dispositivo

1. O dispositivo recebe por MQTT apenas um aviso versionado de atualização; ele
   não recebe o binário pelo broker.
2. Ele busca o manifesto por HTTPS autenticado e valida versão, alvo, tamanho,
   hash e assinatura antes de escrever qualquer dado na flash.
3. A UI apresenta versão, tamanho, resumo curto e ações `Atualizar agora` e
   `Depois`. Atualização automática só poderá existir após experiência real com o
   fluxo manual.
4. Após confirmação, o dispositivo grava a imagem na partição OTA inativa e
   mostra progresso, erro recuperável e orientação para manter alimentação.
5. Depois de reiniciar, o firmware novo executa uma auto-verificação e confirma
   sucesso ao bootloader. Sem essa confirmação, ocorre rollback para a imagem
   anterior.
6. O dispositivo publica o resultado (`download_failed`, `validation_failed`,
   `install_failed`, `rolled_back` ou `installed`) e a versão em execução.

Não haverá atualização enquanto o dispositivo estiver em portal de Wi-Fi,
despareado, com conectividade instável ou executando operação que não possa ser
interrompida. Nesta placa não há medição de bateria; até existir esse hardware, a
UI deve orientar o uso de alimentação USB estável.

### Segurança

- Download somente por HTTPS com validação da cadeia de certificado e horário
  confiável.
- A imagem tem assinatura assimétrica. A chave pública é compilada no firmware;
  a chave privada de release fica fora do repositório e da Raspberry de produção
  sempre que possível.
- SHA-256 protege a integridade do download, mas não substitui a assinatura.
- O dispositivo rejeita downgrade, salvo uma política explícita de recuperação
  administrada pelo backend.
- Credenciais de Wi-Fi, tokens e material de assinatura nunca aparecem em logs,
  mensagens MQTT ou tela.
- URLs de artefatos devem expirar ou exigir autorização de dispositivo quando a
  distribuição deixar de ser interna.

### Partições e recuperação

- A tabela de partições contém `factory`, `ota_0`, `ota_1`, `otadata` e área
  suficiente para NVS e aplicação. Cada slot OTA deve comportar a imagem de
  produção com margem definida durante o build.
- O firmware usa rollback do bootloader ESP-IDF: uma imagem nova só é marcada
  válida depois da auto-verificação de display, armazenamento e conectividade
  mínima.
- Falha de download ou validação preserva a imagem em execução.
- Falha repetida de boot faz rollback. O dispositivo exibe a falha quando a UI
  estiver disponível e a reporta depois.
- Atualização por cabo continua sendo rota de recuperação para desenvolvimento;
  recuperação física e factory reset serão documentados antes da liberação geral.

### Observabilidade

- O dispositivo envia `firmwareVersion`, variante de hardware e resultado da
  última tentativa em seu heartbeat/estado de conexão.
- O backend armazena o histórico por dispositivo, com timestamps de início,
  término e erro normalizado, sem dados sensíveis.
- O painel administrativo mostra adoção por release e permite pausar novas
  instalações se a taxa de falha passar de um limite definido.

## Contratos a definir

- Formato do manifesto, por exemplo: `version`, `hardwareTarget`, `size`,
  `sha256`, `signature`, `url`, `releasedAt` e notas curtas.
- Tópicos MQTT para aviso e resultado de OTA.
- Algoritmo/formato de assinatura e processo de guarda/rotação da chave.
- Política de versão mínima, bloqueio de downgrade e rollout em ondas.
- Limites de tamanho da imagem e tabela final de partições para ESP32-WROOM-32.
- Critérios da auto-verificação que confirma a imagem nova ao bootloader.

## Preparação obrigatória na Fase 2

Estes itens não entregam OTA ao usuário, mas devem ser feitos agora para não
inviabilizar a Fase 4:

- Medir o tamanho do firmware de produção e reservar uma tabela de partições com
  dois slots OTA viáveis, `otadata`, NVS e margem de crescimento. Validar build e
  upload por cabo usando essa tabela.
- Definir e persistir `hardwareTarget`/revisão da placa e `firmwareVersion` no
  firmware. Incluí-los no modelo de dispositivo e no heartbeat futuro do backend.
- Manter a URL do backend configurável por ambiente e estabelecer HTTPS com
  validação de certificado e tempo confiável antes de qualquer download remoto.
- Versionar os contratos MQTT/HTTP e manter identificação/credencial revogável
  por dispositivo; OTA não deve usar uma credencial compartilhada.
- Planejar armazenamento persistente para o resultado da última tentativa de
  atualização, mesmo que a UI e o download ainda não existam.
- Configurar CI para gerar o artefato `.bin`, registrar versão/commit e guardar o
  artefato de cada release. Publicação, assinatura e distribuição ficam para a
  Fase 4.

## Fora do escopo

- Atualização automática em massa sem confirmação do usuário.
- Delta updates, que aumentam muito a complexidade de recuperação.
- Atualização de bootloader, partição ou certificados raiz por OTA na primeira
  entrega.
- Suporte a múltiplas variantes de hardware além da placa atual.

## Critérios de aceite

- Uma versão inválida, alterada ou destinada a outro hardware é rejeitada antes
  de sobrescrever o slot OTA inativo.
- Interromper energia durante download ou instalação mantém ou restaura uma
  imagem inicializável.
- Uma imagem que falha na auto-verificação volta automaticamente à versão
  anterior.
- O backend consegue identificar versão instalada e resultado de uma tentativa
  por dispositivo.
- Um operador consegue pausar uma release sem afetar dispositivos que já estão
  em versão estável.
