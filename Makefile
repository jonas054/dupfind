CXX      := g++
CC       := $(CXX)
CXXFLAGS := -W -Wall -O2 -g --std=c++11 $(EXTRA_FLAGS)
ifeq ($(OS),Windows_NT)
PROGRAM  := dupfind.exe
DOS2UNIX := dos2unix
LDFLAGS  := --static $(EXTRA_FLAGS)
else
PROGRAM  := dupfind
DOS2UNIX := cat
LDFLAGS  := $(EXTRA_FLAGS)
endif
SOURCE   := $(filter-out %_flymake.cc,$(wildcard *.cc))
OBJS     := $(patsubst %.cc,%.o,$(SOURCE))

default: $(PROGRAM)

all: test README.md

-include $(OBJS:.o=.o.d)

%.o: %.cc
	$(CXX) -c $(CXXFLAGS) $< -o $@
	@$(CXX) -MM $< > $@.d

$(PROGRAM): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	rm -f *.o *.o.d *.exe dupfind

README.md: $(PROGRAM)
	./$(PROGRAM) -h 2>&1 | $(DOS2UNIX) > help.txt
	awk '/# Usage/{flag=1;print;next} { if (!flag) print }' $@ > $@.1
	awk '/# Description/{flag=1; print ""} { if (flag) print }' $@ > $@.2
	mv $@.1 $@
	echo "\`\`\`" >> $@
	sed 's/^Usage: /       /' help.txt >> $@
	echo "\`\`\`" >> $@
	cat $@.2 >> $@
	rm $@.2 help.txt

define testcase
	printf "%s: %-28s %s %s\n" $(1) $(3) $(PROGRAM) "$(2)"
	cd tests/data && ../../$(PROGRAM) $(2) > ../$(1)/test-output.txt
	diff -w tests/$(1)/expected-test-output.txt tests/$(1)/test-output.txt
endef

SOME_RB_FILES := offense_count_formatter.rb worst_offenders_formatter.rb

test: $(PROGRAM)
	@$(call testcase,tc000,, "Default")
	@$(call testcase,tc001,-h,"Help")
	@$(call testcase,tc002,-e .rb,"Files ending with")
	@$(call testcase,tc002,-x a -e .cpp -e .rb,"Excluded C++ plus Ruby")
	@$(call testcase,tc003,$(SOME_RB_FILES),"Name files on command line")
	@$(call testcase,tc004,-e .rb -t,"Total")
	@$(call testcase,tc005,-e .rb -T,"Total including test files")
	@$(call testcase,tc006,-e .rb -v,"Verbose")
	@$(call testcase,tc007,-x offense_count -e .rb,"Exclude")
	@$(call testcase,tc008,-w -e .rb,"Word mode")
	@$(call testcase,tc009,-m200 -e .rb,"Minimum length single arg")
	@$(call testcase,tc009,-m 200 -e .rb,"Minimum length split arg")
	@$(call testcase,tc010,-8 -e .rb,"Count")
	@$(call testcase,tc011,-p15 -e .rb,"Proximity single arg")
	@$(call testcase,tc011,-p 15 -e .rb,"Proximity split arg")
	@$(call testcase,tc012,-t -e .cpp,"C++ total")
	@$(call testcase,tc013,-v -e .cpp,"C++ verbose")
	@$(call testcase,tc014,-e .cpp -e .rb,"C++ and Ruby")
	@$(call testcase,tc015,-v -e .erl,"Erlang verbose")
	@echo OK
