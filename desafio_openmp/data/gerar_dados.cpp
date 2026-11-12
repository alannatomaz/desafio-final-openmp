// Gerador da base de dados do aluno.
// Cria "dataset.txt" com 1000 linhas "altura(cm) peso(kg)".
// Seed fixa -> mesma base sempre que for regenerada (reprodutibilidade para o relatório).
#include <cstdio>
#include <random>

int main() {
    const int N = 1000;
    const unsigned int SEED = 20251111; // troque por outro numero se quiser uma base diferente

    std::mt19937 gen(SEED);
    std::normal_distribution<double> altura_dist(170.0, 9.0);  // media 170cm, desvio 9cm
    std::normal_distribution<double> imc_dist(23.0, 3.2);       // IMC "base" da populacao
    std::normal_distribution<double> ruido_peso(0.0, 4.0);      // variacao individual de peso

    FILE* f = fopen("dataset.txt", "w");
    if (!f) {
        perror("Erro ao abrir dataset.txt");
        return 1;
    }

    for (int i = 0; i < N; i++) {
        double altura = altura_dist(gen);
        if (altura < 140.0) altura = 140.0;
        if (altura > 210.0) altura = 210.0;

        double altura_m = altura / 100.0;
        double imc = imc_dist(gen);
        double peso = imc * altura_m * altura_m + ruido_peso(gen);
        if (peso < 35.0) peso = 35.0;
        if (peso > 160.0) peso = 160.0;

        fprintf(f, "%.1f %.1f\n", altura, peso);
    }

    fclose(f);
    printf("dataset.txt gerado com %d pessoas (seed=%u).\n", N, SEED);
    return 0;
}
