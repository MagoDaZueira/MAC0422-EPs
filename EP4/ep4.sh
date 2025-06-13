#!/bin/bash

CLIENT_NUM=$1
FILE_SIZES=${@:2}

SERVER_NAMES="ep4-servidor-inet_processos ep4-servidor-inet_threads ep4-servidor-inet_muxes ep4-servidor-unix_threads"
CLIENT_NAMES="ep4-cliente-inet ep4-cliente-unix"

for file in $SERVER_NAMES $CLIENT_NAMES; do
    echo "Compilando $file"
    gcc ep4-clientes+servidores/$file.c -o /tmp/$file
done

for size in $FILE_SIZES; do
    echo ">>>>>>> Gerando um arquivo texto de: ${size}MB..."
    bytes=$((size * 1048576))
    base64 /dev/urandom | head -c $bytes > /tmp/${size}MB.txt
    echo >> /tmp/${size}MB.txt
done
