CXX      := g++
CC       := $(CXX)
CXXFLAGS := -W -Wall --static -O2

default: dupfind

all: test README.md

dupfind: dupfind.cc
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f *.o *.exe

README.md: dupfind
	./dupfind -h 2>&1 | dos2unix > help.txt
	awk '/# Usage/{flag=1;print;next} { if (!flag) print }' $@ > $@.new
	mv $@.new $@
	echo "\`\`\`" >> $@
	sed 's/^Usage: /       /' help.txt >> $@
	echo "\`\`\`" >> $@
	rm help.txt

TC_001_CMD := dupfind -h
TC_002_CMD := dupfind -e .rb
TC_003_CMD := dupfind offense_count_formatter.rb worst_offenders_formatter.rb
TC_004_CMD := dupfind -e .rb -t
TC_005_CMD := dupfind -e .rb -T

test: dupfind
	@echo TC 001: $(TC_001_CMD)
	@./$(TC_001_CMD) 2> tests/tc001/test-output.txt || true
	@diff tests/tc001/expected-test-output.txt tests/tc001/test-output.txt
	@echo TC 002: $(TC_002_CMD)
	@cd tests/data && ../../$(TC_002_CMD) > ../tc002/test-output.txt
	@diff tests/tc002/expected-test-output.txt tests/tc002/test-output.txt
	@echo TC 003: $(TC_003_CMD)
	@cd tests/data && ../../$(TC_003_CMD) > ../tc003/test-output.txt
	@diff tests/tc003/expected-test-output.txt tests/tc003/test-output.txt
	@echo TC 004: $(TC_004_CMD)
	@cd tests/data && ../../$(TC_004_CMD) > ../tc004/test-output.txt
	@diff tests/tc004/expected-test-output.txt tests/tc004/test-output.txt
	@echo TC 005: $(TC_005_CMD)
	@cd tests/data && ../../$(TC_005_CMD) > ../tc005/test-output.txt
	@diff tests/tc005/expected-test-output.txt tests/tc005/test-output.txt
	@echo OK
