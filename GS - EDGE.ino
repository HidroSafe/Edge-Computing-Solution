//====================== BIBLIOTECAS ======================
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include "DHT.h"
#include <RTClib.h>

 
const int velocidade = 20;  
short int menuatual = 0, opcao = 0;
 
#define SETABAIXO 1
#define SETACIMA 2
#define SETAS 0

#define CHUVA 2

#define DHTpin 5
#define DHTmodel DHT22
DHT dht(DHTpin, DHTmodel);

#define LDR A0
 
RTC_DS1307 rtc;
 
LiquidCrystal_I2C lcd(0x27, 16, 2);
 
const uint8_t ROWS = 4;
const uint8_t COLS = 4;
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '-', 'D' }
};
uint8_t colPins[COLS] = { 9, 8, 7, 6 };     
uint8_t rowPins[ROWS] = { 13, 12, 11, 10 }; 
 
Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
 
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

unsigned long anim_tempo_ultimo_quadro = 0;
const int anim_intervalo_principal = 320;
int anim_pose_atual_aranha = 0;

enum anim_estado_sistema { anim_entrando_e_puxando, anim_texto_centralizado_segurando, anim_soltando_fio, anim_aranha_saindo, anim_texto_final_exibido };
anim_estado_sistema anim_estado_atual = anim_entrando_e_puxando;

const char* anim_nome_empresa = "HidroSafe";
const int anim_comprimento_nome_empresa = 9;
const char* anim_linha_decorativa_texto = "---------";
const int anim_comprimento_linha_decorativa = 9;
const int anim_colunas_lcd = 16;
int anim_coluna_atual_esquerda_aranha = -(anim_comprimento_nome_empresa + 3);
int anim_coluna_atual_inicio_texto;
int anim_coluna_atual_fio;
const int anim_coluna_alvo_inicio_texto = (anim_colunas_lcd - anim_comprimento_nome_empresa) / 2;
const int anim_coluna_alvo_inicio_linha_decorativa = (anim_colunas_lcd - anim_comprimento_linha_decorativa) / 2;
unsigned long anim_tempo_mudanca_estado = 0;
const int anim_duracao_estado_segurando = 1500;

void anim_carregar_sprites_pose(byte pose) {
  for (byte parte = 0; parte < 4; parte++) {
    uint8_t buffer[8];
    memcpy_P(buffer, anim_aranha[pose][parte], 8);
    lcd.createChar(parte, buffer);
  }
}

void anim_desenhar_aranha(byte col_esq) {
  if (col_esq >= 0 && col_esq < anim_colunas_lcd) {
    lcd.setCursor(col_esq, 0); lcd.write(0);
    lcd.setCursor(col_esq, 1); lcd.write(1);
  }
  if ((col_esq + 1) >= 0 && (col_esq + 1) < anim_colunas_lcd) {
    lcd.setCursor(col_esq + 1, 0); lcd.write(2);
    lcd.setCursor(col_esq + 1, 1); lcd.write(3);
  }
}

void anim_desenhar_texto() {
  for (int i = 0; i < anim_comprimento_nome_empresa; i++) {
    int col = anim_coluna_atual_inicio_texto + i;
    if (col >= 0 && col < anim_colunas_lcd)
      lcd.setCursor(col, 0), lcd.print(anim_nome_empresa[i]);
  }
  if (anim_estado_atual >= anim_texto_centralizado_segurando) {
    for (int i = 0; i < anim_comprimento_linha_decorativa; i++) {
      int col = anim_coluna_alvo_inicio_linha_decorativa + i;
      if (col >= 0 && col < anim_colunas_lcd)
        lcd.setCursor(col, 1), lcd.print(anim_linha_decorativa_texto[i]);
    }
  }
}

