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

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ADDRESS 0x3C

#define RED_LED 13
#define GREEN_LED 11
#define BLUE_LED 12
#define BUTTON_A 5
#define BUTTON_B 6
#define BUFFER_SIZE 1024

void extract_data_from_response();

static char response_buffer[BUFFER_SIZE] = {0};  // Buffer para armazenar a resposta
static size_t response_index = 0;  // Índice atual do buffer

char weather_description[100] = {0};
char temperature[10] = {0};
char feels_like[10] = {0};

ssd1306_t ssd; // Estrutura do display OLED

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

static err_t http_response_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p != NULL) {
        // Certifique-se de não ultrapassar o tamanho do buffer
        if (response_index + p->len < BUFFER_SIZE - 1) {
            memcpy(response_buffer + response_index, p->payload, p->len);
            response_index += p->len;
            response_buffer[response_index] = '\0';  // Finaliza a string
        }
        pbuf_free(p);
    } else {
        // Resposta finalizada, imprimir para verificação
        printf("Resposta HTTP armazenada:\n%s\n", response_buffer);
        extract_data_from_response();
        tcp_close(tpcb);
    }
    return ERR_OK;
}

static err_t http_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    printf("Dados enviados com sucesso. Aguardando resposta...\n");
    tcp_recv(tpcb, http_response_callback);
    return ERR_OK;
}

static err_t http_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err == ERR_OK) {
        printf("Conectado ao servidor! Enviando requisição HTTP...\n");
        tcp_write(tpcb, REQUEST, sizeof(REQUEST) - 1, TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
        tcp_sent(tpcb, http_sent_callback);
    } else {
        printf("Erro na conexão: %d\n", err);
        tcp_close(tpcb);
    }
    return ERR_OK;
}

void http_request_init() {
    struct tcp_pcb *pcb = tcp_new();
    if (pcb == NULL) {
        printf("Falha ao criar PCB TCP\n");
        return;
    }
    ip4_addr_t server_ip;
    if (!ip4addr_aton(SERVER_IP, &server_ip)) {
        printf("Erro ao converter endereço IP\n");
        return;
    }
    tcp_connect(pcb, &server_ip, SERVER_PORT, http_connected_callback);
}

void extract_data_from_response() {
    char *description_start, *description_end;
    char *temp_start, *temp_end;
    char *feels_like_start, *feels_like_end;

    // Extrai "description"
    description_start = strstr(response_buffer, "\"description\":\"");
    if (description_start) {
        description_start += strlen("\"description\":\"");
        description_end = strchr(description_start, '"');
        if (description_end) {
            size_t length = description_end - description_start;
            strncpy(weather_description, description_start, length);
            weather_description[length] = '\0';  // Finaliza a string
        }
    }

    // Extrai "temp"
    temp_start = strstr(response_buffer, "\"temp\":");
    if (temp_start) {
        temp_start += strlen("\"temp\":");
        temp_end = strchr(temp_start, ',');
        if (temp_end) {
            size_t length = temp_end - temp_start;
            strncpy(temperature, temp_start, length);
            temperature[length] = '\0';
        }
    }

    // Extrai "feels_like"
    feels_like_start = strstr(response_buffer, "\"feels_like\":");
    if (feels_like_start) {
        feels_like_start += strlen("\"feels_like\":");
        feels_like_end = strchr(feels_like_start, ',');
        if (feels_like_end) {
            size_t length = feels_like_end - feels_like_start;
            strncpy(feels_like, feels_like_start, length);
            feels_like[length] = '\0';
        }
    }
}

int main() {
    stdio_init_all();
    if (!setup() || !connect_wifi(SSID, PASSWORD)) {
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "ERRO", 50, 20);
        ssd1306_draw_string(&ssd, "AO INICIAR", 20, 30);
        ssd1306_send_data(&ssd);
        sleep_ms(5000);
        return -1;
    }

    http_request_init();

    sleep_ms(2000);

    printf("dscr: %s\n", weather_description);
    printf("temp: %sºC\n", temperature);
    printf("feels: %sºC\n", feels_like);

    while (true) {
        cyw43_arch_poll();
        sleep_ms(100);
    }
}