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

Os estados, rótulos, cores e ícones estão definidos em `src/app/app_types.*` e `src/display/netin_display.cpp`. Não há assets externos, GIFs ou fontes customizadas.

### Tela principal

- Não há cabeçalho, rodapé nem texto auxiliar “toque para mudar”.
- O status começa abaixo do botão de menu e mostra ícone e nome do status atual.
- Tocar no status abre o seletor de status.
- Um botão flutuante de 48×48 px com ícone de menu fica no canto superior esquerdo e abre Ajustes.
- Não há indicador de rede: nesta fase o dispositivo é integralmente local.

### Seletor de status

- Lista vertical com cartões de 48 px de altura e intervalo de 8 px entre eles.
- Exibe quatro itens por vez, com os demais acessíveis pelos botões `^` e `v` na lateral direita.
- O estado atual é marcado por um ponto, sem o texto `OK`.
- Um botão flutuante de voltar com ícone fica no canto superior esquerdo e retorna à Home.
- `^` mostra o grupo anterior; `v` mostra o próximo grupo.
- Somente cartões desenhados dentro da área visível recebem toque.

### Alteração de status

1. Tocar em uma opção atualiza e persiste o novo estado imediatamente.
2. Voltar retorna à Home sem alterar o estado atual.

### Ajustes

A tela de Ajustes é acessada apenas pelo menu da Home.

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
| `src/app/app_types.*` | Estados, temas, validação e rótulos. |
| `src/display/netin_display.*` | Inicialização do TFT, compensação de cor e desenho de ícones/controles. |
| `src/input/touch_input.*` | Leitura calibrada, debounce e emissão de eventos. |
| `src/storage/settings_store.*` | Carregamento e gravação de `Preferences`. |
| `src/ui/ui.*` | Máquina de estados das telas, paginação e roteamento de toques. |

### Máquina de estados

```text
BOOT → HOME

HOME
  ├─ status abaixo do menu → STATUS_PICKER
  └─ menu → SETTINGS

STATUS_PICKER
  ├─ ^ / v → página anterior / próxima
  ├─ cartão visível → salvar → HOME
  └─ voltar → HOME

SETTINGS
  ├─ Tema → alternar e salvar → SETTINGS
  └─ Voltar → HOME
```

### Touch

- Um `Tap` exige deslocamento menor que 12 px, duração máxima de 700 ms e lockout de 180 ms.
- O leitor guarda a última coordenada válida antes de soltar, pois o controlador não garante coordenadas úteis no evento de liberação.
- Eventos de swipe ainda são reconhecidos pelo módulo de touch, mas a UI usa apenas taps.

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

## Status de encerramento — concluída

A Fase 1 está concluída como fundação local do produto. As validações de tela,
touch, persistência, tema e LED foram consideradas aceitas durante a evolução
das fases posteriores. Os itens abaixo ficam como melhorias opcionais, não como
condição para reabrir a fase.

### Implementação

- [ ] Exibir erro/retry caso `Preferences::save()` falhe. Hoje a UI atualiza mesmo se a gravação NVS falhar.
- [ ] Remover os eventos de swipe não usados ou reutilizá-los no futuro; a interface atual usa só `Tap`.
- [ ] Decidir se a alteração imediata de status precisa de proteção adicional contra toque acidental.
- [ ] Definir se os quatro estados extras são definitivos ou apenas massa de teste do paginador.
- [ ] Opcional: criar tela de diagnóstico deliberadamente acessível, conforme previsto na versão inicial da spec.

### Validação na placa

- [ ] Trocar cada um dos nove estados e validar cor/ícone/texto.
- [ ] Testar `^` e `v` no primeiro, intermediário e último grupo; botões inativos não podem selecionar itens.
- [ ] Reiniciar e cortar/alimentar a placa após mudar status e tema para confirmar persistência NVS.
- [ ] Executar teste de estabilidade de 30 minutos na Home, sem ruído, deslocamento ou reinício.
- [ ] Testar limites de toque entre cartões, menu, voltar, `^` e `v`.
- [ ] Confirmar que o LED RGB permanece apagado após cinco reinicializações.

## Critérios de aceite

- O painel permanece visualmente estável por 30 minutos.
- Os nove estados podem ser alcançados, selecionados e exibidos com a cor correta.
- Status e tema sobrevivem a reinicialização e corte de energia.
- Controles desabilitados e áreas vazias não acionam navegação nem seleção.
- O menu abre Ajustes e o tema pode ser alterado e restaurado.
- O LED RGB fica apagado após iniciar.
