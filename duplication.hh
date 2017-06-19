#ifndef DUPLICATION_HH
#define DUPLICATION_HH

struct Duplication
{
    Duplication(): instances(0), longestSame(0), indexOf1stInstance(0) {}

    int instances;
    int longestSame;
    int indexOf1stInstance;
};

#endif
