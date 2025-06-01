# 🏷️ HidroSafe

## 💧 Visão Geral

HidroSafe é um **sistema embarcado** para monitoramento de nível de água, temperatura e umidade, com foco em **eventos de enchente**, **persistência em EEPROM** e **exibição dinâmica** de informações. O dispositivo roda em um **Arduino** e utiliza sensores ultrassônico, DHT22 e de chuva, além de um LCD I2C 16×2 e um teclado matricial 4×4 para navegação em menu.

---

## 🚀 Principais Features

- **📐 Medição de Nível de Água**  
  - Sensor ultrassônico calcula distância até a superfície.  
  - Nível em centímetros = `altura_do_sensor` − `distância_meassured`.  

- **🌡️ Temperatura & Umidade**  
  - Sensor DHT22 faz leituras periódicas.  
  - Alertas quando umidade ultrapassa valor crítico (`flagHum`).  
  - Alertas visuais de temperatura (ícones de termômetro).

- **☔ Sensor de Chuva**  
  - Detecta chuva via entrada digital com pull-up.  
  - Combinação com nível para gerar “evento de enchente”.

- **🗓️ Registro de Eventos de Enchente**  
  - Condição de início: nível > `flagAltura` **e** chuva detectada.  
  - Condição de fim: chuva cessou **e** nível ≤ `flagAltura`.  
  - Cada evento grava na EEPROM:  
    1. Timestamp de início (4 bytes)  
    2. Timestamp de fim (4 bytes)  
    3. Pico de nível (2 bytes)  
  - Histórico acessível via menu de “Debug EEPROM”.

- **📊 Displays Dinâmicos**  
  - **Modo 1: Barra de Nível** (16 colunas)  
    - Exibe gráfico proporcional ao nível atual.  
    - Marca visual do limite crítico (`flagAltura`).  
  - **Modo 2: Info Completa**  
    - Exibe “CHUVA” ou “SECO”.  
    - Nível (cm), umidade (%) com ícone de gota, temperatura (C/F) com ícone de termômetro.  
  - **Alternância Automática**  
    - Troca entre Modo 1 e Modo 2 a cada 15 segundos (quando `modoDisplay = 0`).  
    - Pode fixar em Modo 1 (`modoDisplay = 1`) ou Modo 2 (`modoDisplay = 2`).

- **⚙️ Menus Interativos**  
  - LCD I2C 16×2 exibe opções de menu e descrições.  
  - Teclado 4×4 (linhas: 13,12,11,10 / colunas: 9,8,7,6) para:  
    - `A` (▲): Sobe no menu  
    - `B` (▼): Desce no menu  
    - `C` (◀): Voltar/Cancelar  
    - `D` (▶): Confirmar/Entrar  

---

## 🎯 Diferenciais do HidroSafe

1. **🔒 Registro Estruturado de Eventos**  
   - Cada evento de enchente ocupa 10 bytes na EEPROM, garantindo uso eficiente do espaço.  
   - Ponteiro de gravação mantido em EEPROM (4 bytes), possibilitando reinícios sem perda de histórico.  
   - Lógica de “cooldown” (configurável) evita gravações repetidas de flags em curto intervalo.

2. **⚡ Exibição Inteligente e Customizada**  
   - Valores críticos (nível, umidade e temperatura) são destacados com ícones 5×8 customizados.  
   - Alternância automática proporciona visão geral sem intervenção manual.  
   - Menu portátil com **rolagem de texto** (configurável) melhora usabilidade em display 16×2.

3. **💾 Persistência Completa em EEPROM**  
   - Parâmetros de configuração (7 variáveis principais, total 16 bytes)  
   - Histórico de flags de enchente (até 98 eventos possíveis)  
   - Restauração de fábrica com valores-padrão definidos em PROGMEM  

4. **🎨 Design de UX Minimalista**  
   - **Animação de introdução** opcional (logo “HidroSafe” + aranha) para feedback visual ao ligar.  
   - Ícones gráficos (gotas, sol e barras) tornam as leituras mais intuitivas.  
   - Texto de menu e descrições rolante, permitindo leitura de strings longas em 2 linhas.

---

## 🏗️ Arquitetura & Estrutura

### 📟 Sensores e Entradas

| Sensor           | Pino  | Função                                        |
|------------------|-------|-----------------------------------------------|
| Ultrassônico     | 4/Trig, 3/Echo | Calcula distância → nível de água (cm)      |
| DHT22            | 5     | Mede temperatura (°C/°F) e umidade (%)         |
| Chuva            | 2     | Detecta chuva (HIGH = chuva, pull-up interno)  |
| LDR (opcional)   | A0    | Sensor de luminosidade (em procedimentos futuros) |

### 📺 Displays

