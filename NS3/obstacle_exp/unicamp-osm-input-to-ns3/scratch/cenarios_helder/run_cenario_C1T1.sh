#!/bin/sh

echo '\n #------- Run LoRaWAN simulations scripts: -------#\n'

echo '\n[SHELL INFO] START SIMULATION'

echo '\n[SHELL INFO] SIMULATION - Cenario 1 - Test 1 - Channel Propagation Cenario 1'
./waf --run "propagation-models-test --exp_name=C1T1_CP1 --channel_cenario=1 --simu_repeat=2"

echo '\n[SHELL INFO] SIMULATION - Cenario 1 - Test 1 - Channel Propagation Cenario 2'
./waf --run "propagation-models-test --exp_name=C1T1_CP1 --channel_cenario=2 --simu_repeat=2"

echo '\n[SHELL INFO] SIMULATION - Cenario 1 - Test 1 - Channel Propagation Cenario 3'
./waf --run "propagation-models-test --exp_name=C1T1_CP1 --channel_cenario=3 --simu_repeat=2"


echo '[SHELL INFO] SIMULATION FINISHED!'
