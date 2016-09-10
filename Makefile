# Written by Ivan Pogrebnyak

# TODO: fix problem with making in multiple threads

CXX := g++
STD := -std=c++11
DF := $(STD)
CF := $(STD) -Wall #-Og #-flto
LF := $(STD) #-flto

CF += -Iinclude
DF += -Iinclude
# LF +=

SRC := test
BIN := test
BLD := .build

SRCS := $(shell find $(SRC) -type f -name '*.cc')
DEPS := $(patsubst $(SRC)%.cc,$(BLD)%.d,$(SRCS))

GREP_EXES := grep -rl '^ *int \+main *(' $(SRCS)
EXES := $(patsubst $(SRC)%.cc,$(BIN)%,$(shell $(GREP_EXES)))

NODEPS := clean
.PHONY: all clean

all: $(EXES)

test/args_parse: $(BLD)/args_parse_src.o

$(BLD)/args_parse_src.o: src/args_parse.cc include/args_parse.hh

#Don't create dependencies when we're cleaning, for instance
ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
-include $(DEPS)
endif

$(DEPS): $(BLD)/%.d: $(SRC)/%.cc | $(BLD)
	$(CXX) $(DF) -MM -MT '$(@:.d=.o)' $< -MF $@

$(BLD)/%.o: | $(BLD)
	$(CXX) $(CF) $(C_$*) -c $(filter %.cc,$^) -o $@

$(BIN)/%: $(BLD)/%.o | $(BIN)
	$(CXX) $(LF) $(filter %.o,$^) -o $@ $(L_$*)

$(BLD) $(BIN):
	mkdir $@

clean:
	@rm -rfv $(BLD) $(EXES)
