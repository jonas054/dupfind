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
#include <iostream>   // cout, cerr, endl, ios, ostream
#include <iterator>   // istream_iterator
#include <cstring>    // strcmp, strncmp, strlen
#include <climits>    // INT_MAX
#include <sys/stat.h> // struct stat
#include <map>
#include <string>
#include <vector>
#include <dirent.h>   // DIR, opendir(), readdir(), closedir()

using std::cerr;
using std::cout;
using std::endl;
using std::ios;
using std::istream_iterator;
using std::map;
using std::string;
using std::vector;

/*-----------------------------------------------------------------------------
 * Constants
 *---------------------------------------------------------------------------*/

// We use ASCII code 7 as a special value denoting EOF.
static const char SPECIAL_EOF = '\x7';

/*-----------------------------------------------------------------------------
 * Forward declarations
 *---------------------------------------------------------------------------*/

class Bookmark;

/*-----------------------------------------------------------------------------
 * Global variables
 *---------------------------------------------------------------------------*/

static vector<Bookmark> bookmarks;

/*-----------------------------------------------------------------------------
 * Types
 *---------------------------------------------------------------------------*/

struct FileRecord
{
    FileRecord(const char* n, int i): fileName(n), endIx(i) {}
    const char* fileName;
    int         endIx; // Position right after the final char of the file.
};

vector<FileRecord> fileRecords;
int                totalNrOfLines = 0;
string             totalString    = "";

class Bookmark
{
public:
    Bookmark(int i, const char* p): original(i), processed(p) {}

    /**
     * Reports one instance of duplication and optionally prints the duplicated
     * string.
     */
    void report(int aNrOfSame,
                int anInstanceNr,
                bool isVerbose_,
                bool wordMode)
    {
        static int count = 0;

        if (anInstanceNr == 1)
            ++count;

        cout << *this << ":Duplication " << count << " (" << anInstanceNr
             << order(anInstanceNr) << " instance";
        if (anInstanceNr == 1)
        {
            int nrOfLines = details(aNrOfSame, COUNT_LINES, wordMode);
            cout << ", " << aNrOfSame << " characters, "
                 << nrOfLines << " line" << (nrOfLines == 1 ? "" : "s");
        }
        else
            totalNrOfLines += details(aNrOfSame, COUNT_LINES, wordMode);

        cout << ")." << endl;
        if (isVerbose_)
            details(aNrOfSame, PRINT_LINES, wordMode);
    }

    void clear() { this->processed = 0; }

    bool isCleared() const { return this->processed == 0; }

    bool operator<(const Bookmark& another) const // Used in sorting.
    {
        return strcmp(another.processed, this->processed) < 0;
    }

    int nrOfSame(Bookmark b) const
    {
        int index = 0;
        for (; this->processed[index] == b.processed[index]; ++index)
        {
            // The characters are equal so we only have to check one of them.
            if (this->processed[index] == SPECIAL_EOF)
                break;
        }
        return index;
    }

    /**
     * Used for optimization purposes. By comparing strings backwards we can
     * find out quickly if the two strings are not equal in the given number of
     * characters and move on to the next comparison.
     */
    bool sameAs(Bookmark b, int aNrOfCharacters, const char* anEnd) const
    {
        if (&this->processed[aNrOfCharacters] >= anEnd ||
            &b.processed[aNrOfCharacters] >= anEnd)
        {
            return false;
        }

        for (int i = aNrOfCharacters; i >= 0; --i)
            if (this->processed[i] != b.processed[i])
                return false;

        return true;
    }

    static void clearWithin(int indexOf1stInstance,
                            int longestSame,
                            int instances)
    {
        for (int i = 0; i < instances; ++i)
        {
            const char* reportStart =
                bookmarks[indexOf1stInstance + i].processed;

            for (size_t ix = 0; ix < bookmarks.size() - 1; ++ix)
                if (bookmarks[ix].processed >= reportStart &&
                    bookmarks[ix].processed < reportStart + longestSame)
                {
                    bookmarks[ix].clear();
                }
        }
    }

private:
    friend std::ostream& operator<<(std::ostream& os, const Bookmark& b);

