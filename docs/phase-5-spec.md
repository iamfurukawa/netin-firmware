# Netin — especificação da Fase 5: mídia

## Objetivo

Permitir que uma pessoa envie uma foto, GIF ou vídeo curto sem áudio pela PWA
para uma pessoa específica ou para um grupo inscrito. O backend processa o
arquivo para a tela do Netin, o dispositivo baixa-o por HTTPS para o SD card e
inicia a exibição automaticamente ao recebê-lo.

Esta fase usa o canal autenticado, a fila MQTT e o pareamento da Fase 2, além das
regras de grupos e silenciamento da Fase 3. Não inclui reprodução de vídeo na
placa nem streaming de câmera.

## Resultado esperado

Uma pessoa autenticada consegue selecionar uma foto, GIF ou vídeo curto no
celular, escolher um grupo inscrito ou uma pessoa que compartilhe esse grupo e
enviar a mídia. O Netin destinatário:

1. recebe o evento por MQTT;
2. baixa a mídia processada por HTTPS com credencial de dispositivo;
3. grava o arquivo no SD card;
4. abre automaticamente a visualização;
5. confirma a entrega somente depois que o arquivo estiver íntegro e disponível
   para exibição.

Falhas de rede, arquivo inválido ou SD card indisponível não apagam a mídia que
já estava em cache e são reportadas ao backend.

## Decisões da fase

- O SD card é o armazenamento de cache de mídia. A flash interna permanece
  reservada para firmware, NVS e dados pequenos de configuração.
- A mídia é exibida automaticamente ao ser recebida. Um toque fecha a
  visualização e retorna ao painel.
- A PWA envia mídia tanto para um grupo quanto para uma pessoa em grupo em comum.
- Fotos, GIFs e vídeo curto sem áudio são formatos de entrada aceitos. A placa
  recebe apenas JPEG ou GIF otimizado; ela nunca decodifica vídeo diretamente.
- Não há histórico navegável na placa nesta fase; o cache atende à entrega e à
  visualização atual, não a uma galeria.

## Escopo

### Envio pela PWA

- A tela de interações permite escolher `Foto`, `GIF` ou `Vídeo curto`, selecionar
  um arquivo e definir o destino: grupo inscrito ou membro de um grupo em comum.
- A PWA valida tipo e tamanho antes do upload e mostra progresso, processamento,
  sucesso ou erro.
- Para destino de grupo, a regra de inscrição é a mesma de reações e mensagens.
- Para destino individual, remetente e destinatário devem continuar membros do
  grupo de contexto selecionado. Autoenvio não é permitido.
- O envio cria um evento de mídia persistente e não publica o arquivo pelo MQTT.

### Processamento no backend

- O backend recebe o original por HTTPS autenticado, limita tamanho de upload,
  tipo MIME e número de frames e armazena o original somente enquanto necessário
  para o processamento.
- Cada arquivo gera uma variante compatível com a placa, com metadados de
  integridade: formato, largura, altura, tamanho, SHA-256, duração e, para GIF,
  quantidade de frames.
- Fotos são convertidas para JPEG progressivo desabilitado, RGB, com proporção
  preservada e encaixe em `240 × 320`.
- GIFs são redimensionados para caber em `240 × 320`, com paleta limitada,
  duração limitada e taxa de quadros limitada. O backend pode convertê-los para
  um formato de animação próprio caso o decoder escolhido no firmware exija isso.
- Vídeos curtos são processados de forma assíncrona com FFmpeg: o áudio é
  descartado, a duração é limitada, e o vídeo é convertido para o mesmo GIF
  otimizado aceito pela placa. MP4, MOV e WebM são formatos de entrada iniciais,
  sujeitos à validação do conteúdo real do arquivo.
- O arquivo processado fica em armazenamento privado. A URL de download não é
  pública e deve expirar ou exigir autorização de dispositivo.
