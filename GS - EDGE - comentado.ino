//====================== BIBLIOTECAS ======================
#include <EEPROM.h>               // Biblioteca para manipular a EEPROM (memória não volátil)
#include <Wire.h>                 // Biblioteca para comunicação I2C (usada pelo LCD e pelo RTC)
#include <LiquidCrystal_I2C.h>    // Biblioteca para controlar o display LCD via I2C
#include <Keypad.h>               // Biblioteca para ler entradas de teclado matricial 4x4
#include "DHT.h"                   // Biblioteca para sensor de temperatura e umidade DHT
#include <RTClib.h>               // Biblioteca para módulo de Relógio de Tempo Real (DS1307)

//====================== VARIÁVEIS GLOBAIS ======================
const int velocidade = 20;           // Velocidade em milissegundos para rolagem de texto no LCD
short int menuatual = 0, opcao = 0;   // Variáveis de controle do menu atual e opção selecionada

// Definições de valores para indicar direção das setas no menu
#define SETABAIXO 1                  // Exibe seta para baixo
#define SETACIMA 2                   // Exibe seta para cima
#define SETAS 0                      // Exibe ambas as setas (cima e baixo)

#define CHUVA 2                      // Pino digital conectado ao sensor de chuva (pull-up interno)

#define DHTpin 5                     // Pino digital conectado ao sensor DHT22
#define DHTmodel DHT22               // Define o modelo do sensor como DHT22
DHT dht(DHTpin, DHTmodel);            // Cria instância do sensor DHT

#define LDR A0                       // Pino analógico conectado ao sensor LDR (não utilizado diretamente neste trecho)

RTC_DS1307 rtc;                       // Instância do objeto de RTC (Real Time Clock DS1307)

LiquidCrystal_I2C lcd(0x27, 16, 2);   // Instância do display LCD (endereço I2C 0x27, 16 colunas, 2 linhas)

//====================== CONFIGURAÇÃO DO TECLADO MATRICIAL ======================
// Definição de quantas linhas e colunas o teclado possui
const uint8_t ROWS = 4;
const uint8_t COLS = 4;

// Mapeamento de teclas: cada elemento corresponde a uma tecla no teclado matricial
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '-', 'D' }
};

// Pinos do Arduino usados para conectar as colunas do teclado
uint8_t colPins[COLS] = { 9, 8, 7, 6 };
// Pinos do Arduino usados para conectar as linhas do teclado
uint8_t rowPins[ROWS] = { 13, 12, 11, 10 };

// Cria instância do Keypad usando mapeamento de teclas, pinos de linhas/colunas e tamanho
Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

//====================== ANIMAÇÃO DE INICIALIZAÇÃO ======================
// Sprites para a animação de aranha (armazenados em PROGMEM para economizar RAM)
const byte anim_aranha[2][4][8] PROGMEM = {
  { 
    { B00000, B01001, B01001, B01001, B00101, B00011, B00111, B00111 },
    { B00111, B00111, B00011, B00101, B01001, B01001, B01001, B00000 },
    { B00000, B00100, B00100, B01000, B01000, B11100, B11111, B11100 },
    { B11100, B11111, B11100, B01000, B01000, B00100, B00100, B00000 }
  },
  { 
    { B00000, B00101, B00101, B00101, B00011, B00101, B00111, B00111 },
    { B00111, B00111, B00101, B00011, B00101, B00101, B00101, B00000 },
    { B00000, B01000, B01000, B00100, B00100, B11100, B11111, B11100 },
    { B11100, B11111, B11100, B00100, B00100, B01000, B01000, B00000 }
  }
};

unsigned long anim_tempo_ultimo_quadro = 0; // Armazena o momento (millis) do último quadro mostrado
const int anim_intervalo_principal = 320;   // Intervalo em milissegundos entre quadros da animação
int anim_pose_atual_aranha = 0;             // Controla qual pose (0 ou 1) da aranha exibir

// Enumeração dos possíveis estados da animação de inicialização
enum anim_estado_sistema {
  anim_entrando_e_puxando,            // Aranha entra puxando o texto
  anim_texto_centralizado_segurando,   // Texto aparece centralizado e aguarda alguns instantes
  anim_soltando_fio,                   // O fio solta e aranha começa a sair
  anim_aranha_saindo,                  // Aranha sai da tela
  anim_texto_final_exibido             // Texto final exibido e animação concluída
};

anim_estado_sistema anim_estado_atual = anim_entrando_e_puxando; // Estado inicial da animação

// Texto que aparecerá na animação e suas propriedades
const char* anim_nome_empresa = "HidroSafe";
const int anim_comprimento_nome_empresa = 9;       // Comprimento (número de caracteres) do nome
const char* anim_linha_decorativa_texto = "---------";
const int anim_comprimento_linha_decorativa = 9;   // Comprimento da linha decorativa abaixo do nome
const int anim_colunas_lcd = 16;                   // Número de colunas do display

// Variáveis que controlam posições de desenho (inicializadas para fora da tela)
int anim_coluna_atual_esquerda_aranha = -(anim_comprimento_nome_empresa + 3); // Aranha começa à esquerda fora da tela
int anim_coluna_atual_inicio_texto;                  // Coluna onde o texto começará a aparecer
int anim_coluna_atual_fio;                            // Coluna onde o fio está sendo desenhado

// Cálculo das posições centrais alvo (para centralizar texto e linha décor).
const int anim_coluna_alvo_inicio_texto = (anim_colunas_lcd - anim_comprimento_nome_empresa) / 2;
const int anim_coluna_alvo_inicio_linha_decorativa = (anim_colunas_lcd - anim_comprimento_linha_decorativa) / 2;

unsigned long anim_tempo_mudanca_estado = 0;          // Armazena o tempo em que um estado da animação mudou
const int anim_duracao_estado_segurando = 1500;       // Duração (em ms) do estado em que o texto fica centralizado antes de soltar o fio

//====================== FUNÇÕES DE ANIMAÇÃO ======================

// Carrega, na memória do LCD, os bytes correspondentes à pose atual da aranha (4 partes)
void anim_carregar_sprites_pose(byte pose) {
  for (byte parte = 0; parte < 4; parte++) {
    uint8_t buffer[8];
    // Copia os dados do sprite armazenado em PROGMEM para um buffer temporário
    memcpy_P(buffer, anim_aranha[pose][parte], 8);
    // Cria/atualiza o caractere customizado (índice igual a 'parte') no LCD
    lcd.createChar(parte, buffer);
  }
}

// Desenha a aranha na posição informada (coluna esquerda)
void anim_desenhar_aranha(byte col_esq) {
  // Se a posição for visível (entre 0 e colunas-1), desenha parte esquerda e direita
  if (col_esq >= 0 && col_esq < anim_colunas_lcd) {
    lcd.setCursor(col_esq, 0); lcd.write(0); // Desenha parte superior esquerda
    lcd.setCursor(col_esq, 1); lcd.write(1); // Desenha parte inferior esquerda
  }
  if ((col_esq + 1) >= 0 && (col_esq + 1) < anim_colunas_lcd) {
    lcd.setCursor(col_esq + 1, 0); lcd.write(2); // Desenha parte superior direita
    lcd.setCursor(col_esq + 1, 1); lcd.write(3); // Desenha parte inferior direita
  }
}

