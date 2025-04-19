#!/bin/bash

BIN=./practica2SG
LEVEL=0

if [[ -z "$1" ]]; then
    echo "Uso: $0 <archivo_con_tests.test>"
    exit 1
fi

TEST_FILE="$1"
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

check_output2() {
    local output="$1"
    if echo "$output" | grep -q "Error en la colocación"; then
        return 1
    fi
    if echo "$output" | grep -q "Posicion ("; then
        return 1
    fi
    if echo "$output" | grep -q "El rescatador ha alcanzado un puesto base" &&
       echo "$output" | grep -q "El auxiliar ha alcanzado un puesto base" &&
       echo "$output" | grep -q "Misión completada: ambos agentes han alcanzado el puesto base"; then
        return 0
    else
        return 2
    fi
}

while IFS= read -r line || [ -n "$line" ]; do
    # Saltar líneas vacías o comentarios
    if [[ -z "$line" || "$line" =~ ^# ]]; then
        continue
    fi

    ((TOTAL_RUNS++))

    output=$(eval "$line" 2>&1)
    check_output "$output"
    result=$?

    if [[ $result -eq 0 ]]; then
        echo "✅ Test $VALID_TEST_NUM : $line"
        ((PASSED_TESTS++))
    else
        echo "❌ Test $VALID_TEST_NUM : $line"
        FAILED_TESTS+=("$line")
    fi

    ((VALID_TEST_NUM++))

done < "$TEST_FILE"

echo ""
echo "========== RESUMEN =========="
echo "Tests ejecutados: $TOTAL_RUNS"
echo "Tests correctos:  $PASSED_TESTS"
if [[ $TOTAL_RUNS -gt 0 ]]; then
    porcentaje=$((PASSED_TESTS * 100 / TOTAL_RUNS))
else
    porcentaje=0
fi
echo "Porcentaje:       ${porcentaje}%"

if [[ ${#FAILED_TESTS[@]} -gt 0 ]]; then
    echo ""
    echo "❌ Tests con fallos:"
    for fail in "${FAILED_TESTS[@]}"; do
        echo "- $fail"
    done
fi