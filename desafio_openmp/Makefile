CXX = g++
CXXFLAGS = -O2 -fopenmp -Wall

all: gerar_dados coeficiente_variacao

gerar_dados: data/gerar_dados.cpp
	$(CXX) $(CXXFLAGS) -o gerar_dados data/gerar_dados.cpp

coeficiente_variacao: src/coeficiente_variacao.cpp
	$(CXX) $(CXXFLAGS) -o coeficiente_variacao src/coeficiente_variacao.cpp

dataset: gerar_dados
	cd data && ../gerar_dados

run: coeficiente_variacao
	./coeficiente_variacao

clean:
	rm -f gerar_dados coeficiente_variacao

.PHONY: all dataset run clean
