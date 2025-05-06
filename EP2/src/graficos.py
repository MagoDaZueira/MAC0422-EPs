import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from scipy import stats
import sys

def calcular_ic(amostras, confianca=0.95):
    n = len(amostras)
    media = np.mean(amostras)
    erro_padrao = stats.sem(amostras)
    margem = erro_padrao * stats.t.ppf((1 + confianca) / 2., n-1)
    return media, margem

def plotar_comparacoes_pares(arquivos, modo):
    assert len(arquivos) == 6
    assert modo in ['tempo', 'memoria']

    coluna = 'tempo_segundos' if modo == 'tempo' else 'memoria_kbytes'
    nome_y = 'Tempo (s)' if modo == 'tempo' else 'Memória (kB)'

    medias = []
    ics = []
    labels = []

    for i in range(0, 6, 2):
        df1 = pd.read_csv(arquivos[i])
        df2 = pd.read_csv(arquivos[i+1])

        m1, ic1 = calcular_ic(df1[coluna])
        m2, ic2 = calcular_ic(df2[coluna])

        medias.extend([m1, m2])
        ics.extend([ic1, ic2])

        # label1 = f'Ingênua'
        # label2 = f'Eficiente'
        # labels.append((label1, label2))
        labels = [30, 150, 500]
        # labels = [30, 150, 450]

    x = np.arange(3)  # três agrupamentos
    largura = 0.35

    fig, ax = plt.subplots(figsize=(10, 6))
    barras1 = ax.bar(x - largura/2, medias[::2], largura, yerr=ics[::2], label='Ingênua', capsize=5)
    barras2 = ax.bar(x + largura/2, medias[1::2], largura, yerr=ics[1::2], label='Eficiente', capsize=5)

    # Adiciona texto com as médias acima de cada barra
    for bar in barras1:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height + max(ics)*0.05,
                f'{height:.2f}', ha='center', va='bottom', fontsize=9)

    for bar in barras2:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height + max(ics)*0.05,
                f'{height:.2f}', ha='center', va='bottom', fontsize=9)

    ax.set_ylabel(nome_y)
    ax.set_title(f'Comparação de {nome_y} entre Abordagens\nCiclistas = 150, Variando Pista')
    ax.set_xticks(x)
    ax.set_xticklabels([f'Pista de {i}m' for i in labels])
    # ax.set_xticklabels([f'{l1} vs {l2}' for l1, l2 in labels])
    ax.legend()
    plt.tight_layout()
    plt.grid(axis='y', linestyle='--', alpha=0.5)
    plt.show()

# Exemplo de uso:
# python grafico_pares.py tempo arq1.csv arq2.csv arq3.csv arq4.csv arq5.csv arq6.csv
if __name__ == "__main__":
    if len(sys.argv) != 8:
        print("Uso: python grafico_pares.py [tempo|memoria] arq1.csv ... arq6.csv")
        sys.exit(1)

    modo = sys.argv[1]
    arquivos = sys.argv[2:]
    plotar_comparacoes_pares(arquivos, modo)
