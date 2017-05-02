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

#include "bookmark.hh"
#include "bookmark_container.hh"
#include "options.hh"
#include "parser.hh"

using std::cout;

struct Duplication
{
    Duplication(): instances(0), longestSame(0), indexOf1stInstance(0) {}

    int instances;
    int longestSame;
    int indexOf1stInstance;
};

static int expandSearch(const BookmarkContainer&, Duplication&, int, int, int);
static Duplication findWorst(const BookmarkContainer&,
                             const Options&,
                             const char*);
static bool reportOne(BookmarkContainer&, const Options&, const char*, int&);

int main(int argc, char* argv[])
{
    Options           options(argc, argv);
    BookmarkContainer container;
    const char*       processed = Parser::process(container,
                                                  options.wordMode);
    const char*       processedEnd = processed + strlen(processed);
    int               totalDuplication = 0;

    container.sort();

    for (int count = 0; count < options.nrOfWantedReports; ++count)
    {
        if (not reportOne(container, options, processedEnd, totalDuplication))
            break;
    }

    if (options.totalReport != Options::NO_TOTAL)
    {
        const int length = processedEnd - processed;
        cout << "Duplication = " << Bookmark::getTotalNrOfLines() << " lines, "
             << (100 * totalDuplication + length / 2) / length << " %\n";
    }
    delete [] processed;
    return 0;
}

/**
 * Reports one duplication, two or more instances. Returns true if a report was
 * made, false if no big enough duplication could be found.
 */
static bool reportOne(BookmarkContainer& container,
                      const Options&     options,
                      const char*        processedEnd,
                      int&               totalDuplication)
{
    Duplication worst = findWorst(container, options, processedEnd);
    if (worst.instances == 0)
        return false;

    // Report all found instances (exact and approximate matches).
    for (int i = 0; i < worst.instances; ++i)
    {
        container.report(worst.indexOf1stInstance + i, worst.longestSame,
                         i + 1, options.isVerbose && i == worst.instances - 1,
                         options.wordMode);
    }
    cout << std::endl;

    totalDuplication += worst.longestSame * worst.instances;

    // Clear bookmarks that point to something within the reported area.
    // This is to avoid reporting the same section more than once.
    container.clearWithin(worst.indexOf1stInstance, worst.longestSame,
                          worst.instances);
    return true;
}

static Duplication findWorst(const BookmarkContainer& container,
                             const Options&           options,
                             const char*              processedEnd)
{
    Duplication result;

    // Find the two bookmarks that have the longest common substring.
    for (size_t markIx = 0; markIx < container.size() - 1; ++markIx)
    {
        if (container.isCleared(markIx + 1))
            break;

        if (container.same(markIx, markIx + 1, result.longestSame,
                           processedEnd))
        {
            const int same = container.nrOfSame(markIx, markIx + 1);
            if (same > result.longestSame)
            {
                result.indexOf1stInstance = markIx;
                result.longestSame        = same;
            }
        }
    }

    if (result.longestSame >= options.minLength)
    {
        int almostLongest =
            (result.longestSame * options.proximityFactor) / 100;

        // Look for approximate matches in strings just before the current
        // pair.
        int stepsBackward = expandSearch(container, result, almostLongest, -1,
                                         -1);

        // Look for approximate matches in strings just after the current pair.
        int stepsForward = expandSearch(container, result, almostLongest, 2,
                                        1);
        result.instances = 2 + stepsBackward + stepsForward;
        result.indexOf1stInstance -= stepsBackward;
    }
    return result;
}

static int expandSearch(const BookmarkContainer& container,
                        Duplication&             duplication,
                        int                      almostLongest,
                        int                      startingPoint,
                        int                      loopIncrement)
{
    int steps = 0;
    for (int i = duplication.indexOf1stInstance + startingPoint;
         i >= 0 && i < int(container.size()) && not container.isCleared(i);
         i += loopIncrement)
    {
        const int same = container.nrOfSame(duplication.indexOf1stInstance, i);
        if (same < almostLongest)
            break;
        steps++;
        if (duplication.longestSame > same)
            duplication.longestSame = same;
    }
    return steps;
}
