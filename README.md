# 📏 Monitor de Proximidade com VL53L0X + SD + OLED + LEDs

Este projeto implementa um **sistema embarcado** utilizando o **Raspberry Pi Pico W**, que realiza:

- Leitura contínua de distância com o **sensor de proximidade VL53L0X** via **I²C**.
- Exibição em tempo real da distância no **display OLED SSD1306**.
- Registro das medições em **cartão SD**.
- Indicação visual de proximidade através de **LEDs**.
- Consulta do histórico de leituras através de **botão físico**, com scroll no OLED.
- Interrupção da exibição do histórico via **segundo botão físico**.

---

## ⚙️ Funcionalidades

1. **Leitura de Proximidade (VL53L0X)**
   - Captura valores em milímetros (mm).
   - Filtragem por média móvel para reduzir flutuações.

2. **Sistema de LEDs**
   - **LED Vermelho** → Objeto muito próximo (alerta ≤10 cm).  
   - **LED Verde** → Objeto a distância média (50–30 cm).  
   - **LED Azul** → Objeto longe (>30 cm) ou fora do alcance.  

3. **Display OLED SSD1306**
   - Exibe:
     - Estado atual do sensor (ALERTA, MEDIO, LONGE, FORA).
     - Distância média detectada.
   - Ao consultar histórico:
     - Rolagem de linhas do arquivo `distancia.txt`.
     - Possibilidade de interromper a rolagem com botão de parada.

4. **Cartão SD**
   - Salva todas as medições em `distancia.txt`.
   - Formato de registro:
     ```
     [mm:ss] Distancia: XXX mm - Estado: ALERTA
     ```

5. **Botões Físicos**
   - **Botão 1 (GPIO 5)** → Mostra o histórico do SD no OLED e no console.
   - **Botão 2 (GPIO 6)** → Interrompe a exibição do histórico a qualquer momento.

---

## 🖥️ Hardware Utilizado

- **Raspberry Pi Pico W**  
- **Sensor de proximidade VL53L0X**  
- **OLED SSD1306 (I²C, 128x64)**  
- **LEDs RGB** (ou individuais)  
- **Cartão SD com módulo SPI**  
- Botões

---

## 📌 Ligações

| Componente     | Pino Pico W |
|----------------|-------------|
| I²C SDA        | GP0         |
| I²C SCL        | GP1         |
| SPI MISO SD    | GP16        |
| SPI MOSI SD    | GP19        |
| SPI SCK SD     | GP18        |
| SPI CS SD      | GP17        |
| LED Verde      | GP11        |
| LED Azul       | GP12        |
| LED Vermelho   | GP13        |
| Botão Histórico| GP5         |
| Botão Parada   | GP6         |

---

## 🛠️ Como Compilar e Executar

1. git clone https://github.com/ThalesFortes/Alert_Proxmity_VL53L0X_SDCARD.git
2. Configure o **SDK do Raspberry Pi Pico** e **CMake**.
3. Compile o projeto:
   ```bash
   mkdir build && cd build
   cmake ..
   make

Grave o .uf2 no Raspberry Pi Pico W.

Abra o terminal serial para visualizar os logs das medições.

## ⚠ Sistema de Alerta
Limiar de alerta: distância ≤ 100 mm (10 cm).

Se ultrapassado:

LED Vermelho acende.

OLED exibe mensagem de alerta.

Registro no SD indica o estado como ALERTA.

Mensagem impressa no console:

[tempo] ALERTA - XXX mm

## 📊 Exemplo de Saída no Serial
[12345 ms] LONGE - 1500 mm
[12500 ms] MEDIO - 250 mm
[12700 ms] ALERTA - 80 mm
Ao apertar Botão Histórico, as últimas medições são exibidas no OLED com scroll, e podem ser interrompidas pressionando Botão Parada.

## 🚀 Melhorias Futuras
Adicionar controle de alerta sonoro (buzzer).

Exportar medições para aplicativo via Wi-Fi (Pico W).

Implementar filtro digital avançado para suavizar leituras.

Permitir configuração do limiar de alerta via botão ou Wi-Fi.