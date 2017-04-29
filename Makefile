ifeq ($(OS),Windows_NT)
OSTYPE := $(OS)
PROGRAM := $(OSTYPE)/dupfind.exe
DOS2UNIX := dos2unix
else
OSTYPE := $(_system_type)
PROGRAM := $(OSTYPE)/dupfind
DOS2UNIX := cat
endif

CXX      := g++
CC       := $(CXX)
CXXFLAGS := -W -Wall -O2
SOURCE   := $(wildcard *.cc)
OBJS     := $(patsubst %.cc,$(OSTYPE)/%.o,$(SOURCE))

default: $(PROGRAM)

all: test README.md

-include $(OBJS:.o=.o.d)

$(OSTYPE)/%.o: %.cc
	@mkdir -p $(OSTYPE)
	$(CXX) -c $(CXXFLAGS) $< -o $@
	@$(CXX) -MM $< | sed "s-^\(\w*\.o:\)-$(OSTYPE)/\1-" > $@.d

$(PROGRAM): $(OBJS)
	$(CXX) --static -o $@ $^

clean:
	rm -rf $(OSTYPE)

README.md: $(PROGRAM)
	./$(PROGRAM) -h 2>&1 | $(DOS2UNIX) > help.txt
	awk '/# Usage/{flag=1;print;next} { if (!flag) print }' $@ > $@.new
	mv $@.new $@
	echo "\`\`\`" >> $@
	sed 's/^Usage: /       /' help.txt >> $@
	echo "\`\`\`" >> $@
	rm help.txt

# Help
TC_001_CMD := $(PROGRAM) -h
# Files ending with
TC_002_CMD := $(PROGRAM) -e .rb
# Name files on command line
TC_003_CMD := $(PROGRAM) offense_count_formatter.rb worst_offenders_formatter.rb
# Total
TC_004_CMD := $(PROGRAM) -e .rb -t
# Total including test files
TC_005_CMD := $(PROGRAM) -e .rb -T
# Verbose
TC_006_CMD := $(PROGRAM) -e .rb -v
# Exclude
TC_007_CMD := $(PROGRAM) -x offense_count -e .rb
# Word mode
TC_008_CMD := $(PROGRAM) -w -e .rb
# Minimum length
TC_009_CMD := $(PROGRAM) -m200 -e .rb
# Count
TC_010_CMD := $(PROGRAM) -8 -e .rb
# Proximity
TC_011_CMD := $(PROGRAM) -p15 -e .rb

test: $(PROGRAM)
	@echo TC 001: $(TC_001_CMD)
	@./$(TC_001_CMD) 2> tests/tc001/test-output.txt || true
	@diff -w tests/tc001/expected-test-output.txt tests/tc001/test-output.txt
	@echo TC 002: $(TC_002_CMD)
	@cd tests/data && ../../$(TC_002_CMD) > ../tc002/test-output.txt
	@diff -w tests/tc002/expected-test-output.txt tests/tc002/test-output.txt
	@echo TC 003: $(TC_003_CMD)
	@cd tests/data && ../../$(TC_003_CMD) > ../tc003/test-output.txt
	@diff -w tests/tc003/expected-test-output.txt tests/tc003/test-output.txt
	@echo TC 004: $(TC_004_CMD)
	@cd tests/data && ../../$(TC_004_CMD) > ../tc004/test-output.txt
	@diff -w tests/tc004/expected-test-output.txt tests/tc004/test-output.txt
	@echo TC 005: $(TC_005_CMD)
	@cd tests/data && ../../$(TC_005_CMD) > ../tc005/test-output.txt
	@diff -w tests/tc005/expected-test-output.txt tests/tc005/test-output.txt
	@echo TC 006: $(TC_006_CMD)
	@cd tests/data && ../../$(TC_006_CMD) > ../tc006/test-output.txt
	@diff -w tests/tc006/expected-test-output.txt tests/tc006/test-output.txt
	@echo TC 007: $(TC_007_CMD)
	@cd tests/data && ../../$(TC_007_CMD) > ../tc007/test-output.txt
	@diff -w tests/tc007/expected-test-output.txt tests/tc007/test-output.txt
	@echo TC 008: $(TC_008_CMD)
	@cd tests/data && ../../$(TC_008_CMD) > ../tc008/test-output.txt
	@diff -w tests/tc008/expected-test-output.txt tests/tc008/test-output.txt
	@echo TC 009: $(TC_009_CMD)
	@cd tests/data && ../../$(TC_009_CMD) > ../tc009/test-output.txt
	@diff -w tests/tc009/expected-test-output.txt tests/tc009/test-output.txt
	@echo TC 010: $(TC_010_CMD)
	@cd tests/data && ../../$(TC_010_CMD) > ../tc010/test-output.txt
	@diff -w tests/tc010/expected-test-output.txt tests/tc010/test-output.txt
	@echo TC 011: $(TC_011_CMD)
	@cd tests/data && ../../$(TC_011_CMD) > ../tc011/test-output.txt
	@diff -w tests/tc011/expected-test-output.txt tests/tc011/test-output.txt
	@echo OK