void anim_executar_inicializacao() {
  anim_estado_atual = anim_entrando_e_puxando;
  anim_coluna_atual_esquerda_aranha = -(anim_comprimento_nome_empresa + 3);
  anim_tempo_ultimo_quadro = millis();

  while (anim_estado_atual != anim_texto_final_exibido) {
    if (millis() - anim_tempo_ultimo_quadro >= anim_intervalo_principal) {
      lcd.clear();
      anim_tempo_ultimo_quadro = millis();
      anim_pose_atual_aranha = 1 - anim_pose_atual_aranha;
      anim_carregar_sprites_pose(anim_pose_atual_aranha);

      switch (anim_estado_atual) {
        case anim_entrando_e_puxando:
          anim_coluna_atual_esquerda_aranha++;
          anim_coluna_atual_fio = anim_coluna_atual_esquerda_aranha - 1;
          anim_coluna_atual_inicio_texto = anim_coluna_atual_fio - anim_comprimento_nome_empresa;
          if (anim_coluna_atual_inicio_texto >= anim_coluna_alvo_inicio_texto) {
            anim_coluna_atual_inicio_texto = anim_coluna_alvo_inicio_texto;
            anim_estado_atual = anim_texto_centralizado_segurando;
            anim_tempo_mudanca_estado = millis();
          }
          break;
        case anim_texto_centralizado_segurando:
          if (millis() - anim_tempo_mudanca_estado > anim_duracao_estado_segurando)
            anim_estado_atual = anim_soltando_fio;
          break;
        case anim_soltando_fio:
          anim_estado_atual = anim_aranha_saindo;
          break;
        case anim_aranha_saindo:
          anim_coluna_atual_esquerda_aranha++;
          if (anim_coluna_atual_esquerda_aranha >= anim_colunas_lcd) {
            anim_estado_atual = anim_texto_final_exibido;
            lcd.clear();
            anim_desenhar_texto();
            delay(1000);
          }
          break;
        default:
          break;
      }

      if (anim_estado_atual != anim_texto_final_exibido) {
        if (anim_coluna_atual_fio >= 0 && anim_coluna_atual_fio < anim_colunas_lcd &&
            anim_estado_atual <= anim_texto_centralizado_segurando) {
          lcd.setCursor(anim_coluna_atual_fio, 0);
          lcd.print("-");
        }
        anim_desenhar_aranha(anim_coluna_atual_esquerda_aranha);
        anim_desenhar_texto();
      }
    }
    delay(10);
  }

  lcd.clear();
}

#define CFG_INTERVALO_SCROLL_ADDR 0  
#define CFG_UNIDADE_TEMP_ADDR     2 
#define CFG_DISPLAY_ADDR          4  
#define CFG_INTRO_ADDR            6 
#define CFG_ALTURA_ADDR           8
#define CFG_FLAGALTURA_ADDR      10
#define CFG_FLAGHUM_ADDR         12
#define CFG_FLAGCOOLDOWN_ADDR    14
#define CFG_MODODISPLAY_ADDR  16 
#define EEPROM_LUZ_MAX_ADDR      18 
#define ENDERECO_INICIAL_FLAGS   20
int enderecoEEPROM;
 
const uint16_t config_fac[] PROGMEM = {
  800,  
  1,   
  -3,    
  0,
  500,
  360,
  30,
  70,
  0  
};
 
const uint8_t variaveismutaveis = 9; 
 
uint16_t intervaloScroll;
uint16_t unidadeTemperatura;
int16_t display;
uint16_t intro;
uint16_t altura;
uint16_t flagAltura;
uint16_t flagHum;
uint16_t flagCooldown;
uint16_t modoDisplay;

void definevars(void){
  EEPROM.get(CFG_INTERVALO_SCROLL_ADDR, intervaloScroll);
  EEPROM.get(CFG_UNIDADE_TEMP_ADDR, unidadeTemperatura);
  EEPROM.get(CFG_DISPLAY_ADDR, display);
  EEPROM.get(CFG_INTRO_ADDR, intro);
  EEPROM.get(CFG_ALTURA_ADDR,altura);
  EEPROM.get(CFG_FLAGALTURA_ADDR,flagAltura);
  EEPROM.get(CFG_FLAGHUM_ADDR,flagHum);
  EEPROM.get(CFG_FLAGCOOLDOWN_ADDR,flagCooldown);
  EEPROM.get(CFG_MODODISPLAY_ADDR,modoDisplay);
}
 
