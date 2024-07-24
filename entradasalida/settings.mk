# Libraries
LIBS=utils commons pthread readline m

# Custom libraries' paths
STATIC_LIBPATHS=../utils

# Compiler flags
CDEBUG=-g -Wall -DDEBUG -fdiagnostics-color=always
CRELEASE=-O3 -Wall -DNDEBUG -fcommon

# Arguments when executing with start, memcheck or helgrind
ARGS="../entradasalida/stdin.config"
GENERICA = "generica.config"
ARGS3 = "generica2.config"
ARGS_SLP1 = "SLP1.config" # "ESPERA.config"
NOMBRE_INTERFAZ_SLP1 = SLP1  	 # "ESPERA"
TECLADO = "stdin.config"
MONITOR = "stdout.config"
ARGS_STDOUT = "stdout.config"
ARGS_DIALFS = "dialfs.config"
NOMBRE_INTERFAZ1 = generica
NOMBRE_INTERFAZ2 = caro
NOMBRE_INTERFAZ_STDIN = teclado
NOMBRE_INTERFAZ_STDOUT = monitor
NOMBRE_INTERFAZ_DIALFS = ara_dialfs 

# Valgrind flags
MEMCHECK_FLAGS=--track-origins=yes
HELGRIND_FLAGS=