- O processamento é assíncrono: a API devolve aceitação do upload e só cria as
  entregas quando a variante estiver pronta. Falha de processamento fica visível
  à pessoa que enviou, sem criar entrega.

### Entrega e cache no dispositivo

- O servidor cria uma entrega por dispositivo destinatário, como nos eventos
  sociais, e publica um comando MQTT leve contendo `eventId`, metadados e URL de
  download. O payload MQTT nunca contém bytes de mídia.
- O firmware valida o contrato, inicia o download HTTPS autenticado e grava em
  arquivo temporário no SD card.
- Antes de promover o arquivo para o cache ativo, valida tamanho e SHA-256. A
  promoção é atômica: o cache anterior permanece caso o download falhe.
- O cache inicial contém uma mídia ativa por dispositivo. O arquivo anterior é
  removido somente após a nova mídia ser validada; política de galeria ou cache
  múltiplo fica fora desta fase.
- Se o SD card não estiver montado, cheio ou apresentar erro de escrita, o
  firmware publica `media_failed` com código normalizado e não confirma a
  entrega.
- Após download e validação, o firmware exibe a mídia automaticamente e publica
  `media_ack`. A confirmação significa “arquivo disponível para exibição”, não
  “pessoa viu a mídia”.
- Ao tocar na tela durante a exibição, a placa fecha a mídia e retorna à Home.
  Se outra mídia chegar enquanto uma está aberta, a nova entra como pendente e é
  aberta ao fechar a atual; ela não interrompe a imagem ou animação em curso.

### Interface da placa

- Enquanto baixa: tela simples com nome do remetente, tipo de mídia e progresso.
- Foto: escala com proporção preservada e fundo da cor do tema nas áreas vazias.
- GIF: reprodução em loop, com limite de uso de RAM; frames são lidos do SD card
  ou de buffer de tamanho limitado, nunca o GIF inteiro na RAM.
- Erro: mensagem curta (`SD indisponível`, `download falhou`, `arquivo inválido`)
  e botão `Voltar`.
- A Home não terá indicador permanente de mídia. A mídia recebida abre por si e
  a fila de pendências permanece responsabilidade do sincronizador.

## Limites iniciais

Os valores abaixo são ponto de partida e devem ser confirmados depois do primeiro
teste com a placa, decoder e cartão reais:

| Item | Limite inicial |
| --- | --- |
| Foto original recebida pela API | 10 MB |
| Foto processada | JPEG, até 240 × 320, 150 KB |
| GIF original recebido pela API | 10 MB |
| GIF processado | até 240 × 320, 2 MB |
| GIF | até 8 segundos, 12 fps, 96 frames |
| Vídeo original recebido pela API | 10 MB |
| Vídeo | até 8 segundos, sem áudio; convertido para GIF processado |
| Cache por dispositivo | uma mídia ativa + um temporário |
| Prazo de entrega | 7 dias, seguindo a política de eventos sociais |

O backend pode reduzir mais o arquivo para atender à largura de banda ou ao
desempenho do ESP32. O firmware não deve assumir que o cartão possui espaço
livre suficiente sem consultar o sistema de arquivos.

## Dados e contratos

### PostgreSQL

Tabelas ou extensões necessárias:

- `media_assets`: dono/remetente, tipo original e processado, dimensões,
  duração, tamanho, SHA-256, chave privada de armazenamento, estado de
  processamento, expiração e datas.
- `media_events`: `eventId`, remetente, `groupId` de contexto, `targetUserId`
  opcional, `assetId`, criação e expiração.
- `media_deliveries`: evento, dispositivo, tentativas de publicação/download,
  erro normalizado e confirmação.

As regras de limpeza removem originais após processamento, arquivos processados
após expiração e registros de entrega após o encerramento do evento, preservando
somente metadados operacionais mínimos se forem necessários para auditoria.

### MQTT

Comando enviado ao tópico já privado do dispositivo:

