# Libraries
LIBS=utils commons pthread readline m

# Custom libraries' paths
STATIC_LIBPATHS=../utils

# Compiler flags
CDEBUG=-g -Wall -DDEBUG -fdiagnostics-color=always
CRELEASE=-O3 -Wall -DNDEBUG -fcommon

# Arguments when executing with start, memcheck or helgrind
ARGS="/home/utnso/operativos/TPSCAFFOLD1/tp-2024-1c-Sofa-Cama/entradasalida/stdout.config"
ARGS2 = "/home/utnso/operativos/TPSCAFFOLD1/tp-2024-1c-Sofa-Cama/entradasalida/stdin.config"

# Valgrind flags
MEMCHECK_FLAGS=--track-origins=yes
HELGRIND_FLAGS=
