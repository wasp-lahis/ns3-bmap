# ns3-bmap

O objetivo deste repositório é:

- Levantar as principais tecnologias de manipulação de mapas/geoprocessamento;
- Testar as melhores alternativas, extraindo do mapa de interesse (Unicamp) as dimensões dos prédios em metros;
- Implementar a integração de prédios gerados com NS3 (GridBuildingAllocator) para estudo posterior de efeitos dos mesmos no modelo de propagação do canal.


## Teoria e Alguns Mapas

### Teoria - Prédios 3D
- Simple 3D Buildings: https://wiki.openstreetmap.org/wiki/Simple_3D_Buildings
- Slippy map tilenames: 
	- https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
	- https://stackoverflow.com/questions/28476117/easy-openstreetmap-tile-displaying-for-python


### Mapas Unicamp:
- Mapa da evolução territorial das edificações por período: https://www.arcgis.com/home/webmap/viewer.html?webmap=e00e99c1ced642668d25cbf976e58cd1&extent=-47.0855,-22.8388,-47.0221,-22.8058
- Portal de Mapas da UNICAMP: http://www.depi.unicamp.br/geo/maps/
- Geocodificação das bases de dados da Unicamp: http://www.depi.unicamp.br/geo/geocod/
- Mapas HIDS/DEPI: http://www.depi.unicamp.br/geo/geocod/


### Extract OSM info

- **Tools**: 
 - https://www.openstreetmap.org/export#map=15/-22.8224/-47.0617
 - https://www.azavea.com/blog/2015/12/21/tools-for-getting-data-out-of-openstreetmap-and-into-desktop-gis/
 - https://learnosm.org/en/osm-data/getting-data/
 - https://towardsdatascience.com/beginner-guide-to-download-the-openstreetmap-gis-data-24bbbba22a38
 - OSM-3D.org


## Principais tecnologias de manipulação de mapas


### SUMO: 
- **SUMO User Documentation**: https://sumo.dlr.de/docs/index.html
- **Buildings Module**: https://www.nsnam.org/docs/models/html/buildings.html
- **NetSimulyzer ns-3 Module**: https://github.com/usnistgov/NetSimulyzer-ns3-module#about


## Leafleat 
- **Leafleat**: https://leafletjs.com/
- **Leafleat e plugins 3D**:
    https://osmbuildings.org/data/
    https://webkid.io/blog/3d-maps-with-osmbuildings/

## Mapbox:
- **Display buildings in 3D**: https://docs.mapbox.com/mapbox-gl-js/example/3d-buildings/
- **Mapbox Streets v8**: https://docs.mapbox.com/vector-tiles/reference/mapbox-streets-v8/
- **Mapping 3D building features in OpenStreetMap**: https://blog.mapbox.com/mapping-3d-building-features-in-openstreetmap-7685ee12712a
- **Building heights in Mapbox Streets**: https://blog.mapbox.com/building-heights-in-mapbox-streets-14bc7399a4e8

## Libs Python:
- **Geopandas**: https://geopandas.org/
- **OMSnx**: https://github.com/gboeing/osmnx
- **ArchGIS**: https://developers.arcgis.com/python/sample-notebooks/openstreetmap-exploration/
- **Simple GPS data visualization using Python and Open Street Maps**: https://towardsdatascience.com/simple-gps-data-visualization-using-python-and-open-street-maps-50f992e9b676
- **Extracting and Converting OpenStreetMap data in Python**: http://andrewgaidus.com/Convert_OSM_Data/
- **Kepler**: https://kepler.gl/


## Respostas interessantes - Fóruns:
- https://forum.openstreetmap.org/viewtopic.php?id=72373