// Desenha o texto "HidroSafe" e a linha decorativa (quando aplicável)
void anim_desenhar_texto() {
  // Escreve cada caractere do nome na linha superior, com deslocamento animado
  for (int i = 0; i < anim_comprimento_nome_empresa; i++) {
    int col = anim_coluna_atual_inicio_texto + i;
    if (col >= 0 && col < anim_colunas_lcd)
      lcd.setCursor(col, 0), lcd.print(anim_nome_empresa[i]);
  }
  // Após texto centralizado e aguardando, desenha a linha decorativa na segunda linha
  if (anim_estado_atual >= anim_texto_centralizado_segurando) {
    for (int i = 0; i < anim_comprimento_linha_decorativa; i++) {
      int col = anim_coluna_alvo_inicio_linha_decorativa + i;
      if (col >= 0 && col < anim_colunas_lcd)
        lcd.setCursor(col, 1), lcd.print(anim_linha_decorativa_texto[i]);
    }
  }
}

// Executa a sequência completa de animação de inicialização
void anim_executar_inicializacao() {
  anim_estado_atual = anim_entrando_e_puxando;
  // Reinicia posições e temporizadores
  anim_coluna_atual_esquerda_aranha = -(anim_comprimento_nome_empresa + 3);
  anim_tempo_ultimo_quadro = millis();

  // Enquanto o estado não for o final, executa laço de animação
  while (anim_estado_atual != anim_texto_final_exibido) {
    if (millis() - anim_tempo_ultimo_quadro >= anim_intervalo_principal) {
      lcd.clear();
      anim_tempo_ultimo_quadro = millis();
      // Alterna entre pose 0 e pose 1 da aranha (para dar a impressão de movimento)
      anim_pose_atual_aranha = 1 - anim_pose_atual_aranha;
      // Carrega os sprites correspondentes à pose atual
      anim_carregar_sprites_pose(anim_pose_atual_aranha);

      // Lógica de transição de estados
      switch (anim_estado_atual) {
        case anim_entrando_e_puxando:
          // Move a aranha uma coluna para a direita a cada quadro
          anim_coluna_atual_esquerda_aranha++;
          // Define posição do fio (coluna imediatamente à esquerda da aranha)
          anim_coluna_atual_fio = anim_coluna_atual_esquerda_aranha - 1;
          // Define posição inicial do texto (fio menos comprimento do texto)
          anim_coluna_atual_inicio_texto = anim_coluna_atual_fio - anim_comprimento_nome_empresa;
          // Quando o texto alcança a posição central desejada, muda para próximo estado
          if (anim_coluna_atual_inicio_texto >= anim_coluna_alvo_inicio_texto) {
            anim_coluna_atual_inicio_texto = anim_coluna_alvo_inicio_texto;
            anim_estado_atual = anim_texto_centralizado_segurando;
            anim_tempo_mudanca_estado = millis();
          }
          break;

        case anim_texto_centralizado_segurando:
          // Fica nesse estado por determinado tempo para exibir texto centralizado
          if (millis() - anim_tempo_mudanca_estado > anim_duracao_estado_segurando)
            anim_estado_atual = anim_soltando_fio;
          break;

        case anim_soltando_fio:
          // Após aguardar, transita imediatamente para estado de aranha saindo
          anim_estado_atual = anim_aranha_saindo;
          break;

        case anim_aranha_saindo:
          // Move a aranha para fora da tela, coluna a coluna
          anim_coluna_atual_esquerda_aranha++;
          if (anim_coluna_atual_esquerda_aranha >= anim_colunas_lcd) {
            anim_estado_atual = anim_texto_final_exibido;
            lcd.clear();
            anim_desenhar_texto();    
            delay(1000); // Exibe texto final por 1 segundo
          }
          break;

        default:
          break;
      }

      // Se ainda não terminou, desenha fio, aranha e texto no LCD
      if (anim_estado_atual != anim_texto_final_exibido) {
        // Se o fio ainda estiver visível, desenha caracter '-' conectando ao texto
        if (anim_coluna_atual_fio >= 0 && anim_coluna_atual_fio < anim_colunas_lcd &&
            anim_estado_atual <= anim_texto_centralizado_segurando) {
          lcd.setCursor(anim_coluna_atual_fio, 0);
          lcd.print("-");
        }
        // Desenha o sprite da aranha na posição atual
        anim_desenhar_aranha(anim_coluna_atual_esquerda_aranha);
        // Desenha o texto (e, quando aplicável, a linha decorativa)
        anim_desenhar_texto();
      }
    }
    // Pequena pausa para aliviar a CPU
    delay(10);
  }

  // Ao final da animação, limpa a tela
  lcd.clear();
}

//====================== CONFIGURAÇÕES E ENDEREÇOS NA EEPROM ======================
// Endereços (offset em bytes) para salvar cada variável de configuração na EEPROM
#define CFG_INTERVALO_SCROLL_ADDR 0   // 2 bytes (uint16_t)
#define CFG_UNIDADE_TEMP_ADDR     2   // 2 bytes (uint16_t)
#define CFG_DISPLAY_ADDR          4   // 2 bytes (int16_t)
#define CFG_INTRO_ADDR            6   // 2 bytes (uint16_t)
#define CFG_ALTURA_ADDR           8   // 2 bytes (uint16_t)
#define CFG_FLAGALTURA_ADDR      10   // 2 bytes (uint16_t)
#define CFG_FLAGHUM_ADDR         12   // 2 bytes (uint16_t)
#define CFG_FLAGCOOLDOWN_ADDR    14   // 2 bytes (uint16_t)
#define CFG_MODODISPLAY_ADDR     16   // 2 bytes (uint16_t)
#define EEPROM_LUZ_MAX_ADDR      18   // (não usado diretamente)
#define ENDERECO_INICIAL_FLAGS   20   // Início dos registros de eventos (flags)
int enderecoEEPROM;                  // Ponteiro que indica onde gravar próxima flag de evento

// Valores padrão (de fábrica) para cada configuração, armazenados em PROGMEM
const uint16_t config_fac[] PROGMEM = {
  800,  // intervaloScroll: intervalo de rolagem (em ms)
  1,    // unidadeTemperatura: 1=Celsius, 2=Fahrenheit
  -3,   // display (modo não detalhado neste trecho)
  0,    // intro (show/hide animação na inicialização)
  500,  // altura: altura do dispositivo em relação ao solo (em cm)
  360,  // flagAltura: nível crítico de água para gerar evento (em cm)
  30,   // flagHum: limite de umidade para alerta (em %)
  70,   // flagCooldown: tempo mínimo entre gravações de flags (em segundos)
  0     // modoDisplay: 0=exibir alternadamente displays 1 e 2, 1=sempre display1, 2=sempre display2
};