    enum DetailType { PRINT_LINES, COUNT_LINES };

    int details(int aProcessedLength, DetailType  aType, bool wordMode)
    {
        const char* orig = totalString.c_str() + this->original;
        if (not wordMode)
        {
            while (orig > totalString.c_str() && *orig != '\n')
                --orig; // to include leading whitespace in printout
            ++orig;
        }
        int  count     = 1;
        bool blankLine = true;
        for (int pi = 0; pi < aProcessedLength; ++pi, ++orig)
        {
            for (; *orig != 0 && *orig != SPECIAL_EOF; ++orig)
            {
                if (aType == PRINT_LINES)
                    cout << *orig;
                else if (*orig == '\n')
                {
                    if (not blankLine)
                    {
                        count++;
                        blankLine = true;
                    }
                }
                else if (not isspace(*orig))
                    blankLine = false;
                // In word mode, a space in the processed text means any kind
                // of space, so we can not continue to search for an exact
                // match.
                if (*orig == this->processed[pi] ||
                    (isspace(this->processed[pi]) && isspace(*orig)))
                {
                    break;
                }
            }
        }
        if (aType == PRINT_LINES)
            cout << endl;
        return count;
    }

    // Returns the correct suffix for strings like 1st, 2nd, 3rd, 4th, etc.
    static const char* order(int aNumber)
    {
        if      (aNumber % 10 == 1 && aNumber % 100 != 11) return "st";
        else if (aNumber % 10 == 2 && aNumber % 100 != 12) return "nd";
        else if (aNumber % 10 == 3 && aNumber % 100 != 13) return "rd";
        else                                               return "th";
    }

    static int lineNr(const char* aBase, int anOffset, int anIndex)
    {
        const int start = (anIndex == 0) ? 0 : fileRecords[anIndex - 1].endIx;
        int result = 1;
        for (int i = start; i < anOffset; ++i)
            if (aBase[i] == '\n')
                ++result;
        return result;
    }

    int         original;
    const char* processed;
};

std::ostream& operator<<(std::ostream& os, const Bookmark& b)
{
    int recIx = 0;
    while (fileRecords[recIx].endIx <= b.original)
        ++recIx;

    os << fileRecords[recIx].fileName << ":"
       << Bookmark::lineNr(totalString.c_str(), b.original, recIx);
    return os;
}

/*-----------------------------------------------------------------------------
 * Static Functions
 *---------------------------------------------------------------------------*/

static void findFiles(const string&         name,
                      const string&         ending,
                      const vector<string>& excludes,
                      vector<string>&       output)
{
    DIR* dir = opendir(name.c_str());

    if (!dir)
        return;

    for (struct dirent* entry = readdir(dir); entry; entry = readdir(dir))
    {
        const string entryName = entry->d_name;
        if (entryName == "." || entryName == "..")
            continue;
        const string path = name + "/" + entry->d_name;
        bool toBeExcluded = false;
        for (size_t e = 0; e < excludes.size(); ++e)
            if (path.find(excludes[e]) != string::npos)
                toBeExcluded = true;
        if (toBeExcluded)
            continue;
        if (path.length() > ending.length() &&
            path.substr(path.length() - ending.length()) == ending)
        {
            output.push_back(path);
        }
        findFiles(path, ending, excludes, output);
    }
    closedir(dir);
}

inline void addBookmark(const Bookmark& bm)
{
    bookmarks.push_back(bm);
}

