# scratch

* **_simulation-many-lorawan-applications.cc_**: ns3 code to simulate many lorawan applications.



## Run simulation code

Insert this two commands to run simulations:

```bash

$ cd NS3_BASE_DIR
$ ./waf --run "simulation-many-lorawan-applications --simu_repeat=10 --channel_model=log-distance --n_devices_without_dataset=10"
 */
```

Parameters:
- **simu_repeat**: number of simulation repeat
- **channel_model**: channel propagation model
- **n_devices_without_dataset**: number of nodes without dataset. This number of nodes is apply for all applications that does not have dataset.