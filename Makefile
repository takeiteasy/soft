OUTD=build
NAME=sftwrend
SRC=src/$(NAME).c
OBJ=$(OUTD)/$(NAME).o
ifeq ($(OS), Windows_NT)
	CC=cl
	LIBEXT=dll
else
	UNAME_S:=$(shell uname -s)
	CC=clang
	ifeq ($(UNAME_S), Darwin)
		LIBEXT=dylib
	else
		LIBEXT=so
	endif
endif
LIB=$(OUTD)/lib$(NAME).$(LIBEXT)
DOCD=docs
DOUT=$(DOCD)/index.html
WOUT=$(OUTD)/wasm
JSOUT=$(WOUT)/$(NAME).js
TESTD=tests

all: default wasm tests

default: $(OUTD) $(LIB) docs

$(OUTD):
	mkdir $(OUTD)

$(OBJ): $(SRC) 
	$(CC) -Iinclude -c $< -o $@

$(LIB): $(OBJ)
	$(CC) -o $@ -shared -fpic $^

docs: $(DOUT)

$(DOUT):
	headerdoc2html -udpb include/$(NAME).h -o $(DOCD)
	gatherheaderdoc $(DOCD)
	mv $(DOCD)/masterTOC.html $(DOUT)
	
wasm: $(WOUT) $(JSOUT)

$(WOUT):
	mkdir -p $(WOUT)

$(JSOUT):
	emcc -O3 -Iinclude -s WASM=1 $(SRC) -o $(JSOUT)

tests:
	cd $(TESTD) && make
	cd $(TESTD) && make clean

reset:
	rm -f $(OBJ) $(LIB) $(DOUT)
	rm -rf $(DOCD)/$(NAME)_h 
	cd $(TESTD) && make clean

clean:
	rm -f $(OBJ)
	cd $(TESTD) && make clean

.PHONY: all default reset clean tests
