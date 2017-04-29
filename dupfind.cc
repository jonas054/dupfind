//=============================================================================
//
// File: dupfind.cc
//
// Finds and reports duplication in source code or other text files.
//
// Copyright (C) 2002-2017 Jonas Arvidsson
//
//=============================================================================

#include <algorithm>  // stable_sort
#include <fstream>    // ifstream
#include <iostream>   // cout, endl, ostream
#include <cstring>    // strcmp, strncmp, strlen
#include <climits>    // INT_MAX
#include <sys/stat.h> // struct stat
#include <map>
#include <dirent.h>   // DIR, opendir(), readdir(), closedir()

#include "bookmark.hh"
#include "bookmark_container.hh"
#include "options.hh"

using std::cout;
using std::endl;
using std::map;

/*-----------------------------------------------------------------------------
 * Constants
 *---------------------------------------------------------------------------*/

// We use ASCII code 7 as a special value denoting EOF.
const char SPECIAL_EOF = '\x7';

/*-----------------------------------------------------------------------------
 * Static Functions
 *---------------------------------------------------------------------------*/

/**
 * Adds a character to the processed string and sets a bookmark, which is then
 * returned.
 */
static Bookmark addChar(char c, int anOriginalIndex, char* aProcessedText)
{
    static int procIx;
    Bookmark bookmark(anOriginalIndex, &aProcessedText[procIx]);
    aProcessedText[procIx++] = c;
    return bookmark;
}

enum State
{
    NORMAL, COMMENT_START, C_COMMENT, C_COMMENT_END, DOUBLE_QUOTE,
    SINGLE_QUOTE, ESCAPE_DOUBLE, ESCAPE_SINGLE, SKIP_TO_EOL, SPACE, NO_STATE
};

enum Action { NA, ADD_CHAR, ADD_SLASH_AND_CHAR, ADD_BOOKMARK, ADD_SPACE };

struct Cell { State oldState; char event; State newState; Action action; };

struct Key
{
    Key(State s, char e): oldState(s), event(e) {}
    bool operator<(const Key& k) const
    {
        return (oldState < k.oldState ? true :
                oldState > k.oldState ? false :
                event < k.event);
    }
    State oldState;
    char  event;
};

struct Value { State newState; Action action; };

/**
 * Reads the original text into a processed text, which is returned. Also sets
 * the bookmarks to point into the two strings.
 */
