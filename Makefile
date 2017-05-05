CXX      := g++
CC       := $(CXX)
CXXFLAGS := -W -Wall -O2 -g --std=c++11
ifeq ($(OS),Windows_NT)
PROGRAM  := dupfind.exe
DOS2UNIX := dos2unix
LDFLAGS  := --static
else
PROGRAM  := dupfind
DOS2UNIX := cat
LDFLAGS  :=
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
	rm -f *.o *.exe dupfind

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
	echo $(1)":" $(PROGRAM) $(2) $(3)
	cd tests/data && ../../$(PROGRAM) $(2) > ../$(1)/test-output.txt
	diff -w tests/$(1)/expected-test-output.txt tests/$(1)/test-output.txt
endef

test: $(PROGRAM)
	@$(call testcase,tc000,, "(Default)")
	@$(call testcase,tc001,-h, "(Help)")
	@$(call testcase,tc002,-e .rb, "(Files ending with)")
	@$(call testcase,tc003,\
                offense_count_formatter.rb worst_offenders_formatter.rb,\
                "(Name files on command line)")
	@$(call testcase,tc004,-e .rb -t, "(Total)")
	@$(call testcase,tc005,-e .rb -T, "(Total including test files)")
	@$(call testcase,tc006,-e .rb -v, "(Verbose)")
	@$(call testcase,tc007,-x offense_count -e .rb, "(Exclude)")
	@$(call testcase,tc008,-w -e .rb, "(Word mode)")
	@$(call testcase,tc009,-m200 -e .rb, "(Minimum length)")
	@$(call testcase,tc010,-8 -e .rb, "(Count)")
	@$(call testcase,tc011,-p15 -e .rb, "(Proximity)")
	@echo OK
