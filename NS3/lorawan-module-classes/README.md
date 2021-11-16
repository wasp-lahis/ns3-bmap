
# Alterações no Módulo LoRaWAN

## Frenquência da Região (Austrália 915-928 MHZ)

Substituir os arquivos:
```bash
lorawan-mac-helper.cc
lorawan-mac-helper.h

lora-phy-helper-phy.cc
gateway-lora-phy.cc (debug)
```
Pelos respectivos arquivos desse diretório (model/ ou helper/ do módulo LoRaWAN). Essas classes foram baseadas na especificação ***RP002-1.0.3 LoRaWAN® Regional Parameters 2021*** também presente nesse diretório.


## TX power

Para alterar o TX para 20 Dbm por ex, substituir os valores 14 Dbm por 20 Dbm em:
```bash
lorawan-mac-helper.cc
class-A-end-device-lorawan-mac.cc
end-device-lorawan-mac.cc
```

