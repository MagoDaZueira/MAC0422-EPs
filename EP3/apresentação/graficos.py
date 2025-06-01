import pandas as pd
import matplotlib.pyplot as plt
import sys

arquivo_csv = sys.argv[1]

df = pd.read_csv(arquivo_csv)

labels = ['First-fit', 'Next-fit', 'Best-fit', 'Worst-fit']
cores = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728']  # azul, laranja, verde, vermelho

def adicionar_valores(barras):
    for barra in barras:
        altura = barra.get_height()
        plt.text(barra.get_x() + barra.get_width()/2, altura,
                 f'{altura}', ha='center', va='bottom', fontsize=12)

# Gráfico: Tempo
plt.figure(figsize=(10, 5))
barras_tempo = plt.bar(range(len(df)), df['tempo'], tick_label=labels, color=cores)
plt.title('Tempo de Execução', fontsize=16)
plt.xlabel('Algoritmo', fontsize=14)
plt.ylabel('Tempo (s)', fontsize=14)
plt.xticks(fontsize=12)
plt.yticks(fontsize=12)
adicionar_valores(barras_tempo)
plt.tight_layout()
plt.show()

# Gráfico: Falhas
plt.figure(figsize=(10, 5))
barras_falhas = plt.bar(range(len(df)), df['falhas'], tick_label=labels, color=cores)
plt.title('Alocações Impossíveis', fontsize=16)
plt.xlabel('Algoritmo', fontsize=14)
plt.ylabel('Falhas', fontsize=14)
plt.xticks(fontsize=12)
plt.yticks(fontsize=12)
adicionar_valores(barras_falhas)
plt.tight_layout()
plt.show()
