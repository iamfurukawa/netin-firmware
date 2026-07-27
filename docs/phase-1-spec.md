# Netin — especificação da Fase 1

## Objetivo

Entregar uma experiência local, estável e agradável para consultar e alterar o próprio status. A Fase 1 não depende de Wi‑Fi, backend, PWA, amigos ou mídia.

O dispositivo deve restaurar status e tema após reiniciar, responder de forma previsível ao touch e manter o LED RGB apagado.

## Escopo implementado

### Estados

| Estado | Cor | Ícone nativo |
| --- | --- | --- |
| Disponível | verde | círculo preenchido |
| Ocupado | vermelho | sinal de menos |
| Focado | roxo | alvo |
| Ausente | amarelo | relógio |
| Invisível | cinza | olho riscado |
| Em chamada | ciano | telefone |
| Jogando | azul | controle |
| Dormindo | verde-escuro | letras Z |
| Não perturbe | cinza-escuro | sinal de menos |

Os estados, rótulos, cores e ícones estão definidos em `app_types.*` e `netin_display.cpp`. Não há assets externos, GIFs ou fontes customizadas.

### Tela principal

- Não há cabeçalho, rodapé nem texto auxiliar “toque para mudar”.
- Um cartão grande ocupa a maior parte da tela e mostra ícone e nome do status atual.
- Tocar no cartão é a única ação para abrir o seletor de status.
- Um botão de 48×48 px com símbolo de engrenagem fica na parte inferior e abre Ajustes.
- Não há indicador de rede: nesta fase o dispositivo é integralmente local.

### Seletor de status

- Lista vertical com cartões de 48 px de altura e intervalo de 8 px entre eles.
- Exibe quatro itens por vez, com os demais acessíveis por paginação vertical.
- O estado atual é marcado por um ponto, sem o texto `OK`.
- O rodapé contém `Voltar`, `^` e `v`.
- `^` mostra o grupo anterior; `v` mostra o próximo grupo.
- Um botão de navegação indisponível não altera a lista nem seleciona um item oculto.
- Somente cartões desenhados dentro da área visível recebem toque.

### Confirmação

1. Tocar em um estado abre uma tela de confirmação.
2. `Aplicar` atualiza e persiste o novo estado.
3. `Voltar` retorna ao seletor sem alterar o estado atual.

### Ajustes

A tela de Ajustes é acessada apenas pela engrenagem da Home.

- Mostra o título `Ajustes`.
- Permite alternar entre tema escuro e claro.
- Oferece `Voltar` para retornar à Home.

## Persistência

Armazenamento local em `Preferences`/NVS, namespace `netin`.

| Chave | Tipo | Padrão |
| --- | --- | --- |
| `schema` | `UChar` | `1` |
| `status` | `UChar` | `Disponível` |
| `theme` | `UChar` | `Escuro` |
| `changes` | `UInt` | `0` |

Na inicialização, dados ausentes, esquema incompatível ou valores inválidos restauram os padrões sem travar o firmware. O contador é incrementado apenas quando um novo status diferente do atual é aplicado.

## Requisitos de hardware validados

```cpp
// lib/User_Setup.h
#define ILI9341_2_DRIVER
#define SPI_FREQUENCY 80000000

// Aplicação
uint16_t kTouchCalibration[] = {652, 2994, 423, 3361, 3};
constexpr uint16_t kTouchThreshold = 600;
constexpr uint8_t kRgbLedPins[] = {4, 16, 17};
```

- Interface em retrato, 240×320.
- `ILI9341_2_DRIVER` é obrigatório para estabilidade deste painel.
- O painel inverte RGB565 nessa sequência de inicialização; toda primitiva de desenho passa por `panelColor(~color)` no módulo `NetinDisplay`.
- O LED RGB é ativo em nível baixo; os três pinos são configurados em `HIGH` no boot.
- A biblioteca TFT permanece nativa; LVGL está fora da Fase 1.