void restaurarConfiguracoesDeFabrica() {
  for (int i = 0; i < variaveismutaveis; i++) {
    int16_t valor = pgm_read_word_near(config_fac + i);
    EEPROM.put(i * 2, valor);
  }
  limparEEPROMFlags();
  definevars();
  Serial.println("Configurações de fábrica restauradas!");
}
 
void primeirosetup(void){
    if (EEPROM.read(1001) != 1) {
    enderecoEEPROM = ENDERECO_INICIAL_FLAGS;
    EEPROM.put(1010,enderecoEEPROM);
    restaurarConfiguracoesDeFabrica();  
    EEPROM.write(1001, 1); 
  }
}
 
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
  texto0, texto1, texto2, 
  texto3, texto4, texto5,
  texto6, texto7, texto8, 
  texto9, texto10, texto11,
  texto12, texto13, texto14,
  texto15, texto16, texto17
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
 
const uint8_t customChars0[] PROGMEM = {0x00};
const uint8_t customChars1[] PROGMEM = {0x10};
const uint8_t customChars2[] PROGMEM = {0x00,0x00,0x1F,0x11,0x0A,0x04,0x00,0x00};      
const uint8_t customChars3[] PROGMEM = {0x00,0x00,0x1F,0x1F,0x0E,0x04,0x00,0x00};     
const uint8_t customChars4[] PROGMEM = {0x00,0x00,0x04,0x0A,0x11,0x1F,0x00,0x00};      
const uint8_t customChars5[] PROGMEM = {0x00,0x00,0x04,0x0E,0x1F,0x1F,0x00,0x00};   
const uint8_t customChars6[] PROGMEM = {0x00,0x03,0x0F,0x0F,0x0F,0x0F,0x03,0x00};  // Sol 1 - lado esquerdo
const uint8_t customChars7[] PROGMEM = {0x00,0x18,0x1E,0x1E,0x1E,0x1E,0x18,0x00};  // Sol 1 - lado direito
const uint8_t customChars8[] PROGMEM = {0x00,0x03,0x0C,0x08,0x08,0x0C,0x03,0x00};  // Sol 2 - lado esquerdo
const uint8_t customChars9[] PROGMEM = {0x00,0x18,0x06,0x02,0x02,0x06,0x18,0x00};  // Sol 2 - lado direito
const uint8_t customChars10[] PROGMEM = {0x15,0x93,0x0C,0x08,0x08,0x0C,0x93,0x15}; // Sol 3 - lado esquerdo
const uint8_t customChars11[] PROGMEM = {0x14,0x19,0x06,0x02,0x02,0x06,0x19,0x14}; // Sol 3 - lado direito
const uint8_t customChars12[] PROGMEM = { 0x01, 0x02, 0x04, 0x09, 0x08, 0x04, 0x03, 0x00 }; // Gota - lado esquerdo
const uint8_t customChars13[] PROGMEM = { 0x10, 0x08, 0x04, 0x02, 0x02, 0x04, 0x18, 0x00 }; // Gota - lado direito
const uint8_t customChars14[] PROGMEM = { 0x01, 0x02, 0x04, 0x0D, 0x0F, 0x07, 0x03, 0x00 }; // Gota 2 - lado esquerdo
const uint8_t customChars15[] PROGMEM = { 0x10, 0x18, 0x1C, 0x1E, 0x1E, 0x1C, 0x18, 0x00 }; // Gota 2 - lado direito
const uint8_t customChars16[] PROGMEM = { 0x04, 0x15, 0x0E, 0x04, 0x04, 0x0E, 0x15, 0x04 }; // lado esquerdo
const uint8_t customChars17[] PROGMEM = { 0x04, 0x15, 0x0E, 0x04, 0x04, 0x0E, 0x15, 0x04 }; // lado direito
const uint8_t customChars18[] PROGMEM = { 0x04, 0x0C, 0x0A, 0x06, 0x04, 0x0C, 0x0E, 0x04 }; // lado esquerdo
const uint8_t customChars19[] PROGMEM = { 0x04, 0x06, 0x0A, 0x0C, 0x04, 0x06, 0x1C, 0x04 }; // lado direito
const uint8_t level1[] PROGMEM = { B00010000, B00010000, B00010000, B00010000, B00010000, B00010000, B00010000, B00010000 };
const uint8_t level2[] PROGMEM = { B00011000, B00011000, B00011000, B00011000, B00011000, B00011000, B00011000, B00011000 };
const uint8_t level3[] PROGMEM = { B00011100, B00011100, B00011100, B00011100, B00011100, B00011100, B00011100, B00011100 };
const uint8_t level4[] PROGMEM = { B00011110, B00011110, B00011110, B00011110, B00011110, B00011110, B00011110, B00011110 };



