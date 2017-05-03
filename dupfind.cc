//=============================================================================
//
// File: dupfind.cc
//
// Finds and reports duplication in source code or other text files.
//
// Copyright (C) 2002-2017 Jonas Arvidsson
//
//=============================================================================

#include <cstdlib>    // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream>   // cout, endl
#include <cstring>    // strlen

#include "dupfind.hh"
#include "duplication.hh"
#include "bookmark.hh"
#include "bookmark_container.hh"
#include "options.hh"
#include "parser.hh"

using std::cout;

int Dupfind::run(int argc, char* argv[])
{
    itsOptions.parse(argc, argv);

    Parser      parser;
    const char* processed = parser.process(itsContainer, itsOptions.wordMode);
    itsProcessedEnd = processed + strlen(processed);

    itsContainer.sort();

    for (int count = 0; count < itsOptions.nrOfWantedReports; ++count)
        if (not reportOne())
            break;

    if (itsOptions.totalReport != Options::NO_TOTAL)
    {
        const int length = itsProcessedEnd - processed;
        cout << "Duplication = " << Bookmark::getTotalNrOfLines() << " lines, "
             << (100 * itsTotalDuplication + length / 2) / length << " %\n";
    }
    delete [] processed;
    return 0;
}

/**
 * Reports one duplication, two or more instances. Returns true if a report was
 * made, false if no big enough duplication could be found.
 */
bool Dupfind::reportOne()
{
    Duplication worst = findWorst();
    if (worst.instances == 0)
        return false;

    // Report all found instances (exact and approximate matches).
    for (int i = 0; i < worst.instances; ++i)
        itsContainer.report(worst.indexOf1stInstance + i, worst, i + 1,
                            itsOptions);
    cout << std::endl;

    itsTotalDuplication += worst.longestSame * worst.instances;

    // Clear bookmarks that point to something within the reported area.
    // This is to avoid reporting the same section more than once.
    itsContainer.clearWithin(worst);
    return true;
}

Duplication Dupfind::findWorst() const
{
    Duplication result;

    // Find the two bookmarks that have the longest common substring.
    for (size_t markIx = 0; markIx < itsContainer.size() - 1; ++markIx)
        if (itsContainer.same(markIx, markIx + 1, result.longestSame,
                              itsProcessedEnd))
        {
            const int same = itsContainer.nrOfSame(markIx, markIx + 1);
            if (same > result.longestSame)
            {
                result.indexOf1stInstance = markIx;
                result.longestSame        = same;
            }
        }

    if (result.longestSame >= itsOptions.minLength)
    {
        int almostLongest =
            (result.longestSame * itsOptions.proximityFactor) / 100;

        // Look for approximate matches in strings just before the current
        // pair.
        int stepsBackward = expandSearch(result, almostLongest, -1, -1);

        // Look for approximate matches in strings just after the current pair.
        int stepsForward = expandSearch(result, almostLongest, 2, 1);
        result.instances = 2 + stepsBackward + stepsForward;
        result.indexOf1stInstance -= stepsBackward;
    }
    return result;
}

int Dupfind::expandSearch(Duplication& duplication,
                          int          almostLongest,
                          int          startingPoint,
                          int          loopIncrement) const
{
    int steps = 0;
    for (int i = duplication.indexOf1stInstance + startingPoint;
         i >= 0 && i < int(itsContainer.size());
         i += loopIncrement)
    {
        const int same = itsContainer.nrOfSame(duplication.indexOf1stInstance,
                                               i);
        if (same < almostLongest)
            break;
        steps++;
        if (duplication.longestSame > same)
            duplication.longestSame = same;
    }
    return steps;
}

int main(int argc, char* argv[])
{
    Dupfind d;
    return d.run(argc, argv);
}
