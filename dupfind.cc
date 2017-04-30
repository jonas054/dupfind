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

/*-----------------------------------------------------------------------------
 * Static Functions
 *---------------------------------------------------------------------------*/

static int expandSearch(const BookmarkContainer& container,
                        int                      indexForLongest,
                        int                      almostLongest,
                        int                      startingPoint,
                        int                      loopIncrement,
                        int&                     longestSame)
{
    int steps = 0;
    for (int i = indexForLongest + startingPoint;
         i >= 0 && i < int(container.size()) && !container.isCleared(i);
         i += loopIncrement)
    {
        const int same = container.nrOfSame(indexForLongest, i);
        if (same < almostLongest)
            break;
        steps++;
        if (longestSame > same)
            longestSame = same;
    }
    return steps;
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
    int longestSame        = 0;
    int indexOf1stInstance = 0;

    // Find the two bookmarks that have the longest common substring.
    for (size_t markIx = 0; markIx < container.size() - 1; ++markIx)
    {
        if (container.isCleared(markIx + 1))
            break;

        if (container.same(markIx, markIx + 1, longestSame, processedEnd))
        {
            const int same = container.nrOfSame(markIx, markIx + 1);
            if (same > longestSame)
            {
                indexOf1stInstance = markIx;
                longestSame        = same;
            }
        }
    }

    if (longestSame < options.minLength) // Exit loop if the common
        return false;                    // substring is too short.

    int almostLongest = (longestSame * options.proximityFactor) / 100;

    // Look for approximate matches in strings just before the current
    // pair.
    int stepsBackward = expandSearch(container, indexOf1stInstance,
                                     almostLongest, -1, -1, longestSame);

    // Look for approximate matches in strings just after the current pair.
    int stepsForward = expandSearch(container, indexOf1stInstance,
                                    almostLongest, 2, 1, longestSame);

    int instances = 2 + stepsBackward + stepsForward;
    indexOf1stInstance -= stepsBackward;

    // Report all found instances (exact and approximate matches).
    for (int i = 0; i < instances; ++i)
    {
        container.report(indexOf1stInstance + i, longestSame, i + 1,
                         options.isVerbose && i == instances - 1,
                         options.wordMode);
    }
    std::cout << std::endl;

    totalDuplication += longestSame * instances;

    // Clear bookmarks that point to something within the reported area.
    // This is to avoid reporting the same section more than once.
    container.clearWithin(indexOf1stInstance, longestSame, instances);

    return true;
}

/*-----------------------------------------------------------------------------
 * Main
 *---------------------------------------------------------------------------*/

int main(int argc, char* argv[])
{
    BookmarkContainer container;

    if (argc == 1)
        Options::printUsageAndExit(Options::HIDE_EXT_FLAGS, EXIT_SUCCESS);

    Options options(argc, argv);

    if (Bookmark::totalLength() == 0)
        Options::printUsageAndExit(Options::HIDE_EXT_FLAGS, EXIT_FAILURE);

    Bookmark::addFile(0); // Mark end

    const char* processed        = Parser::process(container,
                                                   options.wordMode);
    const char* processedEnd     = processed + strlen(processed);
    int         totalDuplication = 0;

    container.sort();

    for (int count = 0; count < options.nrOfWantedReports; ++count)
    {
        if (not reportOne(container, options, processedEnd, totalDuplication))
            break;
    }

    if (options.totalReport != Options::NO_TOTAL)
    {
        const int length = processedEnd - processed;
        std::cout << "Duplication = " << Bookmark::getTotalNrOfLines()
                  << " lines, "
                  << (100 * totalDuplication + length / 2) / length << " %\n";
    }
    delete [] processed;
    return 0;
}
