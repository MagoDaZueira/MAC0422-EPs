#ifndef EP2_H
#define EP2_H


// ======================== INCLUDES =======================
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>


// ======================== STRUCTS ========================
typedef struct Node {
    void *data;        // Elemento deste nó
    struct Node *next; // Ponteiro para o próximo nó
} Node;

typedef struct {
    Node *top; // Elemento do topo
    int size;  // Quantidade de elementos
} Stack;

typedef struct {
    void **data;     // Vetor com os elementos
    size_t size;     // Quantos elementos tem no vetor
    size_t capacity; // Quanto espaço está alocado para o vetor
} DynamicArray;

typedef struct Ciclista {
    int pos_x;        // Metragem
    int pos_y;        // Faixa
    int tempo_volta;  // Tempo em que concluiu sua última volta
    int volta;        // Volta em que está
    int esta_morto;   // Eliminado ou não
    int id;           // Inteiro que o representa
} Ciclista;


// ========================= STACK =========================
Stack* new_stack();              // Inicializa a pilha
int is_stack_empty(Stack *s);    // Pilha vazia ou não
void push(Stack *s, void *data); // Adiciona elemento ao topo da pilha
void pop(Stack *s);              // Remove elemento do topo da pilha
void* top(Stack *s);             // Retorna o elemento do topo da pilha, caso exista


// ===================== VETOR DINÂMICO ====================
void init_array(DynamicArray *arr, size_t initialCapacity); // Inicializa o vetor
void append(DynamicArray *arr, void *value);                // Adiciona um elemento na última posição
void clear_array(DynamicArray *arr);                        // Deixa o vetor vazio
void clear_and_free_elements(DynamicArray *arr);            // Deixa o vetor vazio e libera a memória dos elementos
void free_array(DynamicArray *arr);                         // Libera a memória ocupada pelo vetor
void copy_dynamic(DynamicArray* from, DynamicArray* to);    // Copia um vetor dinâmico para um outro


// ======================== DEFINES ========================
#define VOLTAS_MAX 25005 // Máximo de voltas relevantes à lógica de eliminação
#define FAIXAS 10        // Quantia de faixas da pista
#define POS_VAZIA -1     // Valor que indica uma posição sem ciclista


// ================= FUNÇÕES CONCORRENTES ==================
void move_ciclista(Ciclista* cic, int x, int y); // Move um ciclista para uma posição da pista (assume mutex trancado)
void* ciclista(void* arg);                       // Thread principal dos ciclistas


// ================= FUNÇÕES COORDENADORAS =================
void destroi_ciclista(Ciclista* cic); // Atualiza variáveis que sinalizam a eliminação de um ciclista
void verifica_destruicoes();          // Gerenciador das condições de eliminação (volta par e quebra)
void adiciona_ultimos_volta();        // Dá um push de blocos de ciclistas que completaram voltas simultaneamente
void resultados_finais();             // Impressões de ranking e ciclistas quebrados
void mostrar_debug();                 // Impressão de todas as posições da pista
void mostrar_informacoes();           // Quando alguma volta é finalizada ou alguma quebra ocorre, faz a impressão necessária
void coordenador();                   // Função principal do programa


// ===================== FUNÇÕES GERAIS ====================
int conta_espaco();                            // Verifica o espaço necessário entre cada posição na impressão do debug                
void gerar_ordem_aleatoria(int* vetor, int n); // Embaralhamento de um vetor de n posições
void posicoes_iniciais();                      // Determina as posições iniciais dos ciclistas, aleatoriamente dentro de um agrupamento
void zera_vetores();                           // Reinicializa os valores de alguns vetores


#endif // EP2_H
