#!/bin/bash

CLIENT_NUM=$1
FILE_SIZES=${@:2}

SERVER_NAMES="ep4-servidor-inet_processos ep4-servidor-inet_threads ep4-servidor-inet_muxes ep4-servidor-unix_threads"
CLIENT_NAMES="ep4-cliente-inet ep4-cliente-unix"

# Compila todos os .c necessários (executáveis em /tmp/)
for file in $SERVER_NAMES $CLIENT_NAMES; do
    echo "Compilando $file"
    gcc ep4-clientes+servidores/$file.c -o /tmp/$file
done

for size in $FILE_SIZES; do
    # Gera arquivo de "size" MB
    echo ">>>>>>> Gerando um arquivo texto de: ${size}MB..."
    bytes=$((size * 1048576))
    base64 /dev/urandom | head -c $bytes > /tmp/${size}MB.txt
    echo >> /tmp/${size}MB.txt

    for server in $SERVER_NAMES; do
        echo "Subindo o servidor $server"
        /tmp/${server}

        pids=()

        for i in $(seq 1 $CLIENT_NUM); do
            if [[ $is_unix -eq 1 ]]; then
                /tmp/ep4-cliente-unix 127.0.0.1 < /tmp/${size}MB.txt &>/dev/null &
            else
                /tmp/ep4-cliente-inet 127.0.0.1 < /tmp/${size}MB.txt &>/dev/null &
            fi
            pids+=($!)
        done

        echo "Esperando os clientes terminarem..."

        for pid in "${pids[@]}"; do
            wait $pid
        done

        pkill -f -15 /tmp/$server
    done
done
