#!/bin/bash

# # # # # # # # # # # # # # # # # #
# NOME: Otávio Garcia Capobianco  #
# NUSP: 15482671                  #
# EXERCÍCIO-PROGRAMA: EP4         #
# # # # # # # # # # # # # # # # # #

CLIENT_NUM=$1
FILE_SIZES=${@:2}

SERVER_NAMES="ep4-servidor-inet_processos ep4-servidor-inet_threads ep4-servidor-inet_muxes ep4-servidor-unix_threads"
CLIENT_NAMES="ep4-cliente-inet ep4-cliente-unix"

# Compila todos os .c necessários (executáveis em /tmp/)
for file in $SERVER_NAMES $CLIENT_NAMES; do
    echo "Compilando $file"
    gcc ep4-clientes+servidores/$file.c -o /tmp/$file -pthread
done

DATA_FILE="/tmp/ep4-resultados-${CLIENT_NUM}.data"
echo -n "" > "$DATA_FILE"

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

        echo ">>>>>>> Fazendo ${CLIENT_NUM} clientes ecoarem um arquivo de: ${size}MB..."
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


        echo "Verificando os instantes de tempo no journald..."

        server_logs=$(journalctl -q --since="$start" | grep "$server")
        first_accept=$(echo "$server_logs" | grep "accept" | head -n 1)
        all_exits=$(echo "$server_logs" | grep "exit")
        last_exit=$(echo "$all_exits" | tail -n 1)

        if [ "$(echo "$all_exits" | wc -l)" -ne "$CLIENT_NUM" ]; then
            echo "Erro: nem todos os clientes encerraram a conexão com ${server}"
            exit 1
        fi

        echo ">>>>>>> ${CLIENT_NUM} clientes encerraram a conexão"

        accept_date=$(echo "$first_accept" | awk '{print $1, $2, $3}')
        exit_date=$(echo "$last_exit" | awk '{print $1, $2, $3}')

        accept_date=$(date -d "$accept_date" +"%Y-%m-%d %H:%M:%S")
        exit_date=$(date -d "$exit_date" +"%Y-%m-%d %H:%M:%S")

        time_spent=$(dateutils.ddiff "$accept_date" "$exit_date" -f "%0M:%0S")

        line+=" $time_spent"

        echo ">>>>>>> Tempo para servir os ${CLIENT_NUM} clientes com o ${server}: ${time_spent}"

        echo "Enviando um sinal 15 para o servidor ${server}..."

        pkill -f -15 /tmp/$server
    done

    echo "$line" >> "$DATA_FILE"

    rm /tmp/${size}MB.txt
done

echo -n ">>>>>>> Gerando o gráfico de ${CLIENT_NUM} clientes com arquivos de:"
for size in ${FILE_SIZES[@]}; do
    echo -n " ${size}MB"
done
echo

GPI_FILE="/tmp/graficos.gpi"

echo "set ydata time" > $GPI_FILE
echo "set timefmt \"%M:%S\"" >> $GPI_FILE
echo "set format y \"%M:%S\"" >> $GPI_FILE
echo "set xlabel \"Dados transferidos por cliente (MB)\"" >> $GPI_FILE
echo "set ylabel \"Tempo para atender ${CLIENT_NUM} clientes concorrentes\"" >> $GPI_FILE
echo "set term pdfcairo" >> $GPI_FILE
echo "set output \"ep4-resultados-${CLIENT_NUM}.pdf\"" >> $GPI_FILE
echo "set grid" >> $GPI_FILE
echo "set key top left" >> $GPI_FILE
echo "plot \"${DATA_FILE}\" using 1:4 with linespoints title \"Sockets da Internet: Mux de E/S\",\\" >> $GPI_FILE
echo "     \"${DATA_FILE}\" using 1:3 with linespoints title \"Sockets da Internet: Threads\",\\" >> $GPI_FILE
echo "     \"${DATA_FILE}\" using 1:2 with linespoints title \"Sockets da Internet: Processos\",\\" >> $GPI_FILE
echo "     \"${DATA_FILE}\" using 1:5 with linespoints title \"Sockets Unix: Threads\"" >> $GPI_FILE

gnuplot $GPI_FILE


for file in $SERVER_NAMES $CLIENT_NAMES; do
    rm /tmp/$file
done
rm $DATA_FILE
rm $GPI_FILE
rm /tmp/uds-echo.sock

exit 0
