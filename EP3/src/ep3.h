#ifndef EP3_H
#define EP3_H


// ====================== INCLUDES =======================
#include <stdio.h>
#include <stdlib.h>


// ===================== CONSTANTES ======================
#define LINHAS_INICIO 3      // Cabeçalho
#define TAM_LINHA 64         // Quantia de caracteres numa linha
#define UNIDADES_NA_LINHA 16 // Unidades de alocação numa linha
#define TAM_UNIDADE 4        // Tamanho em caracteres da unidade de alocação
#define TAM_ARQUIVO 65536    // Tamanho do arquivo em unidades de alocação


// =============== MANIPULAÇÃO DE ARQUIVOS ===============

// Transforma um arquivo numa cópia exata de um outro
// Os parâmetros são os nomes dos respectivos arquivos
// Caso um deles não exista, será criado automaticamente
void copia_arquivo(char* original, char* copia);

 // Altera o valor de uma unidade de alocação no .pgm
void escreve_posicao(FILE* arquivo, int posicao, int valor);

// Retorna o valor de uma unidade de alocação no .pgm
int le_posicao(FILE* arquivo, int posicao);

// ================= FUNÇÕES DE ALOCAÇÃO =================

// Itera sobre "unidades" unidades de alocação a partir de "inicio",
// marcando cada uma delas como ocupada
void preenche_memoria(FILE* memoria, int inicio, int unidades);

// Algoritmos de alocação utilizados
// Tentam alocar "unidades" unidades de alocação,
// retornando 1 em caso de sucesso e 0 caso contrário
int first_fit(FILE* memoria, int unidades);
int next_fit(FILE* memoria, int unidades);
int best_fit(FILE* memoria, int unidades);
int worst_fit(FILE* memoria, int unidades);


#endif // EP3_H
