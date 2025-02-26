#define API_KEY "SUA_API_KEY"
#define CIDADE "SUA_CIDADE,SEU_PAIS"
#define API_URL "/data/2.5/weather?q="CIDADE"&appid="API_KEY"&units=metric&lang=pt_br"
#define URL "api.openweathermap.org"

#define SERVER_IP "38.89.70.155"
#define REQUEST "GET "API_URL" HTTP/1.1\r\n" \
                "Host: api.openweathermap.org\r\n" \
                "Connection: close\r\n\r\n"
#define SERVER_PORT 80

static char* SSID = "SEU_SSID";
static char* PASSWORD = "SUA_SENHA";