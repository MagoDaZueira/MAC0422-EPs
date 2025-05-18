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
