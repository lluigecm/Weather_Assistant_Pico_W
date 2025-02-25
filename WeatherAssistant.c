#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/assets.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"  
#include "lwip/tcp.h"  
#include "lwip/apps/http_client.h"

// Definições do Display OLED
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15      // Definições I2C (DISPLAY OLED - SSD1306)
#define ADDRESS 0x3C

//Definições dos LEDs e Botões
#define RED_LED 13
#define GREEN_LED 11
#define BLUE_LED 12
#define BUTTON_A 5
#define BUTTON_B 6

ssd1306_t ssd; // Estrutura do display OLED
char buffer[1024]; // Buffer para armazenar os dados recebidos da API

bool setup(){
    gpio_init(RED_LED); // Inicializa o pino do LED vermelho
    gpio_init(GREEN_LED); // Inicializa o pino do LED verde
    gpio_init(BLUE_LED); // Inicializa o pino do LED azul
    gpio_init(BUTTON_A); // Inicializa o pino do botão A
    gpio_init(BUTTON_B); // Inicializa o pino do botão B

    gpio_set_dir(RED_LED, GPIO_OUT); // Define o pino do LED vermelho como saída
    gpio_set_dir(GREEN_LED, GPIO_OUT); // Define o pino do LED verde como saída
    gpio_set_dir(BLUE_LED, GPIO_OUT); // Define o pino do LED azul como saída
    gpio_set_dir(BUTTON_A, GPIO_IN); // Define o pino do botão A como entrada
    gpio_set_dir(BUTTON_B, GPIO_IN); // Define o pino do botão B como entrada

    gpio_pull_up(BUTTON_A); // Habilita o pull-up do botão A
    gpio_pull_up(BUTTON_B); // Habilita o pull-up do botão B

    i2c_init(I2C_PORT, 400*5000); // Inicializa o barramento I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Define os pinos SDA e SCL
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA); // Habilita os pull-ups
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ADDRESS, I2C_PORT); // Inicializa o display OLED
    ssd1306_config(&ssd);   // Configura o display OLED
    ssd1306_send_data(&ssd); // Envia os dados para o display
    
    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_send_data(&ssd);   // Envia os dados para o display

    if(cyw43_arch_init()){
        printf("Erro ao iniciar modulo WiFi\n"); // Caso utilize um monitor serial

        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "Erro ao iniciar", 3, 30);
        ssd1306_draw_string(&ssd, " modulo WiFi", 10, 40);
        ssd1306_send_data(&ssd);

        return false;
    }

    return true;
}

bool connect_wifi(char* SSID, char* PASSWORD){
    cyw43_arch_enable_sta_mode(); // Habilita o modo estação

    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "CONECTANDO A", 3, 20);
    ssd1306_draw_string(&ssd, SSID, 3, 30);
    ssd1306_send_data(&ssd);

    if(cyw43_arch_wifi_connect_timeout_ms(SSID, PASSWORD, CYW43_AUTH_WPA3_WPA2_AES_PSK, 10000)){
        printf("Erro ao conectar a rede WiFi\n"); // Caso utilize um monitor serial

        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "ERRO", 50, 20);
        ssd1306_draw_string(&ssd, "AO CONECTAR", 20, 30);
        ssd1306_send_data(&ssd);

        return false;
    }

    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "CONECTADO A", 3, 20);
    ssd1306_draw_string(&ssd, SSID, 3, 30);
    ssd1306_send_data(&ssd);

    printf("Conectado a rede WiFi\n"); // Caso utilize um monitor serial
    return true;
}

int main()
{
    stdio_init_all();
    if(!setup() || !connect_wifi(SSID, PASSWORD)){
        sleep_ms(5000);
        return -1;
    }

    while(true){
        cyw43_arch_poll();  // Necessário para manter o WIFI ativo
        sleep_ms(100);
    }

}