static string readFileIntoString(string aFileName)
{
    struct stat s;
    if ((stat(aFileName.c_str(), &s) == 0) && (s.st_mode & S_IFDIR))
    {
        cerr << "dupfind: " << aFileName << " is a directory.\n";
        exit(EXIT_FAILURE);
    }

    std::ifstream in(aFileName.c_str(), ios::in);
    if (!in.good())
    {
        cerr << "dupfind: File " << aFileName << " not found.\n";
        exit(EXIT_FAILURE);
    }
    in.unsetf(ios::skipws);
    in.seekg(0, ios::end);
    std::streampos fsize = in.tellg() + (std::streamoff)1;
    in.seekg(0, ios::beg);

    vector<char> vec;
    vec.reserve(fsize);
    copy(istream_iterator<char>(in), istream_iterator<char>(),
         back_inserter(vec));

    vec.push_back(SPECIAL_EOF);

    return string(vec.begin(), vec.end());
}

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
static const char* process(bool wordMode)
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
    char* const processed          = new char[totalString.length()];
    bool        timeForNewBookmark = true;

    for (size_t i = 0; i < totalString.length(); ++i)
    {
        const char c = totalString[i];

        // Apparenly there can be zeroes in the total string, but only when
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
                    addBookmark(bm);
                    timeForNewBookmark = false;
                }
            }
            else if (it->second.action == ADD_BOOKMARK)
                timeForNewBookmark = true;
            else if (it->second.action == ADD_SPACE)
            {
                addChar(' ', i, processed);
                addBookmark(addChar(c, i, processed));
            }
        }
        else if (state == NORMAL && not isspace(c))
        { // Handle state/event pair that can't be handled by The Matrix.
            if (timeForNewBookmark && c != '}')
            {
                if (c == '#' ||
                    strncmp("import", &totalString[i], 6) == 0 ||
                    strncmp("using",  &totalString[i], 5) == 0)
                {
                    state = SKIP_TO_EOL;
                }
                else
                    addBookmark(addChar(c, i, processed));
            }
            else
                addChar(c, i, processed);

            timeForNewBookmark = false;
        }
    }
    addChar('\0', totalString.length(), processed);

    return processed;
}

enum ExtFlagMode { SHOW_EXT_FLAGS, HIDE_EXT_FLAGS };

static void printUsageAndExit(ExtFlagMode anExtFlagMode, int anExitCode)
{
    cerr << "Usage: dupfind [-v] [-w] [-<n>|-m<n>] "
         << (anExtFlagMode == SHOW_EXT_FLAGS ? "[-p<n>] " : "")
         << "[-x <substring>] [-e <ending> ...]\n"
         << "       dupfind [-v] [-w] [-<n>|-m<n>] "
         << (anExtFlagMode == SHOW_EXT_FLAGS ? "[-p<n>] " : "") << "<files>\n"
         << "       dupfind -t"
         << (anExtFlagMode == SHOW_EXT_FLAGS ? "|-T" : "")
         << " [-v] [-w] <files>\n"
         << "       -v:    verbose, print strings that are duplicated\n"
         << "       -w:    calculate duplication based on words rather than "
         << "lines\n"
         << "       -10:   report the 10 longest duplications instead of 5,"
         << " which is default\n"
         << "       -m300: report all duplications that are at least 300"
         << " characters long\n"
         << "       -x:    exclude paths matching substring when searching"
         << " for files with -e\n"
         << "              (-x must come before the -e option it applies to)\n"
         << "       -e:    search recursively for files whose names end"
         << " with the given ending\n"
         << "              (several -e options can be given)\n";
    if (anExtFlagMode == SHOW_EXT_FLAGS)
    {
        cerr << "       -p50:  use 50% proximity (more but shorter "
             << "matches); 90% is default\n";
    }
    cerr << "       -t:    set -m100 and sum up the total duplication\n";
    if (anExtFlagMode == SHOW_EXT_FLAGS)
    {
        cerr << "       -T:    same as -t but accept any file (test code"
             << " etc.)\n";
    }
    exit(anExitCode);
}

/**
 * Removes all bookmarks where the "processed" field is null while maintaining
 * a sorted bookmark array. It uses a "two index fingers" algorithm looking for
 * null bookmarks with the left index finger (dest) and for non-null bookmarks
 * with the right index finger (source).
 */
