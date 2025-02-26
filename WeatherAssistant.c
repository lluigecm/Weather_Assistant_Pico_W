#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "inc/ssd1306.h"
#include "inc/assets.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/apps/http_client.h"

void extract_data_from_response();
bool setup();
void display_screens(uint screen);
bool connect_wifi(char* SSID, char* PASSWORD);
static err_t http_response_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t http_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t http_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err);
void http_request_init();
void extract_data_from_response();
bool debounce();
void buttons_handler(uint gpio, uint32_t events);

// Pinos do display OLED
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ADDRESS 0x3C

// Pinos dos LEDs e botões
#define RED_LED 13
#define BLUE_LED 12
#define BUTTON_A 5
#define BUTTON_B 6
#define JYSTCK_BTTN 22

// Constantes auxiliares (Buffer e PWM)
#define BUFFER_SIZE 1024
#define WRAP 50000
#define DIV 16.0
#define STEP_LED (0.250 * WRAP) / 100.0

// Buffer para armazenar a resposta da requisição HTTP
static char response_buffer[BUFFER_SIZE] = {0};  
static size_t response_index = 0;  // Índice atual do buffer -> auxiliar a percorrer o buffer

// Variavel de cntrole de aumento e diminuição do brilho dos LEDs
bool increase = true;

// Variáveis para armazenar informações do clima
char weather_description[100] = {0};
char temperature[10] = {0};
char feels_like[10] = {0};

// Variáveis para controle de tempo, tela do display e brilho dos LEDs
uint last_time = 0;
uint screen = 7;
uint16_t red_led_level = 0;
uint16_t blue_led_level = WRAP;

ssd1306_t ssd; // Estrutura do display OLED

int main() {
    stdio_init_all();
    if (!setup() || !connect_wifi(SSID, PASSWORD)) {
        display_screens(3);
        sleep_ms(2000);
        return -1;
    }

    http_request_init(); // Inicializa a requisição HTTP

    sleep_ms(2000); // pausa pra dar tempo de receber a resposta e atualizar variáveis de informação do clima

    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &buttons_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &buttons_handler);
    gpio_set_irq_enabled_with_callback(JYSTCK_BTTN, GPIO_IRQ_EDGE_FALL, true, &buttons_handler);

    while (true) {
        cyw43_arch_poll();  // Polling do módulo WiFi -> necessário para manter a conexão
        display_screens(screen);

        pwm_set_gpio_level(RED_LED, red_led_level);
        pwm_set_gpio_level(BLUE_LED, blue_led_level);
        if(increase){
            red_led_level += STEP_LED; // -> Aumenta o brilho do LED vermelho
            blue_led_level -= STEP_LED; // -> Diminui o brilho do LED azul
            if(red_led_level >= WRAP){
                increase = false;
            }
        } else {
            red_led_level -= STEP_LED; // -> Diminui o brilho do LED vermelho
            blue_led_level += STEP_LED; // -> Aumenta o brilho do LED azul
            if(red_led_level <= 0){
                increase = true;
            }
        }
        sleep_ms(10);
    }
}

// Função para inicializar o pino como PWM
void init_gpio_pwm(uint gpio){
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_clkdiv(slice_num, DIV);
    pwm_set_wrap(slice_num, WRAP);
    pwm_set_enabled(slice_num, true);
}

// Função auxiliar para converter uma string para maiúsculo
void toUpperString(char *str) {
    while (*str) {
        *str = toupper(*str);
        str++;
    }
}

// Função inicializar os pinos
bool setup(){
    init_gpio_pwm(RED_LED); // Inicializa o pino do LED vermelho
    init_gpio_pwm(BLUE_LED); // Inicializa o pino do LED verde

    gpio_init(BUTTON_A); // Inicializa o pino do botão A
    gpio_init(BUTTON_B); // Inicializa o pino do botão B
    gpio_init(JYSTCK_BTTN); // Inicializa o pino do botão do joystick

    gpio_set_dir(BUTTON_A, GPIO_IN); // Define o pino do botão A como entrada
    gpio_set_dir(BUTTON_B, GPIO_IN); // Define o pino do botão B como entrada
    gpio_set_dir(JYSTCK_BTTN, GPIO_IN); // Define o pino do botão do joystick como entrada

    gpio_pull_up(BUTTON_A); // Habilita o pull-up do botão A
    gpio_pull_up(BUTTON_B); // Habilita o pull-up do botão B
    gpio_pull_up(JYSTCK_BTTN); // Habilita o pull-up do botão do joystick

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

    display_screens(-1);

    return true;
}