const uint8_t variaveismutaveis = 9;   // Quantidade de variáveis configuráveis (para restaurar fábrica)

// Variáveis que serão carregadas/salvas na EEPROM (em RAM durante a execução)
uint16_t intervaloScroll;
uint16_t unidadeTemperatura;
int16_t display;
uint16_t intro;
uint16_t altura;
uint16_t flagAltura;
uint16_t flagHum;
uint16_t flagCooldown;
uint16_t modoDisplay;

//====================== FUNÇÕES DE CONFIGURAÇÃO ======================

// Carrega todas as variáveis de configuração da EEPROM para a RAM
void definevars(void) {
  EEPROM.get(CFG_INTERVALO_SCROLL_ADDR, intervaloScroll);
  EEPROM.get(CFG_UNIDADE_TEMP_ADDR, unidadeTemperatura);
  EEPROM.get(CFG_DISPLAY_ADDR, display);
  EEPROM.get(CFG_INTRO_ADDR, intro);
  EEPROM.get(CFG_ALTURA_ADDR, altura);
  EEPROM.get(CFG_FLAGALTURA_ADDR, flagAltura);
  EEPROM.get(CFG_FLAGHUM_ADDR, flagHum);
  EEPROM.get(CFG_FLAGCOOLDOWN_ADDR, flagCooldown);
  EEPROM.get(CFG_MODODISPLAY_ADDR, modoDisplay);
}

// Restaura as configurações de fábrica (grava valores padrão em PROGMEM na EEPROM)
void restaurarConfiguracoesDeFabrica() {
  for (int i = 0; i < variaveismutaveis; i++) {
    // Lê valor padrão do PROGMEM e grava na EEPROM (cada variável ocupa 2 bytes)
    int16_t valor = pgm_read_word_near(config_fac + i);
    EEPROM.put(i * 2, valor);
  }
  // Limpa flags de eventos (área de 20 a 1000)
  limparEEPROMFlags();
  // Recarrega variáveis na RAM
  definevars();
  Serial.println("Configurações de fábrica restauradas!");
}

// Função que é chamada na primeira vez que o dispositivo liga (flag em endereço 1001)
void primeirosetup(void) {
  if (EEPROM.read(1001) != 1) {
    // Se indicador não estiver marcado, configura ponteiro de flags e restaura fábrica
    enderecoEEPROM = ENDERECO_INICIAL_FLAGS;
    EEPROM.put(1010, enderecoEEPROM); // Salva ponteiro inicial na posição 1010
    restaurarConfiguracoesDeFabrica();
    EEPROM.write(1001, 1); // Marca que o setup inicial já foi feito
  }
}

//====================== TEXTOS E DESCRIÇÕES PARA MENU ======================
// São armazenados em PROGMEM para economizar RAM

const char texto0[] PROGMEM = "*-----Menu-----*";
const char texto1[] PROGMEM = "1. Leitura      ";
const char texto2[] PROGMEM = "2. Configuracao ";
const char texto3[] PROGMEM = "3. Logs         ";
const char texto4[] PROGMEM = "*---Configs---* ";
const char texto5[] PROGMEM = "1.Veloc.Txt     ";
const char texto6[] PROGMEM = "2.Unidade Temp. ";
const char texto7[] PROGMEM = "3.UTC           ";
const char texto8[] PROGMEM = "4.Reset         ";
const char texto9[] PROGMEM = "5.Intro         ";
const char texto10[] PROGMEM = "6.Modo Display ";
const char texto11[] PROGMEM = "7.Altura       ";
const char texto12[] PROGMEM = "8.Flag Altura  ";
const char texto13[] PROGMEM = "6.Flag Humid.  ";
const char texto14[] PROGMEM = "7.Flag Cooldown";
const char texto15[] PROGMEM = "*----Logs----* ";
const char texto16[] PROGMEM = "1.Print Log    ";
const char texto17[] PROGMEM = "2.Limpar Flag  ";

const char* const texto[] PROGMEM = {
  texto0, texto1, texto2, texto3,
  texto4, texto5, texto6, texto7,
  texto8, texto9, texto10, texto11,
  texto12, texto13, texto14, texto15,
  texto16, texto17
};

const char descricoes0[] PROGMEM = "Navegue pelas paginas e selecione no teclado - A. Subir - B. Descer - C. Voltar menu - D. Enter   ";
const char descricoes1[] PROGMEM = "Entra no modo leitura de dados e mostra na tela   ";
const char descricoes2[] PROGMEM = "Configura os parametros do dispositivo   ";
const char descricoes3[] PROGMEM = "Entra das opcoes de log - output na Serial   ";
const char descricoes4[] PROGMEM = "Modifica as configuracoes do dispositivo aperte D para dar entrada   ";
const char descricoes5[] PROGMEM = "Altera a velocidade da rolagem do texto   ";
const char descricoes6[] PROGMEM = "Altera a unidade de medida da temperatura - 1.Celsius - 2.Fahrenheit    ";
const char descricoes7[] PROGMEM = "Altera o fuso horario - ";
const char descricoes8[] PROGMEM = "Redefine para as configuracoes de fabrica    ";
const char descricoes9[] PROGMEM = "Liga ou desliga a intro ao ligar - 0. Desliga - 1.Liga   ";
const char descricoes10[] PROGMEM = "Modifica o modo como as informacoes sao apresentadas - 0 alt modos - 1 modo nivel - 2 modo info ";
const char descricoes11[] PROGMEM = "Modifica o valor(cm) da altura que o dispositivo esta em relacao ao solo - default 500 cm ";
const char descricoes12[] PROGMEM = "Modifica o valor critico da nivel da agua - default 360 cm ";
const char descricoes13[] PROGMEM = "Modifica o valor de log da leitura da humidade(%) - default 70%  ";
const char descricoes14[] PROGMEM = "Muda o intervalo de tempo(Em minutos) do registro das flags- default 1 minuto  ";
const char descricoes15[] PROGMEM = "Sessao dos Logs das Flags - plota no monitor serial as informacoes armazenadas  ";
const char descricoes16[] PROGMEM = "Plota no monitor serial o debug do dispositivo  ";
const char descricoes17[] PROGMEM = "Limpa as Flags da EEPROM - Cabem 140 flags no total  ";

const char* const descricoes[] PROGMEM = {
  descricoes0, descricoes1, descricoes2,
  descricoes3, descricoes4, descricoes5,
  descricoes6, descricoes7, descricoes8,
  descricoes9, descricoes10, descricoes11,
  descricoes12, descricoes13, descricoes14,
  descricoes15, descricoes16, descricoes17
};

