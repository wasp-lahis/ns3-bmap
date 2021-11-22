from xml.dom import minidom

# funcao p corrigir formato do atributo shape
def polygon_to_str(polygon_coords):
    poly_list = polygon_coords.split(", ")
    poly_final_str = ""
    for i in range(len(poly_list)):
        poly_final_str = poly_final_str + poly_list[i].replace(" ", ",") + " "

    return poly_final_str



# ------------ MAIN ------------
xml_file = 'predios_unicamp_dataset_final.xml'
xml_doc = minidom.parse(xml_file)
poly_element = xml_doc.getElementsByTagName("poly")

# atualizando valores dos atributos id, type, shape
for i in range(len(poly_element)):
	id_value = poly_element[i].attributes["id"].value
	poly_element[i].attributes["id"].value = id_value.split('.')[0]
	poly_element[i].attributes["type"].value = "building"	
	poly_element[i].attributes["shape"].value = polygon_to_str(poly_element[i].attributes["shape"].value)

# salvando alteracoes
with open(xml_file, 'w') as file:
    file.write(xml_doc.toxml()) 