// Função para tratar a tela exibida no display
void display_screens(uint screen){
    ssd1306_fill(&ssd, false);

    switch (screen){
        case -1:
            ssd1306_draw_string(&ssd, "PROJETO", 3, 15);
            ssd1306_draw_string(&ssd, "FINAL", 3, 30);
            ssd1306_draw_string(&ssd, "EMBARCATECH", 3, 45);
            break;
        case 0:
            ssd1306_draw_string(&ssd, "CONECTANDO A", 3, 20);
            ssd1306_draw_string(&ssd, SSID, 3, 35);
            break;
        case 1:
            ssd1306_draw_string(&ssd, "CONECTADO A", 3, 20);
            ssd1306_draw_string(&ssd, SSID, 3, 35);
            break;
        case 2:
            ssd1306_draw_string(&ssd, "ERRO", 50, 20);
            ssd1306_draw_string(&ssd, "AO CONECTAR", 20, 35);
            break;
        case 3:
            ssd1306_draw_string(&ssd, "ERRO", 50, 20);
            ssd1306_draw_string(&ssd, "AO INICIAR", 20, 35);
            break;
        case 4:
            ssd1306_draw_string(&ssd, "REQUISITANDO", 3, 20);
            ssd1306_draw_string(&ssd, "DADOS", 30, 35);
            break;
        case 5:
            ssd1306_draw_string(&ssd, "DADOS", 3, 20);
            ssd1306_draw_string(&ssd, "RECEBIDOS", 3, 35);
            break;
        case 6:
            ssd1306_draw_string(&ssd, "TRATANDO", 3, 20);
            ssd1306_draw_string(&ssd, "DADOS", 3, 35);
            break;
        case 7:
            ssd1306_draw_string(&ssd, "TEMPERATURA", 3, 20);
            ssd1306_draw_string(&ssd, temperature, 3, 35);
            break;
        case 8: 
            ssd1306_draw_string(&ssd, "SENSACAO", 3, 20);
            ssd1306_draw_string(&ssd, "TERMICA", 3, 35);
            ssd1306_draw_string(&ssd, feels_like, 3, 50);
            break;
        case 9:
            ssd1306_draw_string(&ssd, "TEMPO", 3, 20);
            ssd1306_draw_string(&ssd, weather_description, 3, 35);
            break;    
        };
    ssd1306_send_data(&ssd);
    sleep_ms(300);
}

// Função para conectar a rede WiFi
bool connect_wifi(char* SSID, char* PASSWORD){
    cyw43_arch_enable_sta_mode(); // Habilita o modo estação

    display_screens(0);

    if(cyw43_arch_wifi_connect_timeout_ms(SSID, PASSWORD, CYW43_AUTH_WPA3_WPA2_AES_PSK, 10000)){ // Tenta a coneeção com a rede WiFi - timeout de 10s
        printf("Erro ao conectar a rede WiFi\n"); // Caso utilize um monitor serial

       display_screens(2);

        return false;
    }

    display_screens(1);

    printf("Conectado a rede WiFi\n"); // Caso utilize um monitor serial
    return true;
}

// Função de callback pque lida com a resposta HTTP
static err_t http_response_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    display_screens(5);
    if (p != NULL) {
        // Certifica de não ultrapassar o tamanho do buffer
        if (response_index + p->len < BUFFER_SIZE - 1) {
            memcpy(response_buffer + response_index, p->payload, p->len);
            response_index += p->len;
            response_buffer[response_index] = '\0';  // Finaliza a string
        }
        pbuf_free(p);
    } else {
        // Resposta finalizada, mostra no terminal para verificação
        printf("Resposta HTTP armazenada:\n%s\n", response_buffer);
        display_screens(6);
        extract_data_from_response();
        tcp_close(tpcb);
    }
    return ERR_OK;  // ERR_OK indica que a função foi executada com sucesso
}

