# INFO
LoRa/LoRaWAN Simulations descriptions.

## Simulations Common Parameters 
```bash
FREQ: 915 MHz
BW: 125 kHz
ED TX: 20 dBm
SF, CR: auto, 4/5
Nº de nós: 222
```



## Simulations Different Parametes

- **01**:
```
1 ano, 4 mensagens ao dia (6 em 6 horas) - Repetição 2x
Modelo de propagação: Okumura-Hata
Tam do payload: 11 bytes
Pacotes esperados: 324120
```
- **02**: 
```
1 mês, 48 mensagens ao dia (30 em 30 min) - Repetição 1x
Modelo de propagação: Okumura-Hata
Tam do payload: 11 bytes
Pacotes esperados: 319680
```

- **03**: 
```
1 mês, 483800 mensagens ao dia (10 em 10 min) - Repetição 1x
Modelo de propagação: Okumura-Hata
Tam do payload: 11 bytes
Pacotes esperados: 3222108000
```

- **04** (Marianna): 
```
Período: 1 mês
Repetição de Simulação: 10x
Modelo de propagação: Okumura-Hata

Intervalo de envio: 1-1h 
Tam do payload: 14 bytes (conteiner)

Intervalo de envio: 6-6h 
Tam do payload: 5 bytes (bateria)

Pacotes esperados: 126000 + 5640 = 131640
```

- **05** (Marianna): 
```
Período: 1 mês
Repetição de Simulação: 10x
Modelo de propagação: Okumura-Hata

Intervalo de envio: 10-10min 
Tam do payload: 14 bytes (conteiner)

Intervalo de envio: 30-30min
Tam do payload: 5 bytes (bateria)

Pacotes esperados: 756000 + 67680 = 823680
```


- **04-log** (Marianna): 
```
Período: 1 mês
Repetição de Simulação: 10x
Modelo de propagação: Log Distance

Intervalo de envio: 1-1h 
Tam do payload: 14 bytes (conteiner)

Intervalo de envio: 6-6h 
Tam do payload: 5 bytes (bateria)

Pacotes esperados: 126000 + 5640 = 131640
```

- **05-log** (Marianna): 
```
Período: 1 mês
Repetição de Simulação: 10x
Modelo de propagação: Log Distance

Intervalo de envio: 10-10min 
Tam do payload: 14 bytes (conteiner)

Intervalo de envio: 30-30min
Tam do payload: 5 bytes (bateria)

Pacotes esperados: 756000 + 67680 = 823680
```

- **04-cor** (Marianna): 
```
Período: 1 mês
Repetição de Simulação: 10x
Modelo de propagação: Log Distance + Correlated Shadowing

Intervalo de envio: 1-1h 
Tam do payload: 14 bytes (conteiner)

Intervalo de envio: 6-6h 
Tam do payload: 5 bytes (bateria)

Pacotes esperados: 126000 + 5640 = 131640
```

- **05-cor** (Marianna): 
```
Período: 1 mês
Repetição de Simulação: 10x
Modelo de propagação: Log Distance + Correlated Shadowing

Intervalo de envio: 10-10min 
Tam do payload: 14 bytes (conteiner)

Intervalo de envio: 30-30min
Tam do payload: 5 bytes (bateria)

Pacotes esperados: 756000 + 67680 = 823680
```

- **06**: 
```
Período: 1 mês
Repetição de Simulação: 10x
Modelo de propagação: Log Distance
Modelo de Energia: corrente radio, sensores e placa controladora

Intervalo de envio: 1-1h
Tam do payload: 31 bytes (conteiner)

Pacotes esperados: 756000 + 67680 = 823680
```

- **07**: 
```
Período: 1 mês
Repetição de Simulação: 10x
Modelo de propagação: Log Distance
Modelo de Energia: corrente radio, sensores e placa controladora, energy h.

Intervalo de envio: 1-1h
Tam do payload: 31 bytes (conteiner)

Pacotes esperados: 756000 + 67680 = 823680
```