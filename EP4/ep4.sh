#!/bin/bash

CLIENT_NUM=$1
FILE_SIZES=${@:2}

SERVER_NAMES="ep4-servidor-inet_processos ep4-servidor-inet_threads ep4-servidor-inet_muxes ep4-servidor-unix_threads"
CLIENT_NAMES="ep4-cliente-inet ep4-cliente-unix"

# Compila todos os .c necessários (executáveis em /tmp/)
for file in $SERVER_NAMES $CLIENT_NAMES; do
    echo "Compilando $file"
    gcc ep4-clientes+servidores/$file.c -o /tmp/$file -pthread
done

times_file="/tmp/ep4-resultados-${CLIENT_NUM}.data"
echo -n "" > "$times_file"

for size in $FILE_SIZES; do
    line=$(printf "%02d" "$size")

    # Gera arquivo de "size" MB
    echo ">>>>>>> Gerando um arquivo texto de: ${size}MB..."
    bytes=$((size * 1048576))
    base64 /dev/urandom | head -c $bytes > /tmp/${size}MB.txt
    echo >> /tmp/${size}MB.txt

    for server in $SERVER_NAMES; do
        echo "Subindo o servidor $server"
        /tmp/${server}

        start=$(date +"%Y-%m-%d %H:%M:%S")

        if [[ $server == *"unix"* ]]; then
            is_unix=1
        else
            is_unix=0
        fi

        pids=()
        for i in $(seq 1 $CLIENT_NUM); do
            if [[ $is_unix -eq 1 ]]; then
                /tmp/ep4-cliente-unix < /tmp/${size}MB.txt &>/dev/null &
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

        first_accept=$(journalctl -q --since="$start" | grep "$server" | grep "accept" | head -n 1)
        last_exit=$(journalctl -q --since="$start" | grep "$server" | grep "exit" | tail -n 1)

        accept_date=$(echo "$first_accept" | awk '{print $1, $2, $3}')
        exit_date=$(echo "$last_exit" | awk '{print $1, $2, $3}')

        accept_date=$(date -d "$accept_date" +"%Y-%m-%d %H:%M:%S")
        exit_date=$(date -d "$exit_date" +"%Y-%m-%d %H:%M:%S")

        time_spent=$(dateutils.ddiff "$accept_date" "$exit_date" -f "%0M:%0S")

        line+=" $time_spent"
    done

    echo "$line" >> "$times_file"
done
