BIN=./practica2SG
MAP_FOLDER=./mapas
NUM_TESTS=100
LEVEL=0

if [[ -z "$1" ]]; then
    echo "Uso: $0 <fichero_salida.test>"
    exit 1
fi

OUTPUT_TEST_FILE="$1"
VALID_TEST_NUM=1
PASSED_TESTS=0
TOTAL_RUNS=0
FAILED_TESTS=()

check_output() {
    local output="$1"
    if echo "$output" | grep -q "Misión completada: ambos agentes han alcanzado el puesto base"; then
        return 0
    else
        return 2
    fi
}

generate_orientation() {
    echo $((RANDOM % 8))
}

get_valid_positions() {
    local mapfile="$1"
    local filas="$2"
    local columnas="$3"

    map_lines=()
    while IFS= read -r line; do
        map_lines+=("$line")
    done < <(tail -n +3 "$mapfile")

    valid_positions=()
    for ((row=3; row<filas-3; row++)); do
        for ((col=3; col<columnas-3; col++)); do
            cell="${map_lines[$row]:$col:1}"
            if [[ "$cell" == "C" ]]; then
                valid_positions+=("$row $col")
            fi
        done
    done
}

MAPS=($(find $MAP_FOLDER -name "*.map" ! -name "scape25.map" ! -name "paldea25.map" ! -name "pruebaDosUnicos_Zapas.map"))

for ((i=1; i<=NUM_TESTS; i++)); do
    MAP_FILE=$(shuf -n 1 -e "${MAPS[@]}")
    filas=$(head -n 1 "$MAP_FILE")
    columnas=$(head -n 2 "$MAP_FILE" | tail -n 1)

    get_valid_positions "$MAP_FILE" "$filas" "$columnas"
    if [[ ${#valid_positions[@]} -lt 2 ]]; then
        continue
    fi

    idx1=$((RANDOM % ${#valid_positions[@]}))
    idx2=$idx1
    while [[ $idx2 -eq $idx1 ]]; do
        idx2=$((RANDOM % ${#valid_positions[@]}))
    done

    read row1 col1 <<< "${valid_positions[$idx1]}"
    read row2 col2 <<< "${valid_positions[$idx2]}"
    ori1=$(generate_orientation)
    ori2=$(generate_orientation)
    seed=$RANDOM

    output=$($BIN "$MAP_FILE" "$seed" "$LEVEL" "$row1" "$col1" "$ori1" "$row2" "$col2" "$ori2" 2>&1)
    check_output "$output"
    result=$?
    ((TOTAL_RUNS++))

    if [[ $result -ne 1 ]]; then
        TEST_CMD="$BIN \"$MAP_FILE\" $seed $LEVEL $row1 $col1 $ori1 $row2 $col2 $ori2"

        if [[ $result -eq 0 ]]; then
            echo "✅ Test $VALID_TEST_NUM : $TEST_CMD"
            ((PASSED_TESTS++))
        else
            echo "❌ Test $VALID_TEST_NUM : $TEST_CMD"
            FAILED_TESTS+=("$TEST_CMD")
        fi

        # Guardar test al final del archivo
        echo "$TEST_CMD" >> "$OUTPUT_TEST_FILE"

        ((VALID_TEST_NUM++))
    fi
done

echo "========== RESUMEN =========="
echo "Tests ejecutados: $TOTAL_RUNS"
echo "Tests correctos:  $PASSED_TESTS"
echo "Porcentaje:       $((PASSED_TESTS * 100 / TOTAL_RUNS))%"

if [[ ${#FAILED_TESTS[@]} -gt 0 ]]; then
    echo ""
    echo "❌ Tests con fallos:"
    for fail in "${FAILED_TESTS[@]}"; do
        echo "- $fail"
    done
fi