//====================== CARACTERES CUSTOMIZADOS PARA ÍCONES ======================
// Cada array representa um caractere 5x8 usado para desenhar ícones (sol, gota, níveis, etc.)
const uint8_t customChars0[] PROGMEM = {0x00};
const uint8_t customChars1[] PROGMEM = {0x10};
// Gota 1 - lado esquerdo
const uint8_t customChars2[] PROGMEM = {0x00,0x00,0x1F,0x11,0x0A,0x04,0x00,0x00};
// Gota 1 - lado direito
const uint8_t customChars3[] PROGMEM = {0x00,0x00,0x1F,0x1F,0x0E,0x04,0x00,0x00};
// Gota 2 - lado esquerdo
const uint8_t customChars4[] PROGMEM = {0x00,0x00,0x04,0x0A,0x11,0x1F,0x00,0x00};
// Gota 2 - lado direito
const uint8_t customChars5[] PROGMEM = {0x00,0x00,0x04,0x0E,0x1F,0x1F,0x00,0x00};
// Sol 1 - lado esquerdo
const uint8_t customChars6[] PROGMEM = {0x00,0x03,0x0F,0x0F,0x0F,0x0F,0x03,0x00};
// Sol 1 - lado direito
const uint8_t customChars7[] PROGMEM = {0x00,0x18,0x1E,0x1E,0x1E,0x1E,0x18,0x00};
// Sol 2 - lado esquerdo
const uint8_t customChars8[] PROGMEM = {0x00,0x03,0x0C,0x08,0x08,0x0C,0x03,0x00};
// Sol 2 - lado direito
const uint8_t customChars9[] PROGMEM = {0x00,0x18,0x06,0x02,0x02,0x06,0x18,0x00};
// Sol 3 - lado esquerdo
const uint8_t customChars10[] PROGMEM = {0x15,0x93,0x0C,0x08,0x08,0x0C,0x93,0x15};
// Sol 3 - lado direito
const uint8_t customChars11[] PROGMEM = {0x14,0x19,0x06,0x02,0x02,0x06,0x19,0x14};
// Gota - lado esquerdo (usada em Display2)
const uint8_t customChars12[] PROGMEM = { 0x01, 0x02, 0x04, 0x09, 0x08, 0x04, 0x03, 0x00 };
// Gota - lado direito (usada em Display2)
const uint8_t customChars13[] PROGMEM = { 0x10, 0x08, 0x04, 0x02, 0x02, 0x04, 0x18, 0x00 };
// Gota 2 - lado esquerdo (usada em Display2)
const uint8_t customChars14[] PROGMEM = { 0x01, 0x02, 0x04, 0x0D, 0x0F, 0x07, 0x03, 0x00 };
// Gota 2 - lado direito (usada em Display2)
const uint8_t customChars15[] PROGMEM = { 0x10, 0x18, 0x1C, 0x1E, 0x1E, 0x1C, 0x18, 0x00 };
// Ícone de temperatura (lado esquerdo)
const uint8_t customChars16[] PROGMEM = { 0x04, 0x15, 0x0E, 0x04, 0x04, 0x0E, 0x15, 0x04 };
// Ícone de temperatura (lado direito)
const uint8_t customChars17[] PROGMEM = { 0x04, 0x15, 0x0E, 0x04, 0x04, 0x0E, 0x15, 0x04 };
// Ícone de alerta de temperatura (lado esquerdo)
const uint8_t customChars18[] PROGMEM = { 0x04, 0x0C, 0x0A, 0x06, 0x04, 0x0C, 0x0E, 0x04 };
// Ícone de alerta de temperatura (lado direito)
const uint8_t customChars19[] PROGMEM = { 0x04, 0x06, 0x0A, 0x0C, 0x04, 0x06, 0x1C, 0x04 };
// Níveis de barra para Display1 (0 a 4)
const uint8_t level1[] PROGMEM = { B00010000, B00010000, B00010000, B00010000, B00010000, B00010000, B00010000, B00010000 };
const uint8_t level2[] PROGMEM = { B00011000, B00011000, B00011000, B00011000, B00011000, B00011000, B00011000, B00011000 };
const uint8_t level3[] PROGMEM = { B00011100, B00011100, B00011100, B00011100, B00011100, B00011100, B00011100, B00011100 };
const uint8_t level4[] PROGMEM = { B00011110, B00011110, B00011110, B00011110, B00011110, B00011110, B00011110, B00011110 };

// Array para agrupar todos os ponteiros para caracteres customizados
const uint8_t* const customChars[] PROGMEM = {
  customChars0, customChars1, customChars2,
  customChars3, customChars4, customChars5,
  customChars6, customChars7, customChars8,
  customChars9, customChars10, customChars11,
  customChars12, customChars13, customChars14,
  customChars15, customChars16, customChars17,
  customChars18, customChars19, level1, level2, level3, level4
};

//====================== FUNÇÕES AUXILIARES INICIAIS ======================

