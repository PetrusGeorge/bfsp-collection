#!/bin/bash

# Configurações
RESULTS_FILE="results.txt"
ALGORITHM_EXEC="./build/algorithms"
INSTANCES_DIR="./instances/J200M10"

# Cores para terminal
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Verificações iniciais
if [ ! -d "$INSTANCES_DIR" ]; then
    echo -e "${RED}ERRO: Diretório '$INSTANCES_DIR' não encontrado!${NC}"
    exit 1
fi

if [ ! -f "$ALGORITHM_EXEC" ]; then
    echo -e "${RED}ERRO: Executável '$ALGORITHM_EXEC' não encontrado!${NC}"
    exit 1
fi


# Função para processar uma instância
process_instance() {
    local instance_path="$1"
    local instance_name=$(basename "$instance_path")
    
    echo -e "${GREEN}Processando: ${instance_name}${NC}"
    
    # Executa o algoritmo e captura a saída
    local output
    if ! output=$("$ALGORITHM_EXEC" "$instance_path" 2>&1); then
        echo -e "${RED}Erro na execução: ${instance_name}${NC}"
        return 1
    fi

    # Extrai makespan e sequência
    local makespan=$(echo "$output" | grep "Best makespan" | awk '{print $4}')
    local sequence=$(echo "$output" | grep "Best sequence" | cut -d':' -f2- | sed 's/^ *//;s/ *$//')

    if [ -z "$makespan" ] || [ -z "$sequence" ]; then
        echo -e "${YELLOW}AVISO: Formato inválido na saída para ${instance_name}${NC}"
        return 1
    fi

    # Salva os resultados
    printf "%-15s | Makespan: %-6s | Sequence: %s\n" "$instance_name" "$makespan" "$sequence" >> "$RESULTS_FILE"
}

# Busca recursiva por instâncias
echo "Buscando instâncias em '$INSTANCES_DIR/'..."
find "$INSTANCES_DIR" -type f | while read -r instance; do
    process_instance "$instance"
done

echo -e "\n${GREEN}Concluído! Resultados salvos em: $RESULTS_FILE${NC}"