```json
{
  "protocolVersion": 1,
  "type": "media_event",
  "eventId": "uuid",
  "sender": { "name": "Nome" },
  "media": {
    "kind": "image/jpeg",
    "width": 240,
    "height": 320,
    "size": 123456,
    "sha256": "hex",
    "durationMs": null,
    "downloadUrl": "https://..."
  },
  "createdAt": "ISO-8601"
}
```

O dispositivo responde por seu tópico de eventos com `media_ack` ou
`media_failed`, sempre incluindo `eventId` e um código sem dados sensíveis.

### HTTP de download

- HTTPS obrigatório, com validação do certificado raiz já usada pelo firmware.
- A requisição deve identificar o dispositivo por credencial revogável ou URL
  assinada curta vinculada a ele.
- O backend autoriza cada download para o `deviceId` que recebeu a entrega.
- Redirecionamentos, MIME inesperado, tamanho acima do anunciado e hash inválido
  são rejeitados pelo firmware.

## Segurança e privacidade

- Arquivos não são públicos nem acessíveis por adivinhação de URL.
- O backend valida assinatura do arquivo, conteúdo real e limites antes do
  processamento; extensão e MIME declarado não são suficientes.
- Nomes de arquivo originais, credenciais e URLs de download não aparecem na
  tela ou logs da placa.
- Silenciamento global impede a criação de novas entregas de mídia para a pessoa,
  assim como já ocorre com as interações sociais.
- Remover alguém do grupo cancela entregas pendentes relacionadas àquele grupo.
- A exclusão de conta deve remover os arquivos privados vinculados, conforme a
  política de retenção a ser definida antes da abertura para outros usuários.

## Implementação proposta

1. Validar hardware: montar, listar, gravar e remover arquivo no SD card da
   placa; definir pinos, driver e formato de sistema de arquivos.
2. Escolher biblioteca de decode JPEG e GIF que opere por streaming a partir do
   SD card e medir RAM, tempo por frame e estabilidade da tela.
3. Criar armazenamento privado de objetos e pipeline de processamento no server
   (Sharp para imagem e FFmpeg para GIF e vídeo curto) compatível com a Raspberry.
4. Criar migrações, upload autenticado, autorização de destino e API de estado
   do processamento.
5. Integrar tela PWA de upload, seleção de destino e acompanhamento do envio.
6. Criar contrato MQTT, entregas persistentes e endpoint de download autorizado.
7. Implementar download temporário, SHA-256, cache atômico e tela de progresso no
   firmware.
8. Implementar visualização automática de JPEG e GIF, fila de mídia pendente e
   tratamento de falhas.
9. Testar com rede lenta, Wi-Fi interrompido, cartão ausente/cheio, arquivo
   corrompido, destino removido do grupo e dispositivo offline.

## Fora do escopo

- Áudio, chamadas e streaming contínuo de câmera.
- Reprodução ou streaming de vídeo diretamente na placa.
- Upload, gravação ou edição de mídia na placa.
- Galeria, histórico navegável e favoritos de mídia.
- Avatar de perfil e mídia pública.
- Criptografia ponta a ponta de arquivos.
- OTA e mudanças de partição por atualização remota; pertencem à Fase 4.

## Critérios de aceite

- Uma foto e um GIF enviados pela PWA chegam a um dispositivo individual ou a
  todos os dispositivos de membros de um grupo elegível.
- O arquivo processado respeita resolução, formato e limites definidos e é
  exibido automaticamente na placa.
- O ESP32 valida hash antes de substituir o cache ativo e usa o SD card sem
  precisar armazenar o arquivo inteiro em RAM ou flash interna.
- Um dispositivo offline recebe a mídia pendente ao reconectar, sem download ou
  exibição duplicados após `media_ack`.
- Sem SD card, com arquivo inválido ou em falha de rede, o firmware permanece
  utilizável, preserva o cache anterior e reporta erro ao servidor.
- Uma pessoa silenciada, removida do grupo ou despareada não recebe mídia nova.
