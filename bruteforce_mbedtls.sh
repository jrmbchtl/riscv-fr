#!/bin/bash

for i in {-10..10..2}
do
    ./mbedtls-attack $i
done