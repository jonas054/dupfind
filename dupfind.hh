#ifndef DUPFIND_HH
#define DUPFIND_HH

#include "bookmark_container.hh"
#include "options.hh"

struct Duplication;

class Dupfind
{
public:
    Dupfind(): itsTotalDuplication(0) {}

    int run(int argc, char* argv[]);

private:
    bool reportOne();

    Duplication findWorst() const;

    int expandSearch(Duplication& duplication,
                     int          almostLongest,
                     int          startingPoint,
                     int          loopIncrement) const;

    Options           itsOptions;
    BookmarkContainer itsContainer;
    int               itsTotalDuplication;
    const char*       itsProcessedEnd;
};

#endif
