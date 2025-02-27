# Weather Assistant 🌦️

## Descrição 📝
Este projeto utiliza a placa **BitDogLab** para explorar conceitos de requisições HTTP, para acessar uma API OpenWeather, que retorna informações climaticas sobre o local desejado. 🛠️

---

## Funcionalidades 🎮
Alguns botões das placas possuem algumas funcionalidades:

- **Botão do Joystick**: Faz uma requisição HTTP para a API do OpenWeatherMap.
- **Display**: Exibe informações sobre status da conexão, temperatura, sensação térmica, e tempo(chuva, ensolarado, nublado).
- **Botão A e B**: Alternam entre as telas do **Display**.

[**Vídeo de Demonstração** 🎥](https://youtu.be/zf86yEIYDLI)

## Observações 📌

- **API Key**: Para utilizar o programa, é necessário obter uma chave de API na [OpenWeatherMap](https://openweathermap.org/) e colocá-la no arquivo `inc/assets.h`.
- **WiFi**: O programa utiliza a conexão WiFi para fazer requisições HTTP.
- **SSID e Senha**: É necessário configurar o SSID e senha da rede WiFi no arquivo `inc/assets.h`.
- **CIDADE**: A cidade utilizada para a requisição deve ser configurada no arquivo `inc/assets.h`. Exemplo: "Sao Paulo, br".

---

## Como Compilar 🛠️
Para compilar o programa, siga os passos abaixo:

1. Configure o ambiente de desenvolvimento para o **Raspberry Pi Pico**.
2. Utilize um compilador C compatível para gerar os arquivos `.uf2` e `.elf`.

Exemplo de botão de compilação:

![Botão Compilador](fotos_readme/compilador.png)

---

## Como Executar ⚡

1. Conecte a placa **BitDogLab** via cabo **micro-USB** 🔌.
2. Ative o modo **BOOTSEL** da placa.
3. Clique no botão **Run** ▶️.
4. Pressione ou movimente o **Joystick**, além do **Botão A** para usufruir das funcionalidades🎮.

---

## Requisitos 📋

- Compilador C (ex: **gcc** ou equivalente) 🖥️.
- Sistema operacional compatível com programas em C.
- Conta na OpenWeatherMap para obter a chave de API 🌐. É gratuito, e fornece até 1000 requests/dia.
- Conexão WiFi.
- Extensão **Raspberry Pi Pico**.
- Placa **BitDogLab**.

---
