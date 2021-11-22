def get_txt_values(filename):
    
    # list of all rows of buildings_settings
    file_rows = []

    # open file in a read mode
    file = open(filename, 'r')

    for line in file.readlines():
        
        # create a str without \n and ","
        line_str = line.strip().split("\t")  
        file_rows.append(line_str)
    
    # remove colunm names
    file_rows.pop(0)
    
    return file_rows



def remove_duplicate_position(rows):
    media = 0
    rssi_list = []
    rssi_float_list = []
    new_rssi_data = []

    for i in range(1,len(rows)):
        lat = rows[i][0]
        lng = rows[i][1]
        rssi = rows[i][2]

        if rows[i-1][0] == lat and rows[i-1][1] == lng:
            if len(rssi_list) == 0:
                rssi_list.append(rows[i-1][2])
            rssi_list.append(rssi)
        else:
            if len(rssi_list) == 0:
                new_rssi_data.append([lat, lng, float(rssi)])
            else:
                rssi_float_list = [float(i) for i in rssi_list]
                media = (sum(rssi_float_list)/len(rssi_float_list))
                # print(rssi_float_list, media)
                new_rssi_data.append([lat, lng, round(media,2)])
                  
            rssi_list.clear()
            rssi_float_list.clear()

    return new_rssi_data


def create_txt(data, file_path):

    with open(file_path, "w") as f:
        for i in range(len(data)):            
            lat = str(data[i][0])
            lng = str(data[i][1])
            rssi = str(data[i][2])
            
            if i == 0:
                f.write("Latitude\tLatitude\tValue\n")
               
            f.write(lat + "\t" + lng + "\t"+ rssi + "\n" ) 


# --------------- MAIN ---------------
original_dataset_path = "table-rssi-2805"
new_dataset_path = "table-rssi-2805-w-duplicate-values"

txt_rows = get_txt_values(original_dataset_path)
print("\n[INFO] READ RSSI DATASET:", original_dataset_path)
new_rssi_data = remove_duplicate_position(txt_rows)
print("[INFO] REMOVE DUPLICATE POSITION VALUES")
print("[INFO] NEW DATASET LEN:", len(new_rssi_data))
create_txt(new_rssi_data, new_dataset_path)
print("[INFO] NEW DATASET CREATED!!\n")