- **LCD I2C 16×2 (0x27)**  
  - Linha 0: Títulos, textos rolantes ou status (“CHUVA/SECO”).  
  - Linha 1: Gráficos, descrições de menu ou informações numéricas.

- **Teclado Matricial 4×4**  
  - Permite navegar entre menus, ajustar variáveis e sair de modos de exibição.

### 🎛️ EEPROM (Endereço & Mapeamento)

| Endereço | Variável             | Tamanho | Descrição                              |
|----------|----------------------|---------|----------------------------------------|
| 0 … 1    | `intervaloScroll`    | 2 bytes | Rolagem de texto (ms)                  |
| 2 … 3    | `unidadeTemperatura` | 2 bytes | 1 = Celsius, 2 = Fahrenheit            |
| 4 … 5    | `display`            | 2 bytes | (reservado)                            |
| 6 … 7    | `intro`              | 2 bytes | 0 = desativado, 1 = ativado animação   |
| 8 … 9    | `altura`             | 2 bytes | Altura do sensor ultrassônico (cm)     |
| 10 … 11  | `flagAltura`         | 2 bytes | Nível crítico para evento (cm)         |
| 12 … 13  | `flagHum`            | 2 bytes | Limite de umidade para alerta (%)      |
| 14 … 15  | `flagCooldown`       | 2 bytes | Cooldown entre flags (segundos)        |
| 16 … 17  | `modoDisplay`        | 2 bytes | 0 = auto, 1 = fixo modo1, 2 = fixo modo2 |
| 20 … 1009| Histórico de Flags   | 10 bytes/flag | (Início/4B, Fim/4B, Pico/2B)         |
| 1010 … 1013 | `ponteiroFlags`   | 4 bytes | Próximo endereço livre na região de flags |
| 1001     | `primeiroSetup`      | 1 byte  | 0 = setup pendente, 1 = já realizado   |

### 📈 Fluxo de Funcionalidade

1. **Inicialização**  
   - Carrega parâmetros da EEPROM.  
   - Executa animação opcional “HidroSafe”.  
   - Verifica byte em `1001`; se ≠ 1, executa “primeiro setup” (restaura fábrica).  

2. **Menu Principal**  
   - Exibe opções:  
     1. 📡 **Leitura/Monitoramento**  
     2. ⚙️ **Configuração**  
     3. 📋 **Logs**  

3. **Submenus & Ajustes**  
   - Configura parâmetros:  
     - Velocidade de rolagem, unidade de temperatura, modo de display, reset de fábrica, intro, limites de sensor.  
   - Logs permitem:  
     - 📥 Debug de EEPROM (exibe parâmetros e histórico de flags via Serial)  
     - 🗑️ Limpar flags (apaga região de eventos)  

4. **Monitoramento Contínuo**  
   - A cada 500 ms:  
     - Lê DHT22 (temp/hum), sensor ultrassônico (nível), sensor de chuva.  
     - Atualiza display conforme modo (1 ou 2) ou alterna a cada 15 s.  
   - Checa condição de “evento de enchente”:  
     - Se `nível > flagAltura` **e** chuva, inicia evento.  
     - Durante evento, registra `picoNivel`.  
     - Quando chuva cessa **e** nível ≤ `flagAltura`, finaliza evento e grava dados na EEPROM.

---

## 📦 Como Usar

1. **Monte o hardware** conforme descrito em “Arquitetura & Estrutura”.  
2. **Abra este projeto na IDE Arduino** e faça upload para sua placa.  
3. **Ao ligar**:  
   - A animação será exibida se habilitada.  
   - Menu principal surgirá.  
4. **Navegue no menu** usando o teclado:  
   - Ajuste limites, modos e opções.  
   - Acesse monitoramento para ver status em tempo real e geração de flags.  
5. **Conecte no Serial Monitor** (9600 bps) para ver logs de eventos e depuração de EEPROM.  

---

## 🎨 Representações Visuais

### Modo 1: Barra de Nível

```
┌────────────────┐
│ Nível:         │
│ ████▌     ▲    │
└────────────────┘
```
- “█” = blocos completos; “▌” = bloco parcial; “▲” = marca do limite crítico.

### Modo 2: Info Completa

```
┌────────────────┐
│ CHUVA          │
│ 123cm  45% 💧  │
│         T: 25C 🌡️ │
└────────────────┘
```
- 💧 = gota (vazia ou cheia conforme `flagHum`); 🌡️ = termômetro (normal ou alerta).

---

## 🌟 Colaboradores

- **Luigi** – Arquitetura de Firmware, Lógica de Eventos, Menus  
- **Rogério** – Design de Displays, EEPROM, Animação “HidroSafe”  

---

## 📄 Licença

Este projeto é liberado sob a **MIT License**.

---
