#!/bin/sh

# Run Lora simulations scripts:
# - ns3 simulation script
# - copy ns3 results files to /NS3
# - convert ns3Files to csv script

echo '\n #------- Run Lora simulations scripts: -------#'

echo '-\n [INFO] ns3 simulation script:'
./waf --run marianna.cc

echo '-\n [INFO] copy ns3 results files to /NS3:'
cp network_results.txt ~/Desktop/Mestrado/Coleta_Residuos/NS3
cp buildings_dimensions.txt ~/Desktop/Mestrado/Coleta_Residuos/NS3


echo '-\n [INFO] convert ns3Files to csv script'
python ~/Desktop/Mestrado/Coleta_Residuos/NS3/ns3files_to_csv.py