// Função de callback para enviar a requisição HTTP
static err_t http_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    printf("Dados enviados com sucesso. Aguardando resposta...\n");
    tcp_recv(tpcb, http_response_callback); // Aguarda a resposta do servidor
    return ERR_OK;
}

// Função de callback para conexão com o servidor
static err_t http_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err == ERR_OK) {
        printf("Conectado ao servidor! Enviando requisição HTTP...\n");
        display_screens(4);
        tcp_write(tpcb, REQUEST, sizeof(REQUEST) - 1, TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
        tcp_sent(tpcb, http_sent_callback);
    } else {
        printf("Erro na conexão: %d\n", err);
        tcp_close(tpcb);
    }
    return ERR_OK;
}

// Função para inicializar a requisição HTTP
void http_request_init() {
    struct tcp_pcb *pcb = tcp_new(); 
    if (pcb == NULL) {
        printf("Falha ao criar PCB TCP\n");
        return;
    }
    ip4_addr_t server_ip;
    if (!ip4addr_aton(SERVER_IP, &server_ip)) { // Converte o endereço IP para o formato correto
        printf("Erro ao converter endereço IP\n");
        return;
    }
    tcp_connect(pcb, &server_ip, SERVER_PORT, http_connected_callback); // Conecta ao servidor
}

// Função de manipulação de string para extrair os dados da resposta
void extract_data_from_response() {
    char *description_start, *description_end;
    char *temp_start, *temp_end;
    char *feels_like_start, *feels_like_end;

    // Extrai "description"
    description_start = strstr(response_buffer, "\"description\":\""); // Procura a string "description"
    if (description_start) {
        description_start += strlen("\"description\":\""); // Avança para o início da descrição
        description_end = strchr(description_start, '"'); // Procura o final da descrição
        if (description_end) {
            size_t length = description_end - description_start;
            strncpy(weather_description, description_start, length); // Copia a descrição para a variável
            weather_description[length] = '\0';  // Finaliza a string
            toUpperString(weather_description); // Converte para maiúsculo
        }
    }

    // Extrai "temp"
    temp_start = strstr(response_buffer, "\"temp\":"); // Procura a string "temp"
    if (temp_start) {
        temp_start += strlen("\"temp\":"); // Avança para o início da temperatura
        temp_end = strchr(temp_start, ',');
        if (temp_end) {
            size_t length = temp_end - temp_start;
            strncpy(temperature, temp_start, length);   ;// Copia a temperatura para a variável
            temperature[length] = '\0';
            strcat(temperature, "_C");  // Adiciona o símbolo de graus Celsius(usei _ como simbolo especial para referenciar o º)
        }
    }

    // Extrai "feels_like"
    feels_like_start = strstr(response_buffer, "\"feels_like\":"); // Procura a string "feels_like"
    if (feels_like_start) {
        feels_like_start += strlen("\"feels_like\":");  // Avança para o início da sensação térmica
        feels_like_end = strchr(feels_like_start, ',');
        if (feels_like_end) {
            size_t length = feels_like_end - feels_like_start;
            strncpy(feels_like, feels_like_start, length);  // Copia a sensação térmica para a variável
            feels_like[length] = '\0';
            strcat(feels_like, "_C");   // Adiciona o símbolo de graus Celsius(usei _ como simbolo especial para referenciar o º)
        }
    }
}

// Função para evitar o boucing dos botões
bool debounce(){
    uint current_time = to_ms_since_boot(get_absolute_time());
    if(current_time - last_time > 200){
        last_time = current_time;
        return true;
    }
    return false;
}

// Função IRQ de callback para tratar os botões
void buttons_handler(uint gpio, uint32_t events){
    if(debounce()){
        if(gpio == BUTTON_A){
            screen = screen == 7 ? 8 : screen == 8 ? 9 : screen; // Alterna entre as telas
        }
        if(gpio == BUTTON_B){
            screen = screen == 9 ? 8 : screen == 8 ? 7 : screen; // Alterna entre as telas
        }
        if(gpio == JYSTCK_BTTN){
            http_request_init();    // Requisita os dados novamente ao pressionar o botão do joystick
        }
    }
}