# Netin Firmware

Firmware inicial para a placa ESP32-2432S024, com display TFT 240×320 e touch resistivo.

## Estado atual

- Tela Netin nativa com TFT_eSPI, em retrato.
- Display estável com `ILI9341_2_DRIVER`.
- Compensação de cor RGB565 para esta revisão do painel.
- Touch calibrado e botão de atualização funcional.
- LED RGB onboard desligado durante a inicialização.

## Desenvolvimento

Com PlatformIO instalado:

```bash
pio run
pio run -t upload
pio device monitor
```

## Estrutura

- `src/main.cpp`: tela inicial, touch e LED.
- `lib/`: cópia local do TFT_eSPI com a configuração da placa.
- `components/TFT_eSPI`: atalho para a biblioteca local, usado pelo PlatformIO.

## Hardware validado

- ESP32-WROOM-32
- TFT ILI9341 240×320
- Touch resistivo
- LED RGB ativo em nível baixo: GPIO 4, 16 e 17
