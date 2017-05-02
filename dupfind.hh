#ifndef DUPFIND_HH
#define DUPFIND_HH

#include "bookmark_container.hh"
#include "options.hh"

class Dupfind
{
    struct Duplication
    {
        Duplication(): instances(0), longestSame(0), indexOf1stInstance(0) {}
        
        int instances;
        int longestSame;
        int indexOf1stInstance;
    };

public:
    Dupfind(): itsTotalDuplication(0) {}

    int run(int argc, char* argv[]);

private:
    int expandSearch(Duplication& duplication,
                     int          almostLongest,
                     int          startingPoint,
                     int          loopIncrement);

    Duplication findWorst(const char* processedEnd);

    bool reportOne(const char* processedEnd);

    Options           itsOptions;
    BookmarkContainer itsContainer;
    int               itsTotalDuplication;
};

#endif
