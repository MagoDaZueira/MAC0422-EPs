/*
NOME: Otávio Garcia Capobianco
NUSP: 15482671
EXERCÍCIO-PROGRAMA: EP3
*/

#include <stdio.h>
#include <stdlib.h>

#define LINHAS_INICIO 3      // Cabeçalho
#define TAM_LINHA 64         // Quantia de caracteres numa linha
#define UNIDADES_NA_LINHA 16 // Unidades de alocação numa linha
#define TAM_UNIDADE 4        // Tamanho em caracteres da unidade de alocação
#define TAM_ARQUIVO 65536    // Tamanho do arquivo em unidades de alocação

/* ==========================================================
   ================= MANIPULAÇÃO DE ARQUIVOS ================
   ========================================================== */
// Escreve um valor inteiro em uma posição de um arquivo .pgm
void escreve_posicao(FILE* arquivo, int posicao, int valor) {
    // Vai para o início do arquivo
    rewind(arquivo);

    // Pula cabeçalho
    for (int i = 0; i < LINHAS_INICIO; i++) {
        while (fgetc(arquivo) != '\n') continue;
    }
    // Início das unidades
    int offset = ftell(arquivo);

    // Pula as linhas e unidades antes da posição
    offset += TAM_LINHA * (posicao / UNIDADES_NA_LINHA);
    offset += TAM_UNIDADE * (posicao % UNIDADES_NA_LINHA);

    // Vai para a posição
    fseek(arquivo, offset, SEEK_SET);

    // Escreve o valor especificado na posição
    if (posicao % UNIDADES_NA_LINHA == UNIDADES_NA_LINHA - 1) {
        fprintf(arquivo, "%3d\n", valor);
    }
    else {
        fprintf(arquivo, "%3d ", valor);
    }
}

// Lê um valor inteiro de uma posição de um arquivo .pgm
int le_posicao(FILE* arquivo, int posicao) {
    // Vai para o início do arquivo
    rewind(arquivo);

    // Pula cabeçalho
    for (int i = 0; i < LINHAS_INICIO; i++) {
        while (fgetc(arquivo) != '\n') continue;
    }

    // Início das unidades
    int offset = ftell(arquivo);

    // Pula as linhas e unidades antes da posição
    offset += TAM_LINHA * (posicao / UNIDADES_NA_LINHA);
    offset += TAM_UNIDADE * (posicao % UNIDADES_NA_LINHA);

    // Vai para a posição
    fseek(arquivo, offset, SEEK_SET);

    // Lê os 3 caracteres do bloco
    char bloco[4];
    if (fread(bloco, 1, 3, arquivo) != 3) {
        fprintf(stderr, "Erro ao ler bloco na posição %d\n", posicao);
        return -1;
    }
    bloco[3] = '\0';

    // Converte para inteiro (ignorando espaços)
    return atoi(bloco);
}

// Cria uma cópia de um arquivo
void copia_arquivo(char* original, char* copia) {
    FILE* arq_original = fopen(original, "r");
    FILE* arq_copia = fopen(copia, "w");

    // Copia cada caractere até o end of file
    char current;
    while ((current = fgetc(arq_original)) != EOF) {
        fputc(current, arq_copia);
    }

    fclose(arq_original);
    fclose(arq_copia);
}

/* ==========================================================
   ================= ALGORITMOS DE ALOCAÇÃO =================
   ========================================================== */

void preenche_memoria(FILE* memoria, int inicio, int unidades) {
    int fim = inicio + unidades;
    for (int i = inicio; i < fim; i++) {
        escreve_posicao(memoria, i, 0);
    }
}

int first_fit(FILE* memoria, int unidades) {
    int tamanho_atual = 0; // Quantidade de unidades livres contínuas atual
    int inicio_bloco = -1; // Índice de início das unidades livres contínuas

    // Itera sobre todas as unidades de alocação, do início
    for (int i = 0; i < TAM_ARQUIVO; i++) {
        // Ocupada
        if (le_posicao(memoria, i) == 0) {
            tamanho_atual = 0;
            continue;
        }
        // Livre
        // Novo bloco contínuo
        if (tamanho_atual == 0) inicio_bloco = i;
        // Incrementa contagem contínua e verifica se achou um bloco grande o suficiente
        tamanho_atual++;
        if (tamanho_atual >= unidades) {
            // Achou uma posição possível. Preenche ela e finaliza a execução
            preenche_memoria(memoria, inicio_bloco, unidades);
            return 1; // Sucesso
        }
    }
    return 0; // Não conseguiu
}

