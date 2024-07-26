include settings.mk

# Set compiler options
CC=gcc

# Set prerrequisites
SRCS_C += $(shell find src/ -iname "*.c")
SRCS_H += $(shell find src/ -iname "*.h")
DEPS = $(foreach STL,$(STATIC_LIBPATHS),$(STL:%=%/bin/lib$(notdir $(STL)).a))

# Set header paths to (-I)nclude
IDIRS += $(addsuffix /src,$(STATIC_LIBPATHS) .)

# Set library paths to (-L)ook
LIBDIRS = $(addsuffix /bin,$(STATIC_LIBPATHS))

# Set intermediate objects
OBJS = $(patsubst src/%.c,obj/%.o,$(SRCS_C))

# Set binary targets
BIN = bin/$(shell cd . && pwd | xargs basename)

.PHONY: all
all: debug

.PHONY: debug
debug: CFLAGS = $(CDEBUG)
debug: $(BIN)

.PHONY: release
release: CFLAGS = $(CRELEASE)
release: $(BIN)

.PHONY: clean
clean:
	-rm -rfv obj bin
	-rm -f dialfs/bitmap.dat
	-rm -f dialfs/bloques.dat

$(BIN): $(OBJS) | $(dir $(BIN))
	$(CC) $(CFLAGS) -o "$@" $^ $(IDIRS:%=-I%) $(LIBDIRS:%=-L%) $(LIBS:%=-l%)

obj/%.o: src/%.c $(SRCS_H) $(DEPS) | $(dir $(OBJS))
	$(CC) $(CFLAGS) -c -o "$@" $< $(IDIRS:%=-I%)

.SECONDEXPANSION:
$(DEPS): $$(shell find $$(patsubst %bin/,%src/,$$(dir $$@)) -iname "*.c" -or -iname "*.h")
	make --no-print-directory -C $(patsubst %bin/,%,$(dir $@)) 3>&1 1>&2 2>&3 | sed -E 's,(src/)[^ ]+\.(c|h)\:,$(patsubst %bin/,%,$(dir $@))&,' 3>&2 2>&1 1>&3

$(sort $(dir $(BIN) $(OBJS))):
	mkdir -pv $@

.PHONY: start
start: debug
	valgrind --tool=none ./$(BIN) $(NOMBRE_INTERFAZ) $(ARGS)

.PHONY: start2
start2: debug
	valgrind --tool=none ./$(BIN) $(GENERICA) $(NOMBRE_INTERFAZ1) 

.PHONY: start3
start3: debug
	valgrind --tool=none ./$(BIN) $(ARGS3) $(NOMBRE_INTERFAZ2) 

.PHONY: start_stdin
start_stdin: debug
	valgrind --tool=none ./$(BIN) $(TECLADO) $(NOMBRE_INTERFAZ_STDIN) 

PHONY: start_stdout
start_stdout: debug
	valgrind --tool=none ./$(BIN) $(MONITOR) $(NOMBRE_INTERFAZ_STDOUT) 

PHONY: start_dialfs
start_dialfs: debug
	valgrind --tool=none ./$(BIN) $(ARGS_DIALFS) $(NOMBRE_INTERFAZ_DIALFS) 

.PHONY: start_SLP1
start_SLP1: debug
	valgrind --tool=none ./$(BIN) $(ARGS_SLP1) $(NOMBRE_INTERFAZ_SLP1) 

.PHONY: start_ESPERA
start_ESPERA: debug
	valgrind --tool=none ./$(BIN) $(ARGS_ESPERA) $(NOMBRE_INTERFAZ_ESPERA) 


.PHONY: memcheck
memcheck: 
	valgrind --leak-check=full $(MEMCHECK_FLAGS) ./$(BIN) $(ARGS)

.PHONY: memcheck2
memcheck2: debug
	valgrind --leak-check=full $(MEMCHECK_FLAGS) ./$(BIN) $(ARGS2) $(NOMBRE_INTERFAZ1)

.PHONY: memcheck_SLP1
memcheck_SLP1: debug
	valgrind --leak-check=full $(MEMCHECK_FLAGS) ./$(BIN) $(ARGS_SLP1) $(NOMBRE_INTERFAZ_SLP1)

.PHONY: memcheck_generica
memcheck_generica: debug
	valgrind --leak-check=full $(MEMCHECK_FLAGS) ./$(BIN) $(GENERICA) $(NOMBRE_INTERFAZ1)

.PHONY: memcheck_stdin
memcheck_stdin: debug
	valgrind --leak-check=full $(MEMCHECK_FLAGS) ./$(BIN) $(TECLADO) $(NOMBRE_INTERFAZ_STDIN)

.PHONY: memcheck_stdout
memcheck_stdout: debug
	valgrind --leak-check=full $(MEMCHECK_FLAGS) ./$(BIN) $(MONITOR) $(NOMBRE_INTERFAZ_STDOUT)

.PHONY: memcheck_espera
memcheck_espera: debug
	valgrind --leak-check=full $(MEMCHECK_FLAGS) ./$(BIN) $(ARGS_ESPERA) $(NOMBRE_INTERFAZ_ESPERA)


.PHONY: helgrind
helgrind: debug
	valgrind --tool=helgrind $(HELGRIND_FLAGS) ./$(BIN) $(ARGS)
