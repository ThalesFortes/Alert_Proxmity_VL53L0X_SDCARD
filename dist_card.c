#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "leds.h"
#include "vl53l0x.h"

// OLED SSD1306
#include "inc/ssd1306.h"
#include "inc/ssd1306_fonts.h"

// FatFs para SD
#include "ff.h"

// I2C
#define I2C_PORT i2c0
#define I2C_SDA  0
#define I2C_SCL  1

// SPI para SD card
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

// Botões
#define BUTTON_PIN_HIST 5  // botão para mostrar histórico
#define BUTTON_PIN_STOP 6  // botão para parar histórico

// Limiar de alerta: <= 100 mm (10 cm)
#define ALERT_THRESH_MM 100

// Distância inválida (maior que o sensor suporta)
#define DISTANCIA_INVALIDA 2001

FATFS fs;  // estrutura do FatFs


void inicializar_botao_hist() {
    gpio_init(BUTTON_PIN_HIST);
    gpio_set_dir(BUTTON_PIN_HIST, false); 
    gpio_pull_up(BUTTON_PIN_HIST);        
}

void inicializar_botao_stop() {
    gpio_init(BUTTON_PIN_STOP);
    gpio_set_dir(BUTTON_PIN_STOP, false); 
    gpio_pull_up(BUTTON_PIN_STOP);        
}


void inicializar_sd() {
    spi_init(SPI_PORT, 1000 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    FRESULT fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        printf("Erro ao montar SD: %d\n", fr);
    } else {
        printf("Cartão SD montado com sucesso.\n");
    }
}


void registrar_distancia(uint16_t distancia_mm, const char* estado, uint64_t tempo_ms) {
    FIL arquivo;
    char linha[100], valor_str[16], unidade[4];

    if (distancia_mm >= 1000 && distancia_mm < DISTANCIA_INVALIDA) {
        snprintf(valor_str, sizeof(valor_str), "%.2f", distancia_mm / 1000.0f);
        strcpy(unidade, "m");
    } else if (distancia_mm >= DISTANCIA_INVALIDA) {
        strcpy(valor_str, "ERRO");
        strcpy(unidade, "");
    } else {
        snprintf(valor_str, sizeof(valor_str), "%d", distancia_mm);
        strcpy(unidade, "mm");
    }

    unsigned long minutos = tempo_ms / 60000;
    unsigned long segundos = (tempo_ms / 1000) % 60;

    FRESULT fr = f_open(&arquivo, "distancia.txt", FA_OPEN_APPEND | FA_WRITE);
    if (fr == FR_OK) {
        snprintf(linha, sizeof(linha), "[%02lu:%02lu] Distancia: %s %s - Estado: %s\n",
                 minutos, segundos, valor_str, unidade, estado);
        UINT bytes_escritos;
        f_write(&arquivo, linha, strlen(linha), &bytes_escritos);
        f_close(&arquivo);
    } else {
        printf("Erro ao abrir arquivo: %d\n", fr);
    }
}


void mostrar_arquivo_sd() {
    FIL arquivo;
    FRESULT fr = f_open(&arquivo, "distancia.txt", FA_READ);
    if (fr != FR_OK) {
        printf("Erro ao abrir arquivo para leitura: %d\n", fr);
        ssd1306_Fill(Black);
        ssd1306_SetCursor(0, 0);
        ssd1306_WriteString("Erro lendo SD", Font_7x10, White);
        ssd1306_UpdateScreen();
        return;
    }

    char linha[128];
    char buffer[6][128]; 
    int count = 0;

    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();

    while (f_gets(linha, sizeof(linha), &arquivo)) {
        
        if (!gpio_get(BUTTON_PIN_STOP)) break;

        printf("%s", linha); 

        // Move buffer para cima
        if (count < 6) {
            strcpy(buffer[count++], linha);
        } else {
            for (int i = 0; i < 5; i++) strcpy(buffer[i], buffer[i + 1]);
            strcpy(buffer[5], linha);
        }

        // Atualiza OLED
        ssd1306_Fill(Black);
        for (int i = 0; i < count; i++) {
            ssd1306_SetCursor(0, i * 10);
            ssd1306_WriteString(buffer[i], Font_7x10, White);
        }
        ssd1306_UpdateScreen();
        sleep_ms(500); // velocidade de scroll
    }

    f_close(&arquivo);
}


