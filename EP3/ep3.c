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

/* ==========================================================
   ================= ALGORITMOS DE ALOCAÇÃO =================
   ========================================================== */

int first_fit(FILE* memoria, int unidades);
int next_fit(FILE* memoria, int unidades);
int best_fit(FILE* memoria, int unidades);
int worst_fit(FILE* memoria, int unidades);

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
    char* entrada = argv[1];
    char* trace = argv[2];
    char* saida = argv[3];

    copia_arquivo(entrada, saida);

    FILE* arq_saida = fopen(saida, "r+");

    escreve_posicao(arq_saida, 15, 254);
    escreve_posicao(arq_saida, 0, 2);

    return 0;
}