// Inicializa sensores, LCD, teclado e RTC
void begins(void) {
  dht.begin();        // Inicia sensor DHT
  lcd.init();         // Inicializa o LCD
  lcd.backlight();    // Ativa a iluminação de fundo do LCD
  kpd.setDebounceTime(5); // Ajusta tempo de debounce do teclado (5 ms)
  rtc.begin();        // Inicia comunicação com o RTC DS1307
  // A linha abaixo ajusta o RTC para a data/hora de compilação, se necessário:
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

// Imprime texto de índice idxtexto (do array 'texto') na linha superior do LCD, rolando caractere a caractere
void print16(int idxtexto) {
  char buffert[17];
  // Copia a string do PROGMEM para um buffer local
  strcpy_P(buffert, (PGM_P)pgm_read_word(&(texto[idxtexto])));

  lcd.setCursor(0,0);
  // Percorre cada caractere do texto e exibe com atraso (rolagem)
  for (int i = 0; i < strlen(buffert); i++) {
    lcd.setCursor(i, 0);
    lcd.print(buffert[i]);
    delay(velocidade); 
  }
}

// Exibe as setas de direção no canto direito do LCD, dependendo do modo indicado
void printSetas(int modoseta) {
  uint8_t buffer[8];
  // Carrega ícone de seta para baixo (índice 2 no customChars)
  memcpy_P(buffer, (uint8_t*)pgm_read_word(&(customChars[2])), 8);
  lcd.createChar(0, buffer);
  // Carrega ícone de seta para cima (índice 4 no customChars)
  memcpy_P(buffer, (uint8_t*)pgm_read_word(&(customChars[4])), 8);
  lcd.createChar(1, buffer);

  // Exibe setas na posição certa:
  if (modoseta == SETABAIXO) {
    // Seta para baixo na linha 2, coluna 15
    lcd.setCursor(15, 1);
    lcd.write(byte(0));
  } else if (modoseta == SETACIMA) {
    // Seta para cima na linha 1, coluna 15
    lcd.setCursor(15, 0);
    lcd.write(byte(1));
  } else {
    // Exibe as duas setas (cima/baixo)
    lcd.setCursor(15, 1);
    lcd.write(byte(0));
    lcd.setCursor(15, 0);
    lcd.write(byte(1));
  }
}

// Mostra a descrição da opção de menu rolando texto na linha inferior, capturando setas e botões
int descricoesFunc(int idxdescricoes, int modoseta) {
  int cursor = 0;
  unsigned long ultimaAtualizacao = 0;
  int letras = 0;
  char textorolante[16];            // Buffer de 15 caracteres + terminador
  char buffer_desc_func[250];       // Buffer grande para copiar a descrição completa
  uint8_t buffer_chars_desc_func[8];

  // Copia string do PROGMEM para buffer_de_desc_func
  strcpy_P(buffer_desc_func, (PGM_P)pgm_read_word(&(descricoes[idxdescricoes])));
  printSetas(modoseta);             // Exibe setas (se houver)

  // Inicializa o buffer de rolagem com os primeiros 15 caracteres da descrição
  for (int i = 0; i < 15; i++) {
    textorolante[i] = buffer_desc_func[letras++];
  }

  // Enquanto houver caracteres restantes na descrição:
  while (letras < strlen(buffer_desc_func)) {
    char tecla = kpd.getKey();      // Verifica se alguma tecla foi pressionada

    // Se pressionou 'B' (descer), exibe ícone de seleção e retorna código 2
    if (tecla == 'B' && (modoseta != SETACIMA || modoseta == SETAS)) {
      memcpy_P(buffer_chars_desc_func, (uint8_t*)pgm_read_word(&(customChars[3])), 8);
      lcd.createChar(3, buffer_chars_desc_func);
      lcd.setCursor(15, 1);
      lcd.write(byte(3));
      return 2;
    // Se pressionou 'A' (subir), exibe ícone de seleção e retorna código 3
    } else if (tecla == 'A' && (modoseta != SETABAIXO || modoseta == SETAS)) {
      memcpy_P(buffer_chars_desc_func, (uint8_t*)pgm_read_word(&(customChars[5])), 8);
      lcd.createChar(3, buffer_chars_desc_func);
      lcd.setCursor(15, 0);
      lcd.write(byte(3));
      return 3;
    // Se pressionou 'D' (enter), retorna código 4
    } else if (tecla == 'D') {
      return 4;
    // Se pressionou 'C' (voltar), retorna código 5
    } else if (tecla == 'C') return 5;

    // Se já passou o tempo definido para rolar, atualiza a linha inferior
    if (millis() - ultimaAtualizacao > intervaloScroll) {
      ultimaAtualizacao = millis();
      // Exibe os primeiros 15 caracteres do buffer rolante
      for (int p = 0; p < 15; p++) {
        lcd.setCursor(p, 1);
        lcd.print(textorolante[p]);
      }
      // Desloca todos à esquerda, descartando o primeiro caractere
      for (int o = 0; o < 14; o++) {
        textorolante[o] = textorolante[o + 1];
      }
      // Coloca próximo caractere da descrição no final do buffer rolante
      textorolante[14] = buffer_desc_func[letras++];
      textorolante[15] = '\0';
    }
  }
  // Quando terminar de rolar toda a descrição, retorna código 1 (aguardando ação)
  return 1;
}

// Função para capturar valor numérico de 1 a 8 dígitos do teclado: retorna valor validado ou valor atual se 'C' for pressionado
int modoInput(int valorAtual, int minimo, int maximo) {
  lcd.clear();
  // Exibe faixa de valores possíveis na segunda linha
  lcd.setCursor(1, 1);
  lcd.print(minimo);
  lcd.print("--");
  lcd.print(maximo);

  lcd.setCursor(0, 0);
  char key = '\0';                  // Armazenará tecla pressionada
  char numChar[9] = "";          // Buffer para até 8 dígitos mais sinal
  int digitosvalidos = 0;

  // Enquanto o usuário não confirmar com 'D'
  while (key != 'D') {
    key = kpd.getKey();
    // Se a tecla for dígito (0-9) ou '-' (no primeiro dígito) e não exceder limite de 8 caracteres:
    if ((key >= '0' && key <= '9' && digitosvalidos < 8) || (key == '-' && digitosvalidos == 0)) {
      numChar[digitosvalidos] = key;
      lcd.setCursor(digitosvalidos, 0);
      lcd.print(numChar[digitosvalidos++]);
    // Se pressionou 'C', cancela e retorna valor anterior
    } else if (key == 'C') {
      return valorAtual;
    }
  }

  numChar[digitosvalidos] = '\0';   // Fecha string com terminador
  int num = atoi(numChar);          // Converte string para inteiro

  // Se valor fora dos limites, exibe mensagem e chama recursivamente para nova entrada
  if (num < minimo || num > maximo) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Valor invalido");
    delay(1000);
    return modoInput(valorAtual, minimo, maximo);
  } else {
    // Caso válido, retorna o valor digitado
    return num;
  }
}

// Função para exibir o menu e capturar navegação/seleção
int menus(int i, int b, int filho) {
  print16(i);           // Exibe título do menu, rolando texto na linha superior
  do {
    opcao = descricoesFunc(i, b); // Exibe descrição e aguarda ação

    if (opcao == 2) {        
      menuatual++;   // Se usuário pressionar descer, avança para próxima opção no menu
      break;
    } else if (opcao == 3) {
      menuatual--;   // Se usuário pressionar subir, volta para opção anterior
      break;
    } else if (opcao == 4) {  
      // Se usuário pressionar 'D' (confirma), verifica se há submenu (filho != 0)
      if (filho != 0) {
        // Efeito piscando backlight do LCD ao entrar em submenu
        lcd.noBacklight();
        delay(250);
        lcd.backlight();
        menuatual = filho; // Define qual submenu entrar
      }
      return 4; // Indica seleção confirmada
    } else if (opcao == 5) {
      menuatual = 0; // Se usuário pressionar 'C', retorna ao menu principal
      break;
    }
  } while (opcao == 1); // Enquanto estiver apenas rolando descrição (sem tecla de navegação), continua no loop
  return 0;
}

// Função para apagar todas as flags de evento da EEPROM (endereços 20 a 1000)
void limparEEPROMFlags() {
  for (int i = 20; i <= 1000; i++) {
    EEPROM.update(i, 0xFF);  // 0xFF significa célula apagada
  }
  // Reseta ponteiro de gravação de flags para início (20)
  EEPROM.put(1010, ENDERECO_INICIAL_FLAGS);
  Serial.println("EEPROM de 20 até 1000 limpa com sucesso.");
}