static const char* process(BookmarkContainer& container, bool wordMode)
{
    const char ANY = '\0';
    static Cell codeBehavior[] =
        { //  oldState       event newState       action
            { NORMAL,        '/',  COMMENT_START, NA                 },
            { NORMAL,        '"',  DOUBLE_QUOTE,  ADD_CHAR           },
            { NORMAL,        '\'', SINGLE_QUOTE,  ADD_CHAR           },
            { NORMAL,        '\n', NORMAL,        ADD_BOOKMARK       },
            // See special handling of NORMAL in code further down.

            { DOUBLE_QUOTE,  '\\', ESCAPE_DOUBLE, ADD_CHAR           },
            { DOUBLE_QUOTE,  '"',  NORMAL,        ADD_CHAR           },
            { DOUBLE_QUOTE,  ANY,  DOUBLE_QUOTE,  ADD_CHAR           },
            { DOUBLE_QUOTE,  '\n', NORMAL,        ADD_BOOKMARK       }, // (1)
            { SINGLE_QUOTE,  '\\', ESCAPE_SINGLE, ADD_CHAR           },
            { SINGLE_QUOTE,  '\'', NORMAL,        ADD_CHAR           },
            { SINGLE_QUOTE,  ANY,  SINGLE_QUOTE,  ADD_CHAR           },
            { SINGLE_QUOTE,  '\n', NORMAL,        ADD_BOOKMARK       }, // (1)
            { ESCAPE_SINGLE, ANY,  SINGLE_QUOTE,  ADD_CHAR           },
            { ESCAPE_DOUBLE, ANY,  DOUBLE_QUOTE,  ADD_CHAR           },
            // (1) probably a mistake if quote reaches end-of-line.

            { COMMENT_START, '*',  C_COMMENT,     NA                 },
            { COMMENT_START, '/',  SKIP_TO_EOL,   NA                 },
            { COMMENT_START, ANY,  NORMAL,        ADD_SLASH_AND_CHAR },
            { SKIP_TO_EOL,   '\n', NORMAL,        ADD_BOOKMARK       },
            { C_COMMENT,     '*',  C_COMMENT_END, NA                 },
            { C_COMMENT_END, '/',  NORMAL,        NA                 },
            { C_COMMENT_END, '*',  C_COMMENT_END, NA                 },
            { C_COMMENT_END, ANY,  C_COMMENT,     NA                 },

            { NO_STATE,      ANY,  NO_STATE,      NA                 }
        };
    static Cell textBehavior[] =
        { //  oldState  event newState action
            { NORMAL,   ' ',  SPACE,    NA       },
            { NORMAL,   '\t', SPACE,    NA       },
            { NORMAL,   '\r', SPACE,    NA       },
            { NORMAL,   '\n', SPACE,    NA       },
            { NORMAL,   '', SPACE,    NA       },
            { NORMAL,   ANY,  NORMAL,   ADD_CHAR },

            { SPACE,    ' ',  SPACE,    NA        },
            { SPACE,    '\t', SPACE,    NA        },
            { SPACE,    '\r', SPACE,    NA        },
            { SPACE,    '\n', SPACE,    NA        },
            { SPACE,    '', SPACE,    NA        },
            { SPACE,    ANY,  NORMAL,   ADD_SPACE },

            { NO_STATE, ANY,  NO_STATE, NA        }
        };

    map<Key, Value> matrix;
    Cell* cells = wordMode ? textBehavior : codeBehavior;

    for (int i = 0; cells[i].oldState != NO_STATE; ++i)
    {
        Key k(cells[i].oldState, cells[i].event);
        Value v;
        v.newState = cells[i].newState;
        v.action   = cells[i].action;
        matrix[k] = v;
    }

    State       state              = NORMAL;
    char* const processed          = new char[Bookmark::totalLength()];
    bool        timeForNewBookmark = true;

    for (size_t i = 0; i < Bookmark::totalLength(); ++i)
    {
        const char c = Bookmark::getChar(i);

        // Apparently there can be zeroes in the total string, but only when
        // running on some machines. Don't know why.
        if (c == '\0')
            continue;

        if (c == SPECIAL_EOF)
        {
            addChar(c, i, processed);
            state = NORMAL;
            continue;
        }

        map<Key, Value>::iterator it;
        if ((it = matrix.find(Key(state, c)))   != matrix.end() ||
            (it = matrix.find(Key(state, ANY))) != matrix.end())
        {
            state = it->second.newState;
            if (it->second.action == ADD_SLASH_AND_CHAR)
            {
                addChar('/', i-1, processed);
                if (not isspace(c))
                    addChar(c, i, processed);
            }
            else if (it->second.action == ADD_CHAR)
            {
                const Bookmark bm = addChar(c, i, processed);
                if (timeForNewBookmark)
                {
                    container.addBookmark(bm);
                    timeForNewBookmark = false;
                }
            }
            else if (it->second.action == ADD_BOOKMARK)
                timeForNewBookmark = true;
            else if (it->second.action == ADD_SPACE)
            {
                addChar(' ', i, processed);
                container.addBookmark(addChar(c, i, processed));
            }
        }
        else if (state == NORMAL && not isspace(c))
        { // Handle state/event pair that can't be handled by The Matrix.
            if (timeForNewBookmark && c != '}')
            {
                if (c == '#' ||
                    strncmp("import", &Bookmark::getChar(i), 6) == 0 ||
                    strncmp("using",  &Bookmark::getChar(i), 5) == 0)
                {
                    state = SKIP_TO_EOL;
                }
                else
                    container.addBookmark(addChar(c, i, processed));
            }
            else
                addChar(c, i, processed);

            timeForNewBookmark = false;
        }
    }
    addChar('\0', Bookmark::totalLength(), processed);

    return processed;
}

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
    cout << endl;

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

    const char* processed        = process(container, options.wordMode);
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
        cout << "Duplication = " << Bookmark::getTotalNrOfLines() << " lines, "
             << (100 * totalDuplication + length / 2) / length << " %\n";
    }
    delete [] processed;
    return 0;
}
