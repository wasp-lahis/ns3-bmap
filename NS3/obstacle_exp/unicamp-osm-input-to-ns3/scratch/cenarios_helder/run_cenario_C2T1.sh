#!/bin/sh

echo '\n #------- Run LoRaWAN simulations scripts: -------#\n'

echo '\n[SHELL INFO] START SIMULATION'

echo '\n[SHELL INFO] SIMULATION - Cenario 2 - Test 1 - Channel Propagation Cenario 1'
./waf --run "simulation-helder-cenarios --exp_name=C2T1 --channel_cenario=1 --simu_repeat=1"

echo '\n[SHELL INFO] SIMULATION - Cenario 2 - Test 1 - Channel Propagation Cenario 2'
./waf --run "simulation-helder-cenarios --exp_name=C2T1 --channel_cenario=2 --simu_repeat=1"

echo '\n[SHELL INFO] SIMULATION - Cenario 2 - Test 1 - Channel Propagation Cenario 3'
./waf --run "simulation-helder-cenarios --exp_name=C2T1 --channel_cenario=3 --simu_repeat=1"


echo '\n[SHELL INFO] SIMULATION FINISHED!'
