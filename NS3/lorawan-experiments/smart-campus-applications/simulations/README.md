# LoRa/LoRaWAN Simulations
LoRa/LoRaWAN Simulations descriptions.

## Simulations Common Parameters 
```bash
FREQ: 915 MHz
BW: 125 kHz
ED TX: 20 dBm
SF, CR: auto, 4/5
```

## Simulations Different Parametes

- **01**:
```
Período: 1 dia
Repetição de Simulação: 10x
Modelo de propagação: Log Distance

Baterias:
Intervalo de envio: 24h 
Tam do payload: 5 bytes
Num de nós: 47
Pacotes esperados: 1 * 47 = 47

Conteiners:
Intervalo de envio: 12 
Tam do payload: 31 bytes 
Num de nós: 175
Pacotes esperados: 2 * 175 = 350

Air monitoring:
Intervalo de envio: 72 segundos 
Tam do payload: 16 bytes 
Num de nós: 10
Pacotes esperados: 1200 * 10 = 12000

Air monitoring 2:
Intervalo de envio: 10 segundos 
Tam do payload: 20 bytes 
Num de nós: 10
Pacotes esperados: 8640 * 10 = 86400

Indoor/Outdoor Localization:
Intervalo de envio: 60 segundos
Tam do payload: 32 bytes 
Num de nós: 10
Pacotes esperados: 1440 * 10 = 14400


Total de pacotes esperados: 113197
```

- **02**:
```
Igual a simulação 01, exceto por:
Modelo de propagação: Okumura-Hata
```

- **03**:
```
Igual a simulação 01, exceto por:
Modelo de propagação: Log Distance + Correlated
```