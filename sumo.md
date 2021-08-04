# SUMO 

## Tutorials:

- Docs:
    - User Documentation: https://sumo.dlr.de/docs/index.html
    - Exemplos: https://sumo.dlr.de/docs/Tutorials/

- Youtube Material:
  - Adil Alsuhaim: https://www.youtube.com/watch?v=IVGgDph51Vg
  - Dr. Joanne Skiles: https://www.youtube.com/watch?v=eXW4D32ePpE
  - Engineering Clinic: https://www.youtube.com/watch?v=XcRXb3NYd3I&list=PLX6MKaDw0naZILVYjo8_quA4JI8Jz-idL&index=24
  - Real time simulation of Vehicular Adhoc Networks (VANET) using NS3 and SUMO: https://www.youtube.com/watch?v=BjU3mdujoXg&t=1002s

- Others:
 - https://www.udemy.com/course/ferramenta-de-microssimulacao-de-trafego-sumo/
 - https://help.sumologic.com/01Start-Here/Quick-Start-Tutorials
  


## Usage:

- ***Find SUMO***:

```shell
which sumo
```

**ou**

```shell
locate -b sumo
```

- ***SUMO path folder (apt install)***: 

```
/usr/share/sumo
```

- ***WebWizard***:
    $ python /usr/share/sumo/tools/python/ osmWebWizard.py
    
    - WebWizard issue (https://stackoverflow.com/questions/57262675/having-problems-declaring-sumo-home)
        - $ export SUMO_HOME=/usr/share/sumo


- ***Netedit***:

```shell
sudo netedit
```

- ***Sumo-gui***:
```shell
sudo sumo-gui
```


- ***NS3***
    - Integração: 
        - Conversion: https://sumo.dlr.de/docs/Tutorials/Trace_File_Generation.html
        - Ns2_mobility_helper: https://www.nsnam.org/doxygen/classns3_1_1_ns2_mobility_helper.html
    - Exemplo:

```shell
sumo -c osm.sumocfg --fcd-output sumoTrace.xml
sudo /usr/share/sumo/tools/traceExporter.py --fcd-input sumoTrace.xml --ns2mobility-output ns2mobility.tcl
```
