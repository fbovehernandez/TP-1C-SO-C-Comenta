# Libraries
LIBS=utils commons pthread readline m

# Custom libraries' paths
STATIC_LIBPATHS=../utils

# Compiler flags
CDEBUG=-g -Wall -DDEBUG -fdiagnostics-color=always
CRELEASE=-O3 -Wall -DNDEBUG -fcommon

# Arguments when executing with start, memcheck or helgrind
ARGS="../entradasalida/stdin.config"
ARGS2 = "stdin.config"
ARGS3 = "stdin.config"
ARGS_STDIN = "stdin.config"
NOMBRE_INTERFAZ1 = caro
NOMBRE_INTERFAZ2 = mati
NOMBRE_INTERFAZ_STDIN = sofi

# Valgrind flags
MEMCHECK_FLAGS=--track-origins=yes
HELGRIND_FLAGS=
