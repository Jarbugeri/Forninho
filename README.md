# Reflow Oven GUI

## Arquivos

- Na pasta **01 PCB** encontram-se os arquivos altium da PCB.

- Na pasta **03 Interface** encontra-se o executável para interface gráfica, e também o código fonte em python.

- Na pasta **04 ArduinoCode** encontra-se o arquivo para gravação do arduino, lá tem os padrões de curva de reflow e parâmetros dos controladores que podem ser alterados.

- Na pasta **05 Doc**, uma breve apresentação.

# Usando o forno

Dois métodos para utilizar

## Sem interface gráfica
- Usando os botões liga e desliga na frente do forno pode ser iniciado o processo de reflow, inicie o processo com o botão START, aguarde o Buzzer sinalizar abra o forno e espere esfriar.

## Com interface gráfica

- Liste as portas, e veja em qual está o arduino (EX:COM3).
- Selecione a porta e abra a conexão (EX:COM3).
- Inicie a aquisição de dados e de start no processo de reflow
- Aguarde o Buzzer sinalizar, abra o forno e espere esfriar.

# Funcionalidades interface

Este é um aplicativo Python que fornece uma Interface Gráfica do Usuário (GUI) para visualização de dados de forno de reflow. Utiliza Tkinter para a GUI e matplotlib para os gráficos.

- **Menu de Arquivo:**
  - **Limpar Dados:** Limpa os dados no arquivo "sampleData.txt".
  - **Abrir:** Não implementado (`donothing`).
  - **Salvar:** Salva o gráfico como "Grafico.png" (`save_fig`).
  - **Fechar:** Não implementado (`donothing`).
  - **Sair:** Fecha a aplicação (`app.quit`).

- **Comunicação (Aba 1):**
  - Botões e funções para listar as portas COM, abrir/fechar a porta COM, testar a comunicação, iniciar/parar a aquisição de dados, iniciar/parar o reflow e limpar o terminal.

- **Gráficos (Aba 2):**
  - Gráfico em tempo real mostrando o perfil de temperatura e a temperatura atual.
  - Botões para iniciar/parar a aquisição de dados, iniciar/parar o reflow e limpar o terminal.
  - Atualização em tempo real dos valores de temperatura e perfil.

- **Relatório (Aba 3):**
  - Uma imagem de um perfil de reflow com um link incorporado para um PDF do perfil de reflow.
  - Botão "Calcular" para calcular valores com base no perfil de reflow atual.
  - Entradas para os resultados calculados, como Taxa de Aquecimento, Temperatura Inicial de Preaquecimento, etc.

- **Funcionalidade de Aquisição de Dados em Tempo Real:**
  - O código contém uma função chamada `run_figure` chamada a cada segundo para atualizar o gráfico em tempo real.
  - Os dados de reflow são lidos da porta serial, e o gráfico é atualizado com base nesses dados.

Lembre-se de que o funcionamento completo do código depende de um ambiente específico, como a presença de uma porta serial real e o correto funcionamento do hardware associado. 
