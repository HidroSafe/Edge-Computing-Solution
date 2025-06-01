# HidroSafe

## Visão Geral

HidroSafe é um sistema embarcado projetado para monitorar nível de água, temperatura e umidade usando Arduino, com foco em registros de eventos de enchente e visualização dinâmica em LCD.

### Funcionalidades Principais

- **Monitoramento de Nível de Água:** Utiliza sensor ultrassônico para medir distância até a água e calcular o nível (cm).  
- **Sensor de Chuva e DHT22:** Detecta chuva (entrada digital) e lê temperatura/umidade.  
- **Registro de Eventos de Enchente:** Quando nível > limite configurado **e** detecta chuva, inicia um evento; encerra quando chuva cessa **e** nível recua. Cada evento grava:
  - Timestamp de início (RTC)
  - Timestamp de fim (RTC)
  - Pico de nível registrado
- **Persistência em EEPROM:** Configurações (rolagem de texto, unidades, limite de nível, limite de umidade, modo de display) e histórico de eventos são salvos em EEPROM, mantendo dados após desligamentos.  
- **Modos de Display Dinâmicos:**  
  1. **Barra de Nível (16 colunas):** Exibe gráfico proporcional ao nível, com marcação do limite crítico.  
  2. **Informações Completas:** Mostra estado “CHUVA/SECO”, nível em cm, umidade (com ícone de gota), temperatura (C/F, com ícone de termômetro).  
  3. **Alternância Automática:** Alterna entre os dois modos a cada 15 segundos, ou fixa um modo conforme configuração.

### Navegação por Menu

- **LCD 16×2 + Teclado Matricial 4×4** para:
  - Ajustar parâmetros (limites, unidades, velocidade de rolagem, habilitar/desabilitar animação introdutória).  
  - Acessar logs de eventos e depurar EEPROM.
  - Opções de reset de fábrica.

### Persistência de Configurações

- Parâmetros armazenados em endereços fixos na EEPROM:
  - Intervalo de rolagem de texto
  - Unidade de temperatura
  - Altura do sensor ultrassônico
  - Limite crítico de nível
  - Limite de umidade para alerta
  - Cooldown entre flags de evento
  - Modo de display (automático, fixo 1 ou fixo 2)
- Indicador de primeiro setup garante gravação de valores-padrão na primeira inicialização.

### Registro de Eventos na EEPROM

- Cada evento ocupa exatamente 10 bytes:
  1. 4 bytes – Timestamp de início (segundos desde 1970)  
  2. 4 bytes – Timestamp de fim  
  3. 2 bytes – Pico de nível (cm)  
- Ponteiro (4 bytes) indica próximo endereço livre; percorre de 20 até 1000 na EEPROM.  
- Função de depuração apresenta histórico de eventos via Serial.

## Diferenciais

- **Integração completa entre sensores (ultrassônico, DHT22 e chuva) e lógica de evento.**
- **Modos de display que alternam automaticamente ou exibem detalhes conforme necessidade.**
- **Persistência robusta em EEPROM, com estrutura clara de configuração e histórico de flags.**

## Colaboradores

- Luigi  
- Rogério

---

**Arquivo gerado automaticamente para facilitar implementação e documentação.**