//====================== FUNÇÃO DE MONITORAMENTO E GRAVAÇÃO DE FLAGS ======================
void monitoramentoDisplay() {
  unsigned long timerPrint   = millis(); // Controle para atualizar display
  unsigned long timerFlag    = millis(); // Controle para cooldown de flags (não implementado detalhadamente)
  unsigned long timerSensor  = millis(); // Controle não utilizado neste trecho
  unsigned long timerDisplay = millis(); // Controle para alternar entre modos de display

  bool mostrandoDisplay1 = true; // Controla qual modo de display está ativo (1 ou 2)
  bool eventoAtivo = false;      // Indica se um evento (chuva + nível alto) está em andamento
  uint32_t timestampInicio = 0;
  uint32_t timestampFim = 0;
  uint16_t picoNivel = 0;        // Pico de nível de água registrado durante evento

  // Loop de monitoramento contínuo (até usuário pressionar 'C')
  while (true) {
    // Verifica se tecla 'C' foi pressionada para sair do modo de monitoramento
    char tecla = kpd.getKey();
    if (tecla == 'C') {
      // Antes de sair, grava ponteiro atual de gravação de flags na EEPROM
      EEPROM.put(1010, enderecoEEPROM);
      lcd.clear();
      menuatual = 0; // Retorna ao menu principal
      return;
    }

    // Leitura do sensor DHT (temperatura e umidade)
    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();
    // Se leitura inválida, exibe erro e reinicia loop
    if (isnan(temp) || isnan(hum)) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Erro no sensor");
      delay(2000);
      continue;
    }

    // Calcula leitura de nível de água usando sensor ultrassônico (função abaixo)
    uint16_t nivelAtual = leituraDaAgua();
    // Lógica de exibição de modos de display (0=alternar automático, 1=sempre modoDisplay1, 2=modoDisplay2)
    if (!modoDisplay) {
      // Muda automaticamente entre displays a cada 15 segundos
      if (millis() - timerDisplay >= 15000) {
        timerDisplay = millis();
        mostrandoDisplay1 = !mostrandoDisplay1;
        lcd.clear();
      }
      // Atualiza display a cada 500 ms
      if (millis() - timerPrint >= 500) {
        timerPrint = millis();
        if (mostrandoDisplay1) {
          // ModoDisplay1: barra de nível
          modoDisplay1(nivelAtual, 0, 500);
        } else {
          // ModoDisplay2: exibe chuva, nível, umidade e temperatura
          modoDisplay2(nivelAtual, hum, temp);
        }
      }
    } else if (modoDisplay == 1) {
      // Sempre exibe modoDisplay1 a cada 500 ms
      if (millis() - timerPrint >= 500) {
        timerPrint = millis();
        modoDisplay1(nivelAtual, 0, 500);
      }
    } else if (modoDisplay == 2) {
      // Sempre exibe modoDisplay2 a cada 500 ms, limpando tela antes
      if (millis() - timerPrint >= 500) {
        timerPrint = millis();
        lcd.clear();
        modoDisplay2(nivelAtual, hum, temp);
      }
    }

    // Verifica estado do sensor de chuva (HIGH = chuva detectada, pois está com pull-up)
    bool chovendo = (digitalRead(CHUVA) == HIGH);
    // Captura timestamp atual (segundos desde 1970) do RTC
    uint32_t agora = rtc.now().unixtime();

    // Se evento não está ativo, verifica condição para iniciá-lo (nível acima do flag + chuva)
    if (!eventoAtivo) {
      if (nivelAtual > flagAltura && chovendo) {
        eventoAtivo = true;
        timestampInicio = agora;
        picoNivel = nivelAtual;
        Serial.println(">> Evento iniciado!");
        Serial.print("Hora de inicio: "); Serial.println(timestampInicio);
      }
    } else { // Se evento está ativo, atualiza pico e verifica condição para finalização
      if (nivelAtual > picoNivel) {
        picoNivel = nivelAtual; // Atualiza pico de nível observado
      }
      // Se chuva cessou e nível caiu abaixo do flag, finaliza evento
      if (!chovendo && nivelAtual <= flagAltura) {
        eventoAtivo = false;
        timestampFim = agora;
        Serial.println(">> Evento finalizado!");
        Serial.print("Hora de inicio: "); Serial.println(timestampInicio);
        Serial.print("Hora de fim: "); Serial.println(timestampFim);
        Serial.print("Pico do nivel: "); Serial.println(picoNivel);

        // Grava os dados do evento (início, fim, pico) na EEPROM, se houver espaço
        if (enderecoEEPROM + 10 <= 1000) { // Cada evento ocupa 10 bytes: 4+4+2
          EEPROM.put(enderecoEEPROM, timestampInicio); enderecoEEPROM += 4;
          EEPROM.put(enderecoEEPROM, timestampFim);    enderecoEEPROM += 4;
          EEPROM.put(enderecoEEPROM, picoNivel);       enderecoEEPROM += 2;

          Serial.println(">> FLAG ESPECIAL gravada na EEPROM!");
          Serial.print("Total Flags Evento: ");
          Serial.println(((enderecoEEPROM - ENDERECO_INICIAL_FLAGS) / 10));
        } else {
          Serial.println("!!! EEPROM cheia, não foi possível gravar evento.");
        }
      }
    }
  }
}

//====================== FUNÇÃO DE LEITURA ULTRASSÔNICA (NÍVEL DE ÁGUA) ======================
int leituraDaAgua(void) {
  uint16_t distancia;
  // Gatilha do sensor ultrassônico: envia pulso alto por 10 microssegundos
  digitalWrite(4, HIGH);
  delayMicroseconds(10);
  digitalWrite(4, LOW);
  // Captura tempo de resposta do eco em microssegundos, divide por 58 para converter em cm
  distancia = pulseIn(3, HIGH) / 58;
  // Retorna diferença entre altura do dispositivo e distância até a água (calcula nível)
  return altura - distancia;
}

