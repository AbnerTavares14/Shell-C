# Modo de usar
### Primeiro utilize o comando abaixo para definir a variável de ambiente onde se encontra os programas de linha de comando do linux.
```bash
export CAMINHO=/usr/bin/
```
### Caso não faça isso, esse bash irá compilar, executar e atualizar(caso o arquivo tenha sido modificado mas já exista um executável dele) arquivos com a extensão ".c" tendo como referência o diretório atual("./"). 

### Ele suporta apenas um pipe por comando, por exemplo:
```bash
ls|wc
```
