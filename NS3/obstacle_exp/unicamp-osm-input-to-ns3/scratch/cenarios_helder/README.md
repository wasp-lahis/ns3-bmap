# Simulando Cenários - Helder 2019

Para simular o cenário de nó em movimento pelo Campus da Unicamp (Cenário 2 - Teste 1), descrito em ***Análise de Desempenho de
Gateway LoRa/LoRaWAN com Dispositivos IoT no SmartCampus***, é necessário seguir as configurações da seções seguintes.

## Configurações para a simulação

Para conseguir executar a simulação, siga os passos:

- Configurar a **FREQ regional** do módulo LoRaWAN para 915 MHZ, conforme descrito em [**+info**](https://github.com/wasp-lahis/ns3-bmap/tree/main/NS3/lorawan-module-classes);
- Configurar **modelo de obstáculo**, descrito em [**+info**](https://github.com/wasp-lahis/ns3-bmap/tree/main/NS3/obstacle_exp/obstacle-module);
- Colocar os datasets de entrada (***rssi_pos_dataset.csv*** e ***predios_unicamp_dataset.xml***) e código da simualação (***simulation-helder-cenarios.cc***) no diretório base do NS-3. Os datasets estão disponíveis em [**+info**](https://github.com/wasp-lahis/ns3-bmap/tree/main/NS3/obstacle_exp/unicamp-osm-input-to-ns3);
- Criar estrutura de pastas para armazenar os resultados dos diferentes modelos de propagação:

```shell
cd $NS3-BASE-DIR
mkdir simulation_results
cd simulation_results
mkdir log_model
mkdir log_obstacle_model
mkdir okumura_model
```

A estrutura deve ficar assim:

``` bash
NS3-BASE-DIR/simulation_results/
├── log_model/
├── log_obstacle_model/
└── pokumura_model/
```

## Executando simulação:

Realizado os passos de configuração da seção anterior, para executar o código, insira:

```shell
cd NS3_BASE_DIR
./waf --run "simulation-helder-cenarios --exp_name=C2T1 --channel_cenario=1 --simu_repeat=1"
```

onde:

* **_--exp_name_**: é o nome do experimento;
* **_--channel_cenario_**: é o cenário de propagação do canal LoRa:
	* (1) Log Distance
	* (2) Log Distance + Obstacle Model
	* (3) Okumura-Hata
* **_--simu_repeat_**: quantidade de vezes que a simualação é repetida



## Executando simulação - Todos os cenários de propagação

Realizado os passos de configuração da seção anterior, para executar o código com todos os cenários de propagação, insira:

```shell
cd NS3_BASE_DIR
./run_cenario_C2T1.sh
```