//====================== FUNÇÃO DE DEBUG DE EEPROM (IMPRIME TODAS AS CONFIGURAÇÕES E FLAGS) ======================
void debugEEPROM() {
  Serial.println("===== DEBUG EEPROM =====");

  int16_t val;
  Serial.print("Scroll: "); Serial.println(intervaloScroll);
  EEPROM.get(CFG_UNIDADE_TEMP_ADDR, val);
  Serial.print("Unidade Temp: "); Serial.println(val);
  EEPROM.get(CFG_DISPLAY_ADDR, val);
  Serial.print("Display: "); Serial.println(val);
  EEPROM.get(CFG_INTRO_ADDR, val);
  Serial.print("Intro: "); Serial.println(val);
  EEPROM.get(CFG_ALTURA_ADDR, val);
  Serial.print("Altura do dispositivo(cm): "); Serial.println(altura);
  EEPROM.get(CFG_FLAGALTURA_ADDR, val);
  Serial.print("Nivel crítico da água(cm): "); Serial.println(flagAltura);
  EEPROM.get(CFG_FLAGHUM_ADDR, val);
  Serial.print("Flag humidade: "); Serial.println(flagHum);
  EEPROM.get(CFG_FLAGCOOLDOWN_ADDR, val);
  Serial.print("Flag cooldown: "); Serial.println(flagCooldown);
  EEPROM.get(1010, val);
  Serial.print("Endereco Eeprom: "); Serial.println(val);

  Serial.println("\n===== FLAGS DE EVENTO SALVAS =====");
  int endereco = ENDERECO_INICIAL_FLAGS;
  int flagCount = 0;

  // Enquanto houver espaço para ler mais um evento (10 bytes por evento)
  while (endereco + 10 <= 1000) {
    uint32_t timestampInicio;
    uint32_t timestampFim;
    uint16_t picoNivel;

    // Lê dados do evento na ordem: início (4 bytes), fim (4 bytes), pico (2 bytes)
    EEPROM.get(endereco, timestampInicio); endereco += 4;
    EEPROM.get(endereco, timestampFim);    endereco += 4;
    EEPROM.get(endereco, picoNivel);       endereco += 2;

    // Verifica se o evento lido é plausível (ano válido, pico < 501)
    DateTime dtInicio(timestampInicio);
    uint16_t yearInicio = dtInicio.year();
    if (yearInicio == 0xFFFF || yearInicio == 0 || yearInicio > 3000 || picoNivel >= 501) break;

    // Exibe informações do evento no Serial Monitor
    Serial.print("[FLAG "); Serial.print(++flagCount); Serial.println("]");

    Serial.print("Início: ");
    Serial.print(yearInicio); Serial.print("-");
    Serial.print(dtInicio.month()); Serial.print("-");
    Serial.print(dtInicio.day()); Serial.print(" ");
    Serial.print(dtInicio.hour()); Serial.print(":");
    Serial.print(dtInicio.minute()); Serial.print(":");
    Serial.println(dtInicio.second());

    DateTime dtFim(timestampFim);
    Serial.print("Fim: ");
    Serial.print(dtFim.year()); Serial.print("-");
    Serial.print(dtFim.month()); Serial.print("-");
    Serial.print(dtFim.day()); Serial.print(" ");
    Serial.print(dtFim.hour()); Serial.print(":");
    Serial.print(dtFim.minute()); Serial.print(":");
    Serial.println(dtFim.second());

    Serial.print("Pico do nível: ");
    Serial.print(picoNivel);
    Serial.println(" cm");

    Serial.println("--------------------------");
  }

  if (flagCount == 0) Serial.println("Nenhuma flag registrada.");
}

//====================== MODO DE EXIBIÇÃO 1 (BARRA DE NÍVEL) ======================
// Desenha uma barra de 16 colunas que indica o nível de água relativo entre vmin e vmax
void modoDisplay1(int valor, int vmin, int vmax) {
  uint8_t buffer[8];
  // Carrega caracteres customizados para níveis (índices 20 a 23 em customChars)
  memcpy_P(buffer, (uint8_t*)pgm_read_word(&(customChars[20])), 8);
  lcd.createChar(1, buffer);
  memcpy_P(buffer, (uint8_t*)pgm_read_word(&(customChars[21])), 8);
  lcd.createChar(2, buffer);
  memcpy_P(buffer, (uint8_t*)pgm_read_word(&(customChars[22])), 8);
  lcd.createChar(3, buffer);
  memcpy_P(buffer, (uint8_t*)pgm_read_word(&(customChars[23])), 8);
  lcd.createChar(4, buffer);
  memcpy_P(buffer, (uint8_t*)pgm_read_word(&(customChars[2])), 8);
  lcd.createChar(5, buffer);

  int slide[16] = {};
  uint8_t leituraSlide;
  uint8_t idx = 0;

  // Exibe texto estático na linha 0
  lcd.setCursor(0, 0);
  lcd.print("Nivel:             ");
  // Coloca marcador de nível crítico (flagAltura) no topo, transformando 0..80 em 0..16 colunas
  lcd.setCursor((map(flagAltura, vmin, vmax, 1, 80) / 5), 0);
  lcd.write(byte(5));

  // Mapeia valor atual para 0..80 e depois para índice 0..16 (índice de bloco completo ou parciais)
  leituraSlide = map(valor, vmin, vmax, 1, 80);
  idx = leituraSlide / 5;

  // Preenche vetor slide[]: posições < idx são blocos cheios, posição == idx é bloco parcial, > idx é vazio
  for (int i = 0; i < 16; i++) {
    if (i < idx) {
      slide[i] = 5;      // bloco cheio (caractere 255)
    } else if (i == idx) {
      slide[i] = leituraSlide % 5; // bloco parcial (1,2,3 ou 4)
    } else {
      slide[i] = 0;      // espaço vazio
    }
  }

  // Desenha a linha inferior: cada coluna usa caractere customizado conforme vetor slide[]
  for (int i = 0; i < 16; i++) {
    lcd.setCursor(i, 1);
    if (slide[i] == 5) {
      lcd.write(byte(255)); // bloco de barra completa
    } else if (slide[i] == 0) {
      lcd.print(' ');       // espaço vazio
    } else {
      lcd.write(slide[i]); // bloco parcial (índice 1..4)
    }
  }
}

//====================== MODO DE EXIBIÇÃO 2 (INFORMAÇÕES BÁSICAS) ======================
// Exibe resumo: chuva ou seco, nível em cm, umidade e temperatura com ícones
void modoDisplay2(uint16_t nivelAgua, uint8_t hum, uint8_t temp) {
  uint8_t spriteL[8], spriteR[8];
  // Linha 0: exibe "CHUVA" ou "SECO"
  lcd.setCursor(0, 0);
  lcd.print(digitalRead(CHUVA) == HIGH ? "CHUVA" : "SECO");

  // Linha 1 (coluna 0): exibe nível da água em cm
  lcd.setCursor(0, 1);
  lcd.print(nivelAgua);
  lcd.print("cm");

  // Exibe umidade e ícone de gota: se abaixo do limite, usa gota transparente, caso contrário, gota preenchida
  lcd.setCursor(6, 0);
  lcd.print(hum, 1);
  lcd.print('%');
  lcd.setCursor(6, 1);
  lcd.print("H:");
  if (hum <= flagHum) {
    memcpy_P(spriteL, (uint8_t*)pgm_read_word(&(customChars[12])), 8);
    memcpy_P(spriteR, (uint8_t*)pgm_read_word(&(customChars[13])), 8);
  } else {
    memcpy_P(spriteL, (uint8_t*)pgm_read_word(&(customChars[14])), 8);
    memcpy_P(spriteR, (uint8_t*)pgm_read_word(&(customChars[15])), 8);
  }
  lcd.createChar(6, spriteL);
  lcd.createChar(7, spriteR);
  lcd.setCursor(8, 1);
  lcd.write(byte(6)); // Lado esquerdo da gota
  lcd.setCursor(9, 1);
  lcd.write(byte(7)); // Lado direito da gota

  // Exibe temperatura e ícone de termômetro: converte para Fahrenheit se necessário
  lcd.setCursor(11, 0);
  lcd.print(unidadeTemperatura == 2 ? temp * 1.8 + 32 : temp, 1);
  lcd.print(unidadeTemperatura == 2 ? 'F' : 'C');
  lcd.setCursor(12, 1);
  lcd.print("T:");
  if (temp <= 30) {
    memcpy_P(spriteL, (uint8_t*)pgm_read_word(&(customChars[16])), 8);
    memcpy_P(spriteR, (uint8_t*)pgm_read_word(&(customChars[17])), 8);
  } else {
    memcpy_P(spriteL, (uint8_t*)pgm_read_word(&(customChars[18])), 8);
    memcpy_P(spriteR, (uint8_t*)pgm_read_word(&(customChars[19])), 8);
  }
  lcd.createChar(1, spriteL);
  lcd.createChar(2, spriteR);
  lcd.setCursor(14, 1);
  lcd.write(byte(1)); // Ícone lado esquerdo
  lcd.setCursor(15, 1);
  lcd.write(byte(2)); // Ícone lado direito
}

