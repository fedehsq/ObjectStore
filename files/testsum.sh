#!/bin/bash


# quale batteria
battery=0

# client lanciati
client=0

# array di 6 elementi con esiti delle batterie
arr=(0 0 0 0 0 0)
# arr[1] = successi batteria 1
# arr[2] = successi batteria 2
# arr[3] = successi batteria 3
# arr[4] = falliementi batteria 1
# arr[5] = fallimenti batteria 2
# arr[6] = fallimenti batteria 3

getNum() {
    found=0
    for word in $1; do
        # numero a fine riga
        if [ $found -eq 1 ]; then
            (( $2+=$word ))
            break
        fi
        # la parola successiva sarà il numero cercato
        if [ "$word" = ":" ]; then
            found=1
        fi
    done
}

# per ogni riga del file
while read line; do
    # controllo quale batteria
    if [[ "$line" == "Batteria"* ]]; then
        battery=0
        getNum "$line" battery
        continue
    fi

    # conto i client
    if [[ "$line" == "User"* ]]; then
        (( client++ ))
        continue
    fi

    # Operazioni success per batteria 
    if [[ "$line" == "Operazioni effettuate con successo"* ]]; then
        # batteria di test
        case $battery in
            1) getNum "$line" arr[1];;
            2) getNum "$line" arr[2];;
            3) getNum "$line" arr[3];;
        esac
        continue
    fi

    # Operazioni fail per batteria
    if [[ "$line" == "Operazioni fallite"* ]]; then
        # batteria di test
        case $battery in
            1) getNum "$line" arr[4];;
            2) getNum "$line" arr[5];;
            3) getNum "$line" arr[6];;
        esac
        continue
    fi

done < testout.log

cat testout.log

echo "Client lanciati: $client"
echo "Operazioni effettuate per batteria 1: $(( ${arr[1]}+${arr[4]} ))"
echo "Opreazioni eseguite con successo per batteria 1: ${arr[1]}"
echo "Opreazioni fallite per batteria 1: ${arr[4]}"

echo "Operazioni effettuate per batteria 2: $(( ${arr[2]}+${arr[5]} ))"
echo "Opreazioni eseguite con successo per batteria 2: ${arr[2]}"
echo "Opreazioni fallite per batteria 2: ${arr[5]}"

echo "Operazioni effettuate per batteria 3: $(( ${arr[3]}+${arr[6]} ))"
echo "Opreazioni eseguite con successo per batteria 3: ${arr[3]}"
echo "Opreazioni fallite per batteria 3: ${arr[6]}"