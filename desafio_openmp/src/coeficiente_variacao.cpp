// Trabalho final - coeficiente de variacao (CV) usando dados agrupados em classes.
// Calculo pra estatura (classe de 8cm) e peso (classe de 4kg) de 1000 pessoas.
// Fiz uma versao sequencial e uma versao com OpenMP pra comparar o tempo das duas.
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <omp.h>

using namespace std;

// Com 1000 dados o calculo e muito rapido (microssegundos), entao medir uma
// unica vez nao da um tempo confiavel. Por isso repito o calculo varias vezes
// so pra cronometrar direito.
const int REPETICOES = 2000;

struct Classes {
    int k;              // numero de classes
    double min_val;     // limite inferior da primeira classe
    double largura;     // largura de cada classe
    vector<double> ponto_medio; // ponto medio de cada classe (x_i)
};

struct Resultado {
    double media;
    double desvio_padrao;
    double cv;
};

// ---------- Etapa 1: montar as classes a partir do min/max ----------

Classes montar_classes(double min_val, double max_val, double largura) {
    Classes c;
    c.largura = largura;
    c.min_val = min_val;
    double amplitude = max_val - min_val;
    c.k = (int) ceil(amplitude / largura);
    if (c.k < 1) c.k = 1;
    c.ponto_medio.resize(c.k);
    for (int i = 0; i < c.k; i++) {
        c.ponto_medio[i] = min_val + largura * i + largura / 2.0;
    }
    return c;
}

int indice_da_classe(double valor, double min_val, double largura, int k) {
    int idx = (int) ((valor - min_val) / largura);
    if (idx < 0) idx = 0;
    if (idx >= k) idx = k - 1;
    return idx;
}

// ---------- Versao SEQUENCIAL ----------

void minmax_sequencial(const vector<double>& dados, double& mn, double& mx) {
    mn = dados[0];
    mx = dados[0];
    for (size_t i = 1; i < dados.size(); i++) {
        if (dados[i] < mn) mn = dados[i];
        if (dados[i] > mx) mx = dados[i];
    }
}

vector<int> histograma_sequencial(const vector<double>& dados, const Classes& c) {
    vector<int> freq(c.k, 0);
    for (size_t i = 0; i < dados.size(); i++) {
        freq[indice_da_classe(dados[i], c.min_val, c.largura, c.k)]++;
    }
    return freq;
}

Resultado estatisticas_sequencial(const vector<int>& freq, const Classes& c, int N) {
    double soma = 0.0;
    for (int i = 0; i < c.k; i++) soma += freq[i] * c.ponto_medio[i];
    double media = soma / N;

    double soma_quad = 0.0;
    for (int i = 0; i < c.k; i++) {
        double diff = c.ponto_medio[i] - media;
        soma_quad += freq[i] * diff * diff;
    }
    double desvio = sqrt(soma_quad / N);

    Resultado r;
    r.media = media;
    r.desvio_padrao = desvio;
    r.cv = (desvio / media) * 100.0;
    return r;
}

// Junta as 4 etapas (min/max, montar classes, histograma, estatisticas) na versao sequencial
Resultado pipeline_sequencial(const vector<double>& dados, double largura, Classes& c_out) {
    double mn, mx;
    minmax_sequencial(dados, mn, mx);
    Classes c = montar_classes(mn, mx, largura);
    vector<int> freq = histograma_sequencial(dados, c);
    Resultado r = estatisticas_sequencial(freq, c, (int) dados.size());
    c_out = c;
    return r;
}

// ---------- Versao PARALELA (OpenMP) ----------

void minmax_paralelo(const vector<double>& dados, double& mn, double& mx) {
    double local_mn = dados[0];
    double local_mx = dados[0];
    int n = (int) dados.size();
    // reduction(min:)/reduction(max:) faz cada thread achar o min/max dela mesma
    // e no final o OpenMP junta tudo sozinho. Nao precisei de critical aqui.
    #pragma omp parallel for reduction(min:local_mn) reduction(max:local_mx)
    for (int i = 0; i < n; i++) {
        if (dados[i] < local_mn) local_mn = dados[i];
        if (dados[i] > local_mx) local_mx = dados[i];
    }
    mn = local_mn;
    mx = local_mx;
}

vector<int> histograma_paralelo(const vector<double>& dados, const Classes& c) {
    vector<int> freq(c.k, 0);
    int n = (int) dados.size();
    // Aqui nao da pra usar reduction direto porque varias threads podem tentar
    // incrementar a mesma posicao do vetor freq ao mesmo tempo (ia bagunçar o
    // resultado). Entao cada thread tem o proprio histograma (freq_local) e so
    // no final junta tudo no histograma de verdade, dentro do critical.
    #pragma omp parallel
    {
        vector<int> freq_local(c.k, 0);
        #pragma omp for nowait
        for (int i = 0; i < n; i++) {
            freq_local[indice_da_classe(dados[i], c.min_val, c.largura, c.k)]++;
        }
        #pragma omp critical
        {
            for (int j = 0; j < c.k; j++) freq[j] += freq_local[j];
        }
    }
    return freq;
}