static void draw_oled_header(void) {
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("Monitor Proximidade", Font_7x10, White);
}


int main() {
    stdio_init_all();

    // I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // OLED
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();

    // LEDs
    leds_init();

    // Botões
    inicializar_botao_hist();
    inicializar_botao_stop(); 

    // SD Card
    inicializar_sd();

    // VL53L0X
    if (!vl53l0x_init(I2C_PORT)) {
        draw_oled_header();
        ssd1306_SetCursor(0, 16);
        ssd1306_WriteString("VL53L0X falhou!", Font_7x10, White);
        ssd1306_UpdateScreen();
        led_red();
        while (1) tight_loop_contents();
    }
    vl53l0x_start_continuous(I2C_PORT);

    char line[32];
    uint16_t mm = 0;

    #define FILTER_SIZE 5
    uint16_t buffer[FILTER_SIZE] = {0};
    int idx = 0;
    bool filled = false;

    bool botao_ativo = false;

    while (1) {
        if (!gpio_get(BUTTON_PIN_HIST)) { 
            if (!botao_ativo) {
                botao_ativo = true;
                mostrar_arquivo_sd(); 
            }
        } else {
            botao_ativo = false;
        }

        // --- Leitura do sensor ---
        bool ok = vl53l0x_read_range_mm(I2C_PORT, &mm, 200); 

        if (!ok) {
            draw_oled_header();
            ssd1306_SetCursor(0, 16);
            ssd1306_WriteString("Leitura: timeout", Font_7x10, White);
            ssd1306_UpdateScreen();
            printf("Leitura: timeout\n");
            led_blue();
            sleep_ms(150);
            leds_off();
            sleep_ms(150);
            continue;
        }

        if (mm < 50) mm = 3000; 

        // --- Filtro de média ---
        buffer[idx++] = mm;
        if (idx >= FILTER_SIZE) { idx = 0; filled = true; }

        uint32_t soma = 0;
        int count_filter = filled ? FILTER_SIZE : idx;
        for (int i = 0; i < count_filter; i++) soma += buffer[i];
        uint16_t media = soma / count_filter;

        // --- Atualiza OLED ---
        draw_oled_header();
        const char* estado = "INDEFINIDO";

        if (media > 2000) {
            leds_off();
            ssd1306_SetCursor(0, 16);
            ssd1306_WriteString("Fora de alcance", Font_7x10, White);
            estado = "FORA";
        } else if (media <= ALERT_THRESH_MM) {
            led_red();
            ssd1306_SetCursor(0, 16);
            ssd1306_WriteString("ALERTA: <=10cm", Font_7x10, White);
            estado = "ALERTA";
        } else if (media <= 300) {
            led_green();
            ssd1306_SetCursor(0, 16);
            ssd1306_WriteString("Objeto medio", Font_7x10, White);
            estado = "MEDIO";
        } else {
            led_blue();
            ssd1306_SetCursor(0, 16);
            ssd1306_WriteString("Objeto longe", Font_7x10, White);
            estado = "LONGE";
        }

        if (media <= 2000) {
            ssd1306_SetCursor(0, 28);
            snprintf(line, sizeof(line), "Dist: %4u mm", media);
            ssd1306_WriteString(line, Font_7x10, White);
        }

        ssd1306_UpdateScreen();

       
        uint64_t tempo_ms = to_ms_since_boot(get_absolute_time());
        registrar_distancia(media, estado, tempo_ms);

   
        printf("[%llu ms] %s - %u mm\n", tempo_ms, estado, media);

        sleep_ms(200);
    }

    return 0;
}