const uint8_t* const customChars[] PROGMEM = {
  customChars0, customChars1, customChars2,
  customChars3, customChars4, customChars5,
  customChars6, customChars7, customChars8,
  customChars9, customChars10, customChars11,
  customChars12, customChars13, customChars14,
  customChars15, customChars16, customChars17,
  customChars18, customChars19, level1, level2, level3, level4

};
 
void begins(void){
  dht.begin();
  lcd.init();      
  lcd.backlight(); 
  kpd.setDebounceTime(5);
  rtc.begin();
 // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}
 
void print16(int idxtexto){
  char buffert[17];
  strcpy_P(buffert, (PGM_P)pgm_read_word(&(texto[idxtexto])));
 
  lcd.setCursor(0,0);
  for(int i = 0; i < strlen(buffert); i++){
    lcd.setCursor(i,0);
    lcd.print(buffert[i]);
    delay(velocidade); 
  }
}
 
void printSetas(int modoseta){
  uint8_t buffer[8]; 
 
  memcpy_P(buffer, (uint8_t*)pgm_read_word(&(customChars[2])), 8);
  lcd.createChar(0, buffer);

  memcpy_P(buffer, (uint8_t*)pgm_read_word(&(customChars[4])), 8);
  lcd.createChar(1, buffer);
 
  if(modoseta == SETABAIXO){ 
    lcd.setCursor(15,1);
    lcd.write(byte(0)); 
  } else if(modoseta == SETACIMA){ 
    lcd.setCursor(15,0);
    lcd.write(byte(1)); 
  } else{ 
    lcd.setCursor(15,1);
    lcd.write(byte(0));
    lcd.setCursor(15,0);
    lcd.write(byte(1));
  }
}
 
int descricoesFunc(int idxdescricoes,int modoseta){
  int cursor = 0;
  unsigned long ultimaAtualizacao = 0;
  int letras = 0;
  char textorolante[16];
  char buffer_desc_func[250]; 
  uint8_t buffer_chars_desc_func[8]; 
  strcpy_P(buffer_desc_func, (PGM_P)pgm_read_word(&(descricoes[idxdescricoes])));
  printSetas(modoseta); 
 
  for(int i = 0; i < 15; i++){
    textorolante[i] = buffer_desc_func[letras++];
  }
 
  while (letras < strlen(buffer_desc_func)) {
    char tecla = kpd.getKey();
   
    if (tecla == 'B' && (modoseta != SETACIMA || modoseta == SETAS) ) {
      memcpy_P(buffer_chars_desc_func, (uint8_t*)pgm_read_word(&(customChars[3])), 8); 
      lcd.createChar(3,buffer_chars_desc_func); 
      lcd.setCursor(15,1);
      lcd.write(byte(3));
      return 2;
    } else if (tecla == 'A' && (modoseta != SETABAIXO || modoseta == SETAS) ) {
      memcpy_P(buffer_chars_desc_func, (uint8_t*)pgm_read_word(&(customChars[5])), 8); 
      lcd.createChar(3,buffer_chars_desc_func); 
      lcd.setCursor(15,0);
      lcd.write(byte(3)); 
      return 3;
    } else if (tecla == 'D'){
      return 4;
    } else if (tecla == 'C')return 5;
 
    if (millis() - ultimaAtualizacao > intervaloScroll) {
      ultimaAtualizacao = millis();
      for (int p = 0; p < 15; p++) {
        lcd.setCursor(p, 1);
        lcd.print(textorolante[p]);
      }
      for (int o = 0; o < 14; o++) {
        textorolante[o] = textorolante[o + 1];
      }
      textorolante[14] = buffer_desc_func[letras++];
      textorolante[15] = '\0';
    }
  }
  return 1;
}
 
int modoInput(int valorAtual, int minimo, int maximo) {
  lcd.clear();
  lcd.setCursor(1, 1);
  lcd.print(minimo);
  lcd.print("--");
  lcd.print(maximo);
 
  lcd.setCursor(0, 0);
  char key = '\0';
  char numChar[9] = "";
  int digitosvalidos = 0;
 
  while (key != 'D') {
    key = kpd.getKey();
    if ((key >= '0' && key <= '9' && digitosvalidos < 8) || (key == '-' && digitosvalidos == 0)) {
      numChar[digitosvalidos] = key;
      lcd.setCursor(digitosvalidos, 0);
      lcd.print(numChar[digitosvalidos++]);
    } else if (key == 'C') {
      return valorAtual;  
    }
  }
 
  numChar[digitosvalidos] = '\0';
  int num = atoi(numChar);
 
  if (num < minimo || num > maximo) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Valor invalido");
    delay(1000);
    return modoInput(valorAtual, minimo, maximo);
  } else {

    return num;
  }
}
 
