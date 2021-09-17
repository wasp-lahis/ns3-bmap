# 3D Obstacle Shadowing Model

Para utilizar o módulo de obstáculo, é necessário instalar a lib de geometria ***CGAL*** e tornar suas dependências visíveis para o NS3. 

## Instalação e Configuração de CGAL e dependências

Para instalar e configurar a lib CGAL, siga os passos:

- Instalar CGAL pela linha de comando (ou diretamente pela source):
```shell
sudo apt-get install libcgal-dev
```

- Adicionar a pasta **obstacle** ao diretório ```$NS3-BASE-DIR/src```

- Dentro da pasta ```$NS3-BASE-DIR/src/obstacle```, abrir o scritp **wscript** e alterar os paths das linhas que tem a função **append_value** de acordo com o tipo de instalação (passo 1). Ele está configurado por padrão para a instalação seguindo ***apt-get***

- Na linha de comando, insira:
```shell

cd $NS3-BASE-DIR
rm -rf build
./waf configure --cxx-standard=-std=c++17
./waf build
```

### Notas
Esses passos foram realizados com as seguintes versões do gcc e g++: 
```shell
gcc-11
g++-11
```


## Utilização do Módulo de Obstáculo 3D

Para testar uso da modulo de ***obstáculo*** com o módulo ***lorawan***:

- Inserir o arquivo ```obstacle-model-test.cc``` na pasta ```$NS3-BASE-DIR/scratch```

- Inserir os .xmls (***LA.ns2mobility.xml*** e ***LA.poly.xml***) na pasta ```$NS3-BASE-DIR```

- Executar:
```shell
cd $NS3-BASE-DIR
./waf --run obstacle-model-test
```

## Referências
- CGAL lib: https://www.cgal.org/download/linux.html
- CGAL Releases: https://github.com/CGAL/cgal/releases
- https://garfield001.wordpress.com/2013/04/16/compile-and-link-ns3-program-with-external-library/
- https://gitlab.com/nsnam/ns-3-dev/-/issues/352