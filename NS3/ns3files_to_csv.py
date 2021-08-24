#!/usr/bin/python
# -*- coding: utf-8 -*-

# Description: Convert .txt NS-3 file to .csv file
#
# Author: Lahis Almeida
#
# To run:
# $ python ns3files_to_csv.py --networkFile "buildings.txt" --buildingsFile "buildings.csv"
# $ python ns3files_to_csv.py 


# libs
import csv
import sys
import argparse
from pathlib import Path


# dataset buildings
buildings_colunms_names  = [
    "building_id",
    "boundaries_xMin",
    "boundaries_yMin",
    "boundaries_xMax",
    "boundaries_yMax",
    "boundaries_zMax",
    "boundaries_zMin"
]

# dataset network
network_colunms_names  = [
    "ed_id",
    "ed_x",
    "ed_y",
    "ed_z",
    "sf",
    "gw_id",
    "gw_x",
    "gw_y",
    "gw_z",
    "distance"
]


#---- extract buildings settings of ns3 txt result file
def get_ns3File_settings(ns3_filename):
    
    # list of all rows of buildings_settings
    file_rows = []

    # open file in a read mode
    file = open(ns3_filename, 'r')

    for line in file.readlines():
        
        # create a str without \n and ","
        line_str = line.strip().split(",")  
        file_rows.append(line_str)

    return file_rows
       


#---- create a csv file
def create_csv(title, colunms_names):
    with open(title,"w+") as file:
        writer = csv.writer(file, delimiter=",")
        writer.writerow(colunms_names)


#---- add row on csv file
def add_csv_rows(ns3_settings, colunms_names, title):
    for colunm in ns3_settings:
        if len(colunm) == len(colunms_names):
            with open(title,"a") as file:
                writer = csv.writer(file, delimiter=",")
                writer.writerow([float(x) for x in colunm])
    




#----------- MAIN PROGRAM -----------

if __name__ == '__main__':

    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--networkFile",
        type = str,
        default = 'network_results.txt',
        help = "File name of the ns3 network info file"
    )

    parser.add_argument(
        "--buildingsFile",
        type = str,
        default = 'buildings_dimensions.txt',
        help = "File name of the ns3 building info file"
    )
       


    args = parser.parse_args()

    network_settings = get_ns3File_settings(args.networkFile)
    buildings_settings = get_ns3File_settings(args.buildingsFile)


    # for i in network:
    #     print (i)

    # create a csv file with ns3 network setting lists
    #output_filename = '../NS3/network_results.csv'
    output_filename = 'network_results.csv'
    create_csv(output_filename, network_colunms_names)
    add_csv_rows(network_settings, network_colunms_names, output_filename)

    # create a csv file with ns3 buildings setting lists
    output_filename = 'buildings_dimensions.csv'
    create_csv(output_filename, buildings_colunms_names)
    add_csv_rows(buildings_settings, buildings_colunms_names, output_filename)




    