int ultima_posicao = 0;
int next_fit(FILE* memoria, int unidades) {
    int tamanho_atual = 0; // Quantidade de unidades livres contínuas atual
    int inicio_bloco = -1; // Índice de início das unidades livres contínuas

    // Itera sobre todas as unidades de alocação circularmente,
    // a partir da posição à direita da última alocada
    int i = ultima_posicao;
    int passos = TAM_ARQUIVO;
    while (passos--) {
        // Ocupada
        if (le_posicao(memoria, i) == 0) {
            tamanho_atual = 0;
        }
        // Livre
        else {
            // Novo bloco contínuo
            if (tamanho_atual == 0) inicio_bloco = i;
            // Incrementa contagem contínua e verifica se achou um bloco grande o suficiente
            tamanho_atual++;
            if (tamanho_atual >= unidades) {
                // Achou uma posição possível. Preenche ela e finaliza a execução
                preenche_memoria(memoria, inicio_bloco, unidades);
                ultima_posicao = (i + 1) % TAM_ARQUIVO;
                return 1; // Sucesso
            }
        }
        // Incrementa circularmente, zerando a contagem de contínuos ao dar a volta
        i = (i + 1) % TAM_ARQUIVO;
        if (i == 0) tamanho_atual = 0;
    }

    // Vai até o fim do bloco livre contínuo em que a posição original estava
    // Caso tal posição não esteja no início do bloco, poderíamos ignorar um espaço válido
    if (tamanho_atual == 0) inicio_bloco = i;
    while (i < TAM_ARQUIVO && le_posicao(memoria, i) != 0) {
        tamanho_atual++;
        if (tamanho_atual >= unidades) {
            // Achou uma posição possível. Preenche ela e finaliza a execução
            preenche_memoria(memoria, inicio_bloco, unidades);
            ultima_posicao = (i + 1) % TAM_ARQUIVO;
            return 1; // Sucesso
        }
    }

    return 0; // Não conseguiu
}

int best_fit(FILE* memoria, int unidades) {
    int tamanho_atual = 0; // Quantidade de unidades livres contínuas atual
    int inicio_bloco = -1; // Índice de início das unidades livres contínuas

    // Gerência do espaço escolhido (inicializado com valor grande arbitrário)
    int melhor_tamanho_bloco = 100000; // Quantidade de unidades livres contínuas do melhor bloco
    int melhor_inicio_bloco = -1;      // Índice de inínio do melhor bloco

    // Itera sobre todas as unidades de alocação, do início
    for (int i = 0; i < TAM_ARQUIVO; i++) {
        // Ocupada
        if (le_posicao(memoria, i) == 0) {
            // Avalia se o bloco que acabou agora é o melhor
            if (tamanho_atual >= unidades && tamanho_atual < melhor_tamanho_bloco) {
                melhor_tamanho_bloco = tamanho_atual;
                melhor_inicio_bloco = inicio_bloco;
            }
            tamanho_atual = 0;
            continue;
        }
        // Livre
        if (tamanho_atual == 0) inicio_bloco = i; // Novo bloco contínuo
        tamanho_atual++; // Incrementa contagem contínua
    }

    // Encontrou espaço válido
    if (melhor_inicio_bloco != -1) {
        preenche_memoria(memoria, melhor_inicio_bloco, unidades);
        return 1;
    }
    return 0; // Não conseguiu
}

