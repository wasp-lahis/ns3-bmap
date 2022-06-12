# LoRa/LoRaWAN Simulations
LoRa/LoRaWAN Simulations descriptions.

## Simulations Common Parameters 
```bash
FREQ: 915 MHz
BW: 125 kHz
ED TX: 20 dBm
SF, CR: auto, 4/5
Modelo de propagação: Okumura + Nakagami
Período: 7 dias
Repetição de Simulação: 10x

```

## Simulations Different Parameters

- **01** (Marianna):
```
Baterias:
Intervalo de envio: 3x na semana (392h)
Tam do payload: 5 bytes
Num de nós: 47

Conteiners:
Intervalo de envio: 4x ao dia (6h)
Tam do payload: 31 bytes 
Num de nós: 175

Medidores Inteligentes:
Intervalo de envio: 15min
Tam do payload: 49 bytes 
Num de nós: 50

TOTAL DE PACOTES ENVIADOS: 220753
```

- **02** (Lahis):
```
Baterias:
Intervalo de envio: 3x na semana (56h)
Num de nós: 47
Pacotes esperados: 3 x 47 = 141

Conteiners:
Intervalo de envio: 4x ao dia (6h)
Num de nós: 175 
Pacotes esperados: 28 * 175 = 4900

Medidores Inteligentes:
Intervalo de envio: 15min
Num de nós: 321 
Pacotes esperados: 672 * 321 = 215712

Air monitoring:
Intervalo de envio: 10 segundos 
Tam do payload: 20 bytes 
Num de nós: 10
Pacotes esperados: 60480 * 10 = 604800

Indoor/Outdoor Localization:
Intervalo de envio: 60 segundos
Tam do payload: 32 bytes 
Num de nós: 10
Pacotes esperados: 10080 * 10 = 100800

TOTAL DE PACOTES ENVIADOS: 926353
```

