# NS3 

- Support: ns3 google groups
- Some commands:
    - run a program: 
    ```bash
    $ ./waf --run program_name
    $ ./waf --run mestrado
    ```   
    - run and debug a program:
    ```bash
    $ ./waf --run program_name --gdb
    (gdb) run
    (gdb) info locals
    (gdb) help
    quit
    ```       
    - run a program and write its output in a file
    ```bash
    $ ./waf --run program_name > log_ns3_program.out 2>&1
    $ ./waf --run complete-network-example > log_ns3_program.out 2>&1
    ```

## Lorawan module:
    - Github: https://github.com/signetlabdei/lorawan
    - Doxygen: 
        - https://signetlabdei.github.io/lorawan-docs/models/build/html/lorawan.html
        - https://signetlabdei.github.io/lorawan-docs/html/index.html
    - Support: https://gitter.im/ns-3-lorawan/Lobby?source=orgpage