int worst_fit(FILE* memoria, int unidades) {
    int tamanho_atual = 0; // Tamanho de unidades livres contínuas atual
    int inicio_bloco = -1; // Índice de início das unidades livres contínuas

    // Gerência do espaço escolhido (inicializado com valor menor que 0 arbitrário)
    int melhor_tamanho_bloco = -1; // Quantidade de unidades livres contínuas do melhor bloco
    int melhor_inicio_bloco = -1;  // Índice de inínio do melhor bloco

    // Itera sobre todas as unidades de alocação, do início
    for (int i = 0; i < TAM_ARQUIVO; i++) {
        // Ocupada
        if (le_posicao(memoria, i) == 0) {
            // Avalia se o bloco que acabou agora é o melhor
            // Abaixo está a única diferença entre este e o best fit: trocamos "<" por ">"
            if (tamanho_atual >= unidades && tamanho_atual > melhor_tamanho_bloco) {
                melhor_tamanho_bloco = tamanho_atual;
                melhor_inicio_bloco = inicio_bloco;
            }
            tamanho_atual = 0;
            continue;
        }
        // Livre
        if (tamanho_atual == 0) inicio_bloco = i; // Novo bloco contínuo
        tamanho_atual++; // Incrementa contagem contínua
    }

    // Encontrou espaço válido
    if (melhor_inicio_bloco != -1) {
        preenche_memoria(memoria, melhor_inicio_bloco, unidades);
        return 1;
    }
    return 0; // Não conseguiu
}

void compactar(FILE* memoria) {
    int pos_livre = 0;
    int pos_ocupada;
    
    // Queremos garantir que pos_livre é sempre um valor 255
    while (pos_livre < TAM_ARQUIVO && le_posicao(memoria, pos_livre) == 0) pos_livre++;
    if (pos_livre == TAM_ARQUIVO) return;

    // Roda até compactar tudo
    // Garantimos que todas as posições atrás de pos_livre são 0, ocupadas
    while (1) {
        // pos_ocupada parte de pos_livre até achar uma posição de fato ocupada
        pos_ocupada = pos_livre;
        while (pos_ocupada < TAM_ARQUIVO && le_posicao(memoria, pos_ocupada) == 255) pos_ocupada++;

        // Não há mais memória ocupada à frente de pos_livre: acabou a compactação
        if (pos_ocupada == TAM_ARQUIVO) return;

        // Com pos_livre partindo do início do espaço vazio, o pos_ocupada do início do ocupado,
        // Preenchemos as livres e liberamos as ocupadas, uma a uma
        while (pos_ocupada < TAM_ARQUIVO && le_posicao(memoria, pos_ocupada) == 0) {
            escreve_posicao(memoria, pos_livre, 0); pos_livre++;
            escreve_posicao(memoria, pos_ocupada, 255); pos_ocupada++;
        }
    }
}


void gerenciador(int algoritmo, char* trace, char* saida) {
    FILE* arq_trace = fopen(trace, "r");
    FILE* arq_memoria = fopen(saida, "r+");
    char linha[16];
    int impossiveis = 0;
    
    while (fgets(linha, sizeof(linha), arq_trace)) {
        char parte1[6], parte2[6];
        sscanf(linha, "%6s %6s", parte1, parte2);

        int vai_compactar = parte2[0] == 'C';

        if (vai_compactar) {
            compactar(arq_memoria);
        }
        else {
            int linha_atual = atoi(parte1);
            int unidades = atoi(parte2);
            int conseguiu;

            switch (algoritmo) {
            case 1:
                conseguiu = first_fit(arq_memoria, unidades);
                break;
            case 2:
                conseguiu = next_fit(arq_memoria, unidades);
                break;
            case 3:
                conseguiu = best_fit(arq_memoria, unidades);
                break;
            default:
                conseguiu = worst_fit(arq_memoria, unidades);
                break;
            }
            if (!conseguiu) {
                printf("%d\n", linha_atual);
                impossiveis++;
            }
        }
    }

    printf("%d\n", impossiveis);

    fclose(arq_trace);
    fclose(arq_memoria);
}

int main(int argc, char *argv[]) {
    int algoritmo = atoi(argv[1]);
    char* entrada = argv[2];
    char* trace = argv[3];
    char* saida = argv[4];

    copia_arquivo(entrada, saida);

    gerenciador(algoritmo, trace, saida);

    return 0;
}