int menus(int i, int b, int filho) {
  print16(i); 
  do {
    opcao = descricoesFunc(i, b);

    if (opcao == 2) { 
      menuatual++;
      break;
    } else if (opcao == 3) { 
      menuatual--;
      break;
    } else if (opcao == 4) { 
      if (filho != 0) { 
        lcd.noBacklight();  
        delay(250);
        lcd.backlight();    
        menuatual = filho; 
      }
      return 4; 
    } else if (opcao == 5) { 
      menuatual = 0; 
      break;
    }
  } while (opcao == 1);
  return 0;
}

void limparEEPROMFlags() {
  for (int i = 20; i <= 1000; i++) {
    EEPROM.update(i, 0xFF);  // padrão de memória "apagada"
  }
  EEPROM.put(1010,ENDERECO_INICIAL_FLAGS);  // reseta o ponteiro de escrita
  Serial.println("EEPROM de 20 até 1000 limpa com sucesso.");
}
 


void monitoramentoDisplay() {
  unsigned long timerPrint = millis();
  unsigned long timerFlag = millis();
  unsigned long timerSensor = millis();
  unsigned long timerDisplay = millis();
 
  
  bool mostrandoDisplay1 = true;
  bool eventoAtivo = false;
  uint32_t timestampInicio = 0;
  uint32_t timestampFim = 0;
  uint16_t picoNivel = 0;
  while (true) {
    // Verifica tecla de saída

    char tecla = kpd.getKey();
    if (tecla == 'C') {
      EEPROM.put(1010,enderecoEEPROM);
      lcd.clear();
      menuatual = 0;
      return;
    }

    // Leitura dos sensores
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    if (isnan(temp) || isnan(hum)) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Erro no sensor");
      delay(2000);
      continue;
    }
    
    // Output do display
    uint16_t nivelAtual = leituraDaAgua();
    if (!modoDisplay) {
        if (millis() - timerDisplay >= 15000) {
            timerDisplay = millis();
            mostrandoDisplay1 = !mostrandoDisplay1; // troca de display
            lcd.clear(); // limpa tela ao trocar de display
        }

        if (millis() - timerPrint >= 500) {
            timerPrint = millis();
            if (mostrandoDisplay1) {
                modoDisplay1(nivelAtual, 0, 500);
            } else {
                modoDisplay2(nivelAtual, hum, temp);
            }
        }
    } else if (modoDisplay == 1) {
        if (millis() - timerPrint >= 500) {
            timerPrint = millis();
            modoDisplay1(nivelAtual, 0, 500);
        }
    } else if (modoDisplay == 2) {
        if (millis() - timerPrint >= 500) {
            timerPrint = millis();
            lcd.clear();
            modoDisplay2(nivelAtual, hum, temp);
        }
    }
   

    bool chovendo = (digitalRead(CHUVA) == HIGH); // LOW se sensor de chuva é pullup
    uint32_t agora = rtc.now().unixtime();

    // Evento NÃO está ativo → verifica se deve iniciar
    if (!eventoAtivo) {
        if (nivelAtual > flagAltura && chovendo) {
            eventoAtivo = true;
            timestampInicio = agora;
            picoNivel = nivelAtual;
            Serial.println(">> Evento iniciado!");
            Serial.print("Hora de inicio: "); Serial.println(timestampInicio);
        }
    } else { // Evento ATIVO → monitora
        // Atualiza pico
        if (nivelAtual > picoNivel) {
            picoNivel = nivelAtual;
        }

        // Verifica se deve finalizar
        if (!chovendo && nivelAtual <= flagAltura) {
            eventoAtivo = false;
            timestampFim = agora;
            Serial.println(">> Evento finalizado!");
            Serial.print("Hora de inicio: "); Serial.println(timestampInicio);
            Serial.print("Hora de fim: "); Serial.println(timestampFim);
            Serial.print("Pico do nivel: "); Serial.println(picoNivel);

            // ==== Grava na EEPROM ====
            if (enderecoEEPROM + 10 <= 1000) { // 4 + 4 + 2 = 10 (coloquei margem p/ segurança)
                // Grava inicio
                EEPROM.put(enderecoEEPROM, timestampInicio); enderecoEEPROM += 4;
                // Grava fim
                EEPROM.put(enderecoEEPROM, timestampFim); enderecoEEPROM += 4;
                // Grava pico
                EEPROM.put(enderecoEEPROM, picoNivel); enderecoEEPROM += 2;

                Serial.println(">> FLAG ESPECIAL gravada na EEPROM!");
                Serial.print("Total Flags Evento: ");
                Serial.println(((enderecoEEPROM - 20) / 10));
            } else {
                Serial.println("!!! EEPROM cheia, não foi possível gravar evento.");
            }
        }
    }
  }
}

