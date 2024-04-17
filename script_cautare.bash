#!/bin/bash

fisier=$1

# Cautăm cuvântul "key" în fișier
if grep -q "corupted" "dangerous" "risk" "attack" "malware" "malicious" "$fisier"; then
    echo "Cuvântul 'key' a fost găsit în fișierul $fisier"
else
    echo "Cuvântul 'key' nu a fost găsit în fișierul $fisier"
fi