static void getRidOfHoles()
{
    size_t source = 0;
    for (size_t dest = 0; dest < bookmarks.size() - 1; ++dest)
    {
        if (bookmarks[dest].isCleared())
        {
            if (source <= dest)
                source = dest + 1;
            while (source < bookmarks.size() && bookmarks[source].isCleared())
                ++source;

            if (source == bookmarks.size())
                break;

            // Now pointing to a null bookmark (dest) and a non-null bookmark
            // (source). Swap them.
            bookmarks[dest] = bookmarks[source];
            bookmarks[source].clear();
        }
    }
}

class Options
{
public:
    enum TotalReport { NO_TOTAL, RESTRICTED_TOTAL, UNRESTRICTED_TOTAL };

    int            nrOfWantedReports;
    bool           isVerbose;
    TotalReport    totalReport;
    int            minLength;
    int            proximityFactor;
    bool           wordMode;
    vector<string> foundFiles;
    vector<string> excludes;

    Options(int argc, char* argv[]): nrOfWantedReports(5),
                                     isVerbose(false),
                                     totalReport(NO_TOTAL),
                                     minLength(10),
                                     proximityFactor(90),
                                     wordMode(false)
    {
        for (int i = 1; i < argc; ++i)
            if (argv[i][0] == '-')
                i = processFlag(i, argc, argv);
            else
                processFileName(argv[i]);
    }

private:
    int processFlag(int i, int argc, char* argv[])
    {
        const char* arg = argv[i];
        char flag = arg[1];

        if (isdigit(flag))
        {
            nrOfWantedReports = -atoi(arg);
            return i;
        }

        switch (flag)
        {
        case 't':
        case 'T':
            totalReport =
                (flag == 't') ? RESTRICTED_TOTAL : UNRESTRICTED_TOTAL;

            nrOfWantedReports = INT_MAX;
            minLength         = 100;
            proximityFactor   = 100;
            break;
        case 'e': {
            bool isRestrictedTotal = (totalReport == RESTRICTED_TOTAL);
            for (int k = 1; !isRestrictedTotal && k < argc; ++k)
                if (argv[k][0] == '-' && argv[k][1] == 't')
                    isRestrictedTotal = true;
            findFiles(".", argv[++i], excludes, foundFiles);
            for (size_t ii = 0; ii < foundFiles.size(); ++ii)
            {
                if (!isRestrictedTotal ||
                    foundFiles[ii].find("test") == string::npos)
                {
                    totalString += readFileIntoString(foundFiles[ii]);
                    fileRecords.push_back(FileRecord(foundFiles[ii].c_str(),
                                                     totalString.length()));
                }
            }
            break;
        }
        case 'v':
            isVerbose = true;
            break;
        case 'x':
            excludes.push_back(argv[++i]);
            break;
        case 'w':
            wordMode = true;
            break;
        case 'm':
            if (arg[2] == '\0')
                printUsageAndExit(HIDE_EXT_FLAGS, EXIT_FAILURE);

            nrOfWantedReports = INT_MAX;
            minLength         = atoi(&arg[2]);
            break;
        case 'p':
            if (arg[2] == '\0')
                printUsageAndExit(SHOW_EXT_FLAGS, EXIT_FAILURE);

            proximityFactor = atoi(&arg[2]);
            if (proximityFactor < 1 || proximityFactor > 100)
            {
                cerr << "Proximity factor must be between 1 and 100 "
                     << "(inclusive)." << endl;
                printUsageAndExit(SHOW_EXT_FLAGS, EXIT_FAILURE);
            }
            break;
        default:
            printUsageAndExit(SHOW_EXT_FLAGS, EXIT_FAILURE);
        }
        return i;
    }