int leituraDaAgua(void){
 uint16_t distancia;
 
 digitalWrite(4, HIGH);
 delayMicroseconds(10);
 digitalWrite(4, LOW);
 distancia = pulseIn(3,HIGH)/58;

 return altura - distancia;
}


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
Serial.print("Flag cooldown: "); Serial.println(val);
EEPROM.get(1010, val);
Serial.print("Endereco Eeprom: "); Serial.println(val);

Serial.println("\n===== FLAGS DE EVENTO SALVAS =====");
int endereco = ENDERECO_INICIAL_FLAGS;

int flagCount = 0;
while (endereco + 10 <= 1000) { // lê enquanto houver espaço para 1 flag completa
    uint32_t timestampInicio;
    uint32_t timestampFim;
    uint16_t picoNivel;

    // Lê a flag
    EEPROM.get(endereco, timestampInicio); endereco += 4;
    EEPROM.get(endereco, timestampFim);    endereco += 4;
    EEPROM.get(endereco, picoNivel);       endereco += 2;

    // Verifica se a flag é válida (ano plausível)
    DateTime dtInicio(timestampInicio);
    uint16_t yearInicio = dtInicio.year();

    if (yearInicio == 0xFFFF || yearInicio == 0 || yearInicio > 3000 || picoNivel >= 501) break;

    // Exibe a flag
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

if (flagCount == 0)Serial.println("Nenhuma flag registrada.");
}

