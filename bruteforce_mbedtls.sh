#!/bin/bash

for i in {-10..10..2}
do
    echo "Trying $i"
    ./mbedtls-attack $i
done