    void processFileName(const char* arg)
    {
        if (totalReport == RESTRICTED_TOTAL &&
            (string(arg).find("test") != string::npos ||
             (string(arg).find("_R") != string::npos &&
              isdigit(arg[string(arg).find("_R") + 2]))))
        {
            cerr << "The file " << arg << " is not included in the total "
                 << "duplication calculations. Use -T if you want to include "
                 << "it." << endl;
        }
        else
        {
            totalString += readFileIntoString(arg);
            fileRecords.push_back(FileRecord(arg, totalString.length()));
        }
    }
};

/*-----------------------------------------------------------------------------
 * Main
 *---------------------------------------------------------------------------*/

int main(int argc, char* argv[])
{
    if (argc == 1)
        printUsageAndExit(HIDE_EXT_FLAGS, EXIT_SUCCESS);

    Options options(argc, argv);

    if (totalString.length() == 0)
        printUsageAndExit(HIDE_EXT_FLAGS, EXIT_FAILURE);

    fileRecords.push_back(FileRecord(0, 0)); // Mark end

    const char* processed        = process(options.wordMode);
    const char* processedEnd     = processed + strlen(processed);
    int         totalDuplication = 0;

    // std::stable_sort(), which is a merge sort, has proved to be much faster
    // than std::sort() in this context.
    std::stable_sort(bookmarks.begin(), bookmarks.end());

    for (int count = 0; count < options.nrOfWantedReports; ++count)
    {
        int longestSame        = 0;
        int indexOf1stInstance = 0;

        // Find the two bookmarks that have the longest common substring.
        for (size_t markIx = 0; markIx < bookmarks.size() - 1; ++markIx)
        {
            if (bookmarks[markIx + 1].isCleared())
                break;

            if (bookmarks[markIx].sameAs(bookmarks[markIx + 1],
                                         longestSame, processedEnd))
            {
                const int same =
                    bookmarks[markIx].nrOfSame(bookmarks[markIx + 1]);
                if (same > longestSame)
                {
                    indexOf1stInstance = markIx;
                    longestSame        = same;
                }
            }
        }

        if (longestSame < options.minLength) // Exit loop if the common
            break;                           // substring is too short.

        int instances = 2;
        int almostLongest = (longestSame * options.proximityFactor) / 100;
        int origIndexForLongest = indexOf1stInstance;

        // Look for approximate matches in strings just before the current
        // pair.
        for (int i = origIndexForLongest - 1; i >= 0; --i)
        {
            const int same =
                bookmarks[origIndexForLongest].nrOfSame(bookmarks[i]);
            if (same < almostLongest)
                break;
            instances++;
            if (longestSame > same)
                longestSame = same;
            indexOf1stInstance = i;
        }

        // Look for approximate matches in strings just after the current pair.
        for (size_t i = origIndexForLongest + 2;
             i < bookmarks.size() && !bookmarks[i].isCleared();
             ++i)
        {
            const int same =
                bookmarks[origIndexForLongest].nrOfSame(bookmarks[i]);
            if (same < almostLongest)
                break;
            instances++;
            if (longestSame > same)
                longestSame = same;
        }

        // Report all found instances (exact and approximate matches).
        for (int i = 0; i < instances; ++i)
        {
            bookmarks[indexOf1stInstance + i].
                report(longestSame, i + 1,
                       options.isVerbose && i == instances - 1,
                       options.wordMode);
        }
        cout << endl;

        totalDuplication += longestSame * instances;

        // Clear bookmarks that point to something within the reported area.
        // This is to avoid reporting the same section more than once.
        Bookmark::clearWithin(indexOf1stInstance, longestSame, instances);

        getRidOfHoles();
    } // for (int count = 0; count < options.nrOfWantedReports; ++count)

    if (options.totalReport != Options::NO_TOTAL)
    {
        const int length = processedEnd - processed;
        cout << "Duplication = " << totalNrOfLines << " lines, "
             << (100 * totalDuplication + length / 2) / length << " %\n";
    }
    delete [] processed;
    return 0;
}