void modoDisplay1(int valor, int vmin, int vmax){
  uint8_t buffer[8];
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
  lcd.setCursor(0,0);
  lcd.print("Nivel:             ");  
  lcd.setCursor((map(flagAltura,vmin, vmax, 1, 80)/5),0);
  lcd.write(byte(5));

  leituraSlide = map(valor, vmin, vmax, 1, 80);
  idx = leituraSlide / 5;
  // Atualiza o vetor slide[]
  for (int i = 0; i < 16; i++) {
    if (i < idx) {
      slide[i] = 5; 
    } else if (i == idx) {
      slide[i] = leituraSlide % 5;
    } else {
      slide[i] = 0; // espaço vazio
    }
  }

  // Desenha barra na linha 1
  for (int i = 0; i < 16; i++) {
    lcd.setCursor(i, 1);
  
    if (slide[i] == 5) {
      lcd.write(byte(255)); // bloco cheio
    } else if (slide[i] == 0) {
      lcd.print(' ');       // espaço vazio
    } else {
      lcd.write(slide[i]);  // slots 1..4
    }
  

  }
  
}


void modoDisplay2(uint16_t nivelAgua, uint8_t hum, uint8_t temp){
  uint8_t spriteL[8],spriteR[8];
  lcd.setCursor(0,0);
  lcd.print(digitalRead(CHUVA) == HIGH? "CHUVA":"SECO");

  lcd.setCursor(0,1);
  lcd.print(nivelAgua);
  lcd.print("cm");


  lcd.setCursor(6,0); lcd.print(hum, 1);
  lcd.print('%');
  lcd.setCursor(6,1); lcd.print("H:");
  if (hum <= flagHum) {
    memcpy_P(spriteL, (uint8_t*)pgm_read_word(&(customChars[12])), 8);
    memcpy_P(spriteR, (uint8_t*)pgm_read_word(&(customChars[13])), 8);
  } else {
    memcpy_P(spriteL, (uint8_t*)pgm_read_word(&(customChars[14])), 8);
    memcpy_P(spriteR, (uint8_t*)pgm_read_word(&(customChars[15])), 8);
  }
  lcd.createChar(6, spriteL);
  lcd.createChar(7, spriteR);
  lcd.setCursor(8, 1); lcd.write(byte(6));
  lcd.setCursor(9, 1); lcd.write(byte(7));


  lcd.setCursor(11,0); lcd.print(unidadeTemperatura == 2?temp * 1.8 + 32:temp, 1);
  lcd.print(unidadeTemperatura == 2?'F':'C');
  lcd.setCursor(12,1); lcd.print("T:");
  if (temp <= 30) {
    memcpy_P(spriteL, (uint8_t*)pgm_read_word(&(customChars[16])), 8);
    memcpy_P(spriteR, (uint8_t*)pgm_read_word(&(customChars[17])), 8);
  } else {
    memcpy_P(spriteL, (uint8_t*)pgm_read_word(&(customChars[18])), 8);
    memcpy_P(spriteR, (uint8_t*)pgm_read_word(&(customChars[19])), 8);
  }
  lcd.createChar(1, spriteL);
  lcd.createChar(2, spriteR);
  lcd.setCursor(14, 1); lcd.write(byte(1));
  lcd.setCursor(15, 1); lcd.write(byte(2));

}


void setup() {

  definevars();
  begins(); 
  if(intro)anim_executar_inicializacao();
  Serial.begin(9600); 
  primeirosetup();
  pinMode(CHUVA, INPUT_PULLUP);
  pinMode(4, OUTPUT);
  pinMode(3, INPUT);
}
 
