CXX      := g++
CC       := $(CXX)
CXXFLAGS := -W -Wall --static -O2

all: README.md

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