## Arquitetura atual

| Módulo | Responsabilidade |
| --- | --- |
| `main.cpp` | Inicialização de LED, display, touch, NVS e ciclo principal. |
| `app_types.*` | Estados, temas, validação e rótulos. |
| `netin_display.*` | Inicialização do TFT, compensação de cor e desenho de ícones/controles. |
| `touch_input.*` | Leitura calibrada, debounce e emissão de eventos. |
| `settings_store.*` | Carregamento e gravação de `Preferences`. |
| `ui.*` | Máquina de estados das telas, paginação e roteamento de toques. |

### Máquina de estados

```text
BOOT → HOME

HOME
  ├─ cartão de status → STATUS_PICKER
  └─ engrenagem → SETTINGS

STATUS_PICKER
  ├─ ^ / v → página anterior / próxima
  ├─ cartão visível → STATUS_CONFIRM
  └─ Voltar → HOME

STATUS_CONFIRM
  ├─ Aplicar → salvar → HOME
  └─ Voltar → STATUS_PICKER

SETTINGS
  ├─ Tema → alternar e salvar → SETTINGS
  └─ Voltar → HOME
```

### Touch

- Um `Tap` exige deslocamento menor que 12 px, duração máxima de 700 ms e lockout de 180 ms.
- O leitor guarda a última coordenada válida antes de soltar, pois o controlador não garante coordenadas úteis no evento de liberação.
- Eventos de swipe ainda são reconhecidos pelo módulo de touch, mas a UI da Fase 1 não os usa; a lista é navegada exclusivamente pelos botões `^` e `v`.

### Renderização

- A Home e cada tela são redesenhadas somente após uma transição ou alteração de preferência; não há animação nem atualização periódica.
- `setTextWrap(false, false)` evita quebra acidental de texto.
- Não há `delay()` no fluxo normal.

## Fora do escopo

- Wi‑Fi, portal de configuração, backend e PWA.
- Amigos, grupos, mensagens, broadcast e cutucar.
- GIFs, fotos, vídeos, câmera e streaming.
- OTA e notificações push.
- Texto de contexto, status personalizado e expiração automática.
- Tela de diagnóstico exposta ao usuário.

## Pendências para encerrar a Fase 1

### Implementação

- [ ] Exibir erro/retry caso `Preferences::save()` falhe. Hoje a UI atualiza mesmo se a gravação NVS falhar.
- [ ] Remover os eventos de swipe não usados ou reutilizá-los no futuro; a interface atual usa só `Tap`.
- [ ] Decidir se a confirmação de status continua necessária após os testes de uso. Ela protege contra toque acidental, mas adiciona uma etapa.
- [ ] Definir se os quatro estados extras são definitivos ou apenas massa de teste do paginador.
- [ ] Opcional: criar tela de diagnóstico deliberadamente acessível, conforme previsto na versão inicial da spec.

### Validação na placa

- [ ] Trocar cada um dos nove estados e validar cor/ícone/texto.
- [ ] Testar `^` e `v` no primeiro, intermediário e último grupo; botões inativos não podem selecionar itens.
- [ ] Reiniciar e cortar/alimentar a placa após mudar status e tema para confirmar persistência NVS.
- [ ] Executar teste de estabilidade de 30 minutos na Home, sem ruído, deslocamento ou reinício.
- [ ] Testar limites de toque entre cartões, engrenagem, `Voltar`, `^` e `v`.
- [ ] Confirmar que o LED RGB permanece apagado após cinco reinicializações.

## Critérios de aceite

- O painel permanece visualmente estável por 30 minutos.
- Os nove estados podem ser alcançados, confirmados e exibidos com a cor correta.
- Status e tema sobrevivem a reinicialização e corte de energia.
- Controles desabilitados e áreas vazias não acionam navegação nem seleção.
- A engrenagem abre Ajustes e o tema pode ser alterado e restaurado.
- O LED RGB fica apagado após iniciar.