void loop() {

  switch(menuatual){

    case 1:  // Menu Principal → 1. Display
      menus(1, SETAS, 99); break;

    case 2:  // Menu Principal → 2. Setup
      menus(2, SETAS, 4); break;

    case 3:  // Menu Principal → 3. Sobre
      menus(3, SETAS, 15); break;

    case 4:  // Submenu de Setup
      menus(4, SETAS, 0); break;

    case 5:  // Setup → 1. Velocidade de Texto
      menus(5, SETAS, 100); break;

    case 6:  // Setup → 2. Unidade de Temperatura
      menus(6, SETAS, 101); break;

    case 7:  // Setup → 3. Modo de Display
      menus(7, SETAS, 102); break;

    case 8:  // Setup → 4. Reset de Fábrica
      menus(8, SETAS, 103); break;

    case 9:  // Setup → 5. Intro Lig/Deslig
      menus(9, SETAS, 104); break;

    case 10: // Setup → 6. Setup LDR
      menus(10, SETAS, 105); break;

    case 11: // Setup → 7. Limite Luminosidade Flag
      menus(11, SETAS, 106); break;

    case 12: // Setup → 8. Limite Temperatura Flag
      menus(12, SETAS, 107); break;

    case 13: // Setup → 9. Limite Umidade Flag
      menus(13, SETAS, 108); break;

    case 14: // Setup → 10. Cooldown entre Flags
      menus(14, SETAS, 109); break;
    
    case 15:
      menus(15, SETAS, 0);break;
    
    case 16:
      menus(16, SETAS, 98);break;
    
    case 17:
      menus(17, SETAS, 97);break;

    case 97:
      limparEEPROMFlags();menuatual = 17;break;

    case 98: // Comando Especial → Debug EEPROM
      debugEEPROM();menuatual = 16; break;

    case 99: // Comando Especial → Monitoramento
      EEPROM.get(1010,enderecoEEPROM);monitoramentoDisplay(); break;

    case 100: // Input → Velocidade de rolagem
      intervaloScroll = modoInput(intervaloScroll, 100, 2000);
      EEPROM.put(CFG_INTERVALO_SCROLL_ADDR, intervaloScroll);
      menuatual = 5; break;

    case 101: // Input → Unidade de Temperatura
      unidadeTemperatura = modoInput(unidadeTemperatura, 1, 2);
      EEPROM.put(CFG_UNIDADE_TEMP_ADDR, unidadeTemperatura);
      menuatual = 6; break;

    case 102: // Input → Tipo de Display
      display = modoInput(display, -12, 12);
      EEPROM.put(CFG_DISPLAY_ADDR, display);
      menuatual = 7; break;

    case 103: // Executa Reset de Fábrica
      restaurarConfiguracoesDeFabrica();
      definevars();
      menuatual = 8; break;

    case 104: // Input → Intro Ligado ou Desligado
      intro = modoInput(intro, 0, 1);
      EEPROM.put(CFG_INTRO_ADDR, intro);
      menuatual = 9; break;

    case 105: // Executa setup de Luz Mínima/Máxima
      modoDisplay = modoInput(modoDisplay, 0, 2);
      EEPROM.put(CFG_MODODISPLAY_ADDR, modoDisplay);
      menuatual = 10; break;

    case 106: // Input → Limite de Altura do dispositivo
      altura = modoInput(altura, 0, 1000);
      EEPROM.put(CFG_ALTURA_ADDR, altura);
      menuatual = 11; break;

    case 107: // Input → Nivel crítico da água
      flagAltura = modoInput(flagAltura, 0, 999);
      EEPROM.put(CFG_FLAGALTURA_ADDR, flagAltura);
      menuatual = 12; break;

    case 108: // Input → Limite de umidade para flag
      flagHum = modoInput(flagHum, 0, 100);
      EEPROM.put(CFG_FLAGHUM_ADDR, flagHum);
      menuatual = 13; break;

    case 109: // Input → Tempo de cooldown entre flags (ms)
      flagCooldown = modoInput(flagCooldown, 1, 60);
      EEPROM.put(CFG_FLAGCOOLDOWN_ADDR, flagCooldown);
      menuatual = 14; break;

    default: // Menu Inicial  
      menuatual = 0;
      menus(0, SETABAIXO, 0);
  }
}