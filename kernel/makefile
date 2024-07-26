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

$(BIN): $(OBJS) | $(dir $(BIN))
	$(CC) $(CFLAGS) -o "$@" $^ $(IDIRS:%=-I%) $(LIBDIRS:%=-L%) $(LIBS:%=-l%)

obj/%.o: src/%.c $(SRCS_H) $(DEPS) | $(dir $(OBJS))
	$(CC) $(CFLAGS) -c -o "$@" $< $(IDIRS:%=-I%)

.SECONDEXPANSION:
$(DEPS): $$(shell find $$(patsubst %bin/,%src/,$$(dir $$@)) -iname "*.c" -or -iname "*.h")
	make --no-print-directory -C $(patsubst %bin/,%,$(dir $@)) 3>&1 1>&2 2>&3 | sed -E 's,(src/)[^ ]+\.(c|h)\:,$(patsubst %bin/,%,$(dir $@))&,' 3>&2 2>&1 1>&3

$(sort $(dir $(BIN) $(OBJS))):
	mkdir -pv $@

# Modificado para aceptar argumentos
.PHONY: start
start: debug
	$(eval ARGS := $(filter-out $@,$(MAKECMDGOALS)))
	valgrind --tool=none ./$(BIN) $(ARGS)

.PHONY: memcheck
memcheck: debug
	$(eval ARGS := $(filter-out $@,$(MAKECMDGOALS)))
	valgrind --leak-check=full $(MEMCHECK_FLAGS) ./$(BIN) $(ARGS)

.PHONY: helgrind
helgrind: debug
	$(eval ARGS := $(filter-out $@,$(MAKECMDGOALS)))
	valgrind --tool=helgrind $(HELGRIND_FLAGS) ./$(BIN) $(ARGS)

# Evitar que make interprete argumentos adicionales como objetivos
%:
	@:

# Definir reglas específicas para pruebas
.PHONY: salvation_edge
salvation_edge: 
	make start salvations_edge.config salvations_edge.log

.PHONY: memoria_fifo
memoria_fifo: 
	make start memoria_y_tlb.config tlb_fifo.log

.PHONY: memoria_lru
memoria_lru: 
	make start memoria_y_tlb.config tlb_lru.log

.PHONY: io
io: 
	make start io.config io.log

.PHONY: fs
fs:
	make start fs.config fs.log

.PHONY: plani_fifo
plani_fifo:
	make start plani_fifo.config plani_fifo.log

.PHONY: plani_rr
plani_rr:
	make start plani_rr.config plani_rr.log

.PHONY: plani_vrr
plani_vrr:
	make start plani_vrr.config plani_vrr.log
	
.PHONY: deadlock
deadlock:
	make start deadlock.config deadlock.log