Resultado estatisticas_paralela(const vector<int>& freq, const Classes& c, int N) {
    double soma = 0.0;
    #pragma omp parallel for reduction(+:soma)
    for (int i = 0; i < c.k; i++) soma += freq[i] * c.ponto_medio[i];
    double media = soma / N;

    double soma_quad = 0.0;
    #pragma omp parallel for reduction(+:soma_quad)
    for (int i = 0; i < c.k; i++) {
        double diff = c.ponto_medio[i] - media;
        soma_quad += freq[i] * diff * diff;
    }
    double desvio = sqrt(soma_quad / N);

    Resultado r;
    r.media = media;
    r.desvio_padrao = desvio;
    r.cv = (desvio / media) * 100.0;
    return r;
}

Resultado pipeline_paralelo(const vector<double>& dados, double largura, Classes& c_out) {
    double mn, mx;
    minmax_paralelo(dados, mn, mx);
    Classes c = montar_classes(mn, mx, largura);
    vector<int> freq = histograma_paralelo(dados, c);
    Resultado r = estatisticas_paralela(freq, c, (int) dados.size());
    c_out = c;
    return r;
}

// ---------- Utilidades ----------

bool carregar_dados(const string& caminho, vector<double>& alturas, vector<double>& pesos) {
    FILE* f = fopen(caminho.c_str(), "r");
    if (!f) return false;
    double a, p;
    while (fscanf(f, "%lf %lf", &a, &p) == 2) {
        alturas.push_back(a);
        pesos.push_back(p);
    }
    fclose(f);
    return true;
}

void imprimir_resultado(const string& nome, const Resultado& r, const Classes& c) {
    printf("\n--- %s ---\n", nome.c_str());
    printf("Numero de classes (k): %d (largura = %.1f)\n", c.k, c.largura);
    printf("Media ponderada (mu):      %.4f\n", r.media);
    printf("Desvio padrao (sigma):     %.4f\n", r.desvio_padrao);
    printf("Coeficiente de variacao:   %.2f %%\n", r.cv);
}

double cronometrar_sequencial(const vector<double>& dados, double largura, int repeticoes) {
    double inicio = omp_get_wtime();
    Classes c_tmp;
    for (int rep = 0; rep < repeticoes; rep++) {
        pipeline_sequencial(dados, largura, c_tmp);
    }
    return omp_get_wtime() - inicio;
}

double cronometrar_paralelo(const vector<double>& dados, double largura, int repeticoes) {
    double inicio = omp_get_wtime();
    Classes c_tmp;
    for (int rep = 0; rep < repeticoes; rep++) {
        pipeline_paralelo(dados, largura, c_tmp);
    }
    return omp_get_wtime() - inicio;
}

void processar_variavel(const string& nome, const vector<double>& dados, double largura) {
    Classes c_seq, c_par;
    Resultado r_seq = pipeline_sequencial(dados, largura, c_seq);
    Resultado r_par = pipeline_paralelo(dados, largura, c_par);

    imprimir_resultado(nome + " (sequencial)", r_seq, c_seq);
    imprimir_resultado(nome + " (paralelo)", r_par, c_par);

    double diff = fabs(r_seq.cv - r_par.cv);
    if (diff > 1e-6) {
        printf("ATENCAO: resultados sequencial e paralelo divergem para %s (diff=%.8f)\n",
               nome.c_str(), diff);
    } else {
        printf(">> Resultados conferem entre sequencial e paralelo.\n");
    }

    printf("\n%s - cronometrando %d repeticoes...\n", nome.c_str(), REPETICOES);
    double t_seq = cronometrar_sequencial(dados, largura, REPETICOES);
    double t_par = cronometrar_paralelo(dados, largura, REPETICOES);
    printf("Tempo sequencial: %.4f s\n", t_seq);
    printf("Tempo paralelo:   %.4f s\n", t_par);
    printf("Speedup:          %.2fx\n", t_seq / t_par);
}

int main() {
    vector<double> alturas, pesos;
    if (!carregar_dados("data/dataset.txt", alturas, pesos)) {
        fprintf(stderr, "Nao foi possivel abrir data/dataset.txt "
                         "(rode a partir da pasta do projeto, ou gere o dataset antes).\n");
        return 1;
    }

    printf("Dados carregados: %zu pessoas.\n", alturas.size());
    printf("Threads disponiveis: %d\n", omp_get_max_threads());

    processar_variavel("Estatura (cm)", alturas, 8.0);
    processar_variavel("Peso (kg)", pesos, 4.0);

    return 0;
}