//====================== SETUP E LOOP PRINCIPAIS ======================

void setup() {
  definevars();        // Carrega configurações da EEPROM para RAM
  begins();            // Inicializa sensores, LCD, teclado e RTC
  if (intro)          // Se a opção de intro estiver habilitada
    anim_executar_inicializacao(); // Executa animação de inicialização
  Serial.begin(9600);  // Inicia comunicação serial para debug
  primeirosetup();     // Executa setup inicial (restaura fábrica em primeira execução)
  pinMode(CHUVA, INPUT_PULLUP); // Define pino do sensor de chuva como entrada com pull-up
  pinMode(4, OUTPUT);             // Pino 4 usado como trigger do sensor ultrassônico
  pinMode(3, INPUT);              // Pino 3 usado como echo do sensor ultrassônico
}

void loop() {
  // Switch de acordo com menuatual para navegar pelas opções
  switch (menuatual) {
    case 1:  // Menu Principal → 1. Leitura/Monitoramento (Chama modoMonitoramento)
      menus(1, SETAS, 99);
      break;

    case 2:  // Menu Principal → 2. Configuração (entra em submenu 4)
      menus(2, SETAS, 4);
      break;

    case 3:  // Menu Principal → 3. Sobre/Logs (entra em submenu 15)
      menus(3, SETAS, 15);
      break;

    case 4:  // Submenu Configurações (título de submenu)
      menus(4, SETAS, 0);
      break;

    case 5:  // Configuração → 1. Velocidade de Texto
      menus(5, SETAS, 100);
      break;

    case 6:  // Configuração → 2. Unidade de Temperatura
      menus(6, SETAS, 101);
      break;

    case 7:  // Configuração → 3. Modo de Display
      menus(7, SETAS, 102);
      break;

    case 8:  // Configuração → 4. Reset de Fábrica
      menus(8, SETAS, 103);
      break;

    case 9:  // Configuração → 5. Intro Lig/Deslig
      menus(9, SETAS, 104);
      break;

    case 10: // Configuração → 6. Setup LDR (não implementado texto específico)
      menus(10, SETAS, 105);
      break;

    case 11: // Configuração → 7. Limite Luminosidade Flag (não implementado texto específico)
      menus(11, SETAS, 106);
      break;

    case 12: // Configuração → 8. Limite Temperatura Flag (não implementado texto específico)
      menus(12, SETAS, 107);
      break;

    case 13: // Configuração → 9. Limite Umidade Flag (não implementado texto específico)
      menus(13, SETAS, 108);
      break;

    case 14: // Configuração → 10. Cooldown entre Flags (não implementado texto específico)
      menus(14, SETAS, 109);
      break;

    case 15: // Submenu de Logs (sem ações diretas)
      menus(15, SETAS, 0);
      break;

    case 16: // Logs → opção 1: Debug EEPROM (ativa submenu 98)
      menus(16, SETAS, 98);
      break;

    case 17: // Logs → opção 2: Limpar Flags (ativa submenu 97)
      menus(17, SETAS, 97);
      break;

    case 97: // Ação: Limpar Flags (chama função, retorna para menu 17)
      limparEEPROMFlags();
      menuatual = 17;
      break;

    case 98: // Ação: Debug EEPROM (chama função e volta para menu 16)
      debugEEPROM();
      menuatual = 16;
      break;

    case 99: // Ação: Monitoramento / Leitura de sensores (entra em monitoramentoDisplay)
      EEPROM.get(1010, enderecoEEPROM); // Carrega ponteiro atual de flags
      monitoramentoDisplay();
      break;

    case 100: // Configuração de Velocidade de Texto (captura valor e salva)
      intervaloScroll = modoInput(intervaloScroll, 100, 2000);
      EEPROM.put(CFG_INTERVALO_SCROLL_ADDR, intervaloScroll);
      menuatual = 5;
      break;

    case 101: // Configuração de Unidade de Temperatura (1 ou 2)
      unidadeTemperatura = modoInput(unidadeTemperatura, 1, 2);
      EEPROM.put(CFG_UNIDADE_TEMP_ADDR, unidadeTemperatura);
      menuatual = 6;
      break;

    case 102: // Configuração de Display (valor -12 a 12, uso não detalhado)
      display = modoInput(display, -12, 12);
      EEPROM.put(CFG_DISPLAY_ADDR, display);
      menuatual = 7;
      break;

    case 103: // Executa Reset de Fábrica (restaura padrões)
      restaurarConfiguracoesDeFabrica();
      definevars();
      menuatual = 8;
      break;

    case 104: // Habilita/desabilita Intro na inicialização (0 ou 1)
      intro = modoInput(intro, 0, 1);
      EEPROM.put(CFG_INTRO_ADDR, intro);
      menuatual = 9;
      break;

    case 105: // Seleciona modo de display (0, 1 ou 2)
      modoDisplay = modoInput(modoDisplay, 0, 2);
      EEPROM.put(CFG_MODODISPLAY_ADDR, modoDisplay);
      menuatual = 10;
      break;

    case 106: // Configura altura do dispositivo em cm (0 a 1000)
      altura = modoInput(altura, 0, 1000);
      EEPROM.put(CFG_ALTURA_ADDR, altura);
      menuatual = 11;
      break;

    case 107: // Configura nível crítico da água (flagAltura) em cm (0 a 999)
      flagAltura = modoInput(flagAltura, 0, 999);
      EEPROM.put(CFG_FLAGALTURA_ADDR, flagAltura);
      menuatual = 12;
      break;

    case 108: // Configura limite de umidade para alerta (0 a 100)
      flagHum = modoInput(flagHum, 0, 100);
      EEPROM.put(CFG_FLAGHUM_ADDR, flagHum);
      menuatual = 13;
      break;

    case 109: // Configura tempo de cooldown mínimo entre flags (1 a 60 segundos)
      flagCooldown = modoInput(flagCooldown, 1, 60);
      EEPROM.put(CFG_FLAGCOOLDOWN_ADDR, flagCooldown);
      menuatual = 14;
      break;

    default: // Menu inicial: exibe menu principal (índice 0)
      menuatual = 0;
      menus(0, SETABAIXO, 0);
  }
}
