# INFO
Essa pasta contém os datasets gerados a partir do *_Mapa de Edificações da Unicamp_*. Os datasets estão em formato de **ESRI shapefiles**. Além das informações sobre os prédios (eg., id nome, unidade, geometria), também foi adicionada um coluna com o ponto central de cada prédio (centroid).

### Prerequisites
jupyter, geopandas, shapely, matplotlib

### Pastas
* _unicamp_edificios_sirgas2000_: dataset de edificações no CRS SIRGAS 2000 / UTM ZONE 23
* _unicamp_edificios_wgs84_: dataset de edificações no CRS WGS 84

### Arquivos
* _create-geodatasets_: notebook usado para gerar os datasets.
* _mapa_unicamp_edificios_e_centroids_: plot do mapa de edificações com os seus respectivos centroids.

