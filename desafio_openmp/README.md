# Coeficiente de Variação com OpenMP

Exercício final — IESB, Coefficient of Variation (OpenMP).

Calcula o coeficiente de variação (CV) para **estatura** (classe de 8 cm) e **peso**
(classe de 4 kg) de 1000 pessoas, usando dados agrupados em intervalos de classe.
Cada cálculo é feito em versão sequencial e em versão paralela (OpenMP), e os tempos
são comparados.

## Estrutura

```
desafio_openmp/
├── data/
│   ├── gerar_dados.cpp   # gera a base de dados do aluno (1000 pessoas)
│   └── dataset.txt       # base gerada (criada ao rodar o gerador)
├── src/
│   └── coeficiente_variacao.cpp   # programa principal
├── Makefile
└── README.md
```

## Como rodar

```bash
make gerar_dados
make dataset          # cria data/dataset.txt
make coeficiente_variacao
make run
```

Ou manualmente:

```bash
g++ -O2 -fopenmp -o gerar_dados data/gerar_dados.cpp
(cd data && ../gerar_dados)
g++ -O2 -fopenmp -Wall -o coeficiente_variacao src/coeficiente_variacao.cpp
./coeficiente_variacao
```

## Sobre a base de dados

A base é gerada por `data/gerar_dados.cpp` com uma seed fixa (`20251111`), então é
sempre reprodutível, mas é "sua" — pode trocar a seed ou os parâmetros das
distribuições se quiser uma base diferente. Altura segue uma distribuição normal
(média 170 cm, desvio 9 cm); peso é derivado de um IMC aleatório aplicado à altura
de cada pessoa, mais ruído individual.

## Sobre a paralelização

Veja os comentários em `src/coeficiente_variacao.cpp`. Resumo:

- **min/max**: `reduction(min:) reduction(max:)` — cada thread reduz seu próprio
  min/max parcial, sem precisar de seção crítica.
- **histograma (frequência por classe)**: não dá pra usar `reduction` simples porque
  várias posições do vetor são compartilhadas. Cada thread acumula um histograma
  *local* e só no fim soma no histograma global dentro de uma seção `critical`.
- **soma ponderada (média) e soma dos quadrados (desvio padrão)**: `reduction(+:)`,
  o caso mais direto de paralelização.

## Resultado observado

Com N=1000 (o tamanho exigido pelo exercício), a versão paralela é **mais lenta**
que a sequencial — o overhead de criar/sincronizar threads domina um trabalho tão
pequeno. O programa inclui uma demonstração extra com dados sintéticos maiores
que mostra a partir de que tamanho de N o paralelismo passa a compensar. Essa é
justamente a discussão pedida na análise de desempenho do relatório.
