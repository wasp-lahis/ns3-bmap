# Aquisição de dados - RSSI

O objetivo dos arquivos **.html** é fornecer as coordenas (latitude e longitude) dos locais mapeados em nossos datasets de Pilhas/Baterias e Conteiners. A partir da aquisição dos valores de RSSI da recepção de mensagens no Gateway, vindos a partir de cada um das posições dos mapas, poderemos verificar qual modelo de propagação é o mais realista ao analisar a cobertura LoRa no campus da Unicamp.


# Arquivos - Mapas

Ambos os arquivos **.html** contém todos os pontos dos datasets de Pilhas e Baterias e Conteiners:

- **_all_nodes_dataset.html_**: pontos em azul são do dataset de Pilhas e pontos em vermelho são dos datasets de Conteiners.
- **_cluster_dataset.html_**: nesse mapa, os pontos estão agrupados por clusters. Ao utilizar o zoom, todos os pontos podem ser visualizados individualmente ou em subclusters, a depender do nível de zoom aplicado.
- **_dataset_rssi_**: dataset que será montado a partir dos RSSI do gateway, vindos de pacotes enviados por um endnode posicionado em alguns pontos dos mapas dos **.html**. [Link para o dataset](https://docs.google.com/spreadsheets/d/1u8dD__Z0yxiDYEFKSOTMfDRplLIBHsaj-2D8jHnDvXU/edit?usp=sharing)

Todos os pontos nas ruas pertencem ao dataset de Conteiners. Já os pontos localizados nos prédios, pertecem ao dataset de Pilhas e Baterias.


### Outras informações:

Ambos os mapas contém:
```bash
Total de pontos: 123
Total de pontos no dataset de Conteiners: 76
Total de pontos no dataset de Pilhas e Baterias: 47

```