#include <algorithm>  // EXIT_FAILURE
#include <fstream>    // ifstream
#include <iostream>   // cout, cerr, endl, ios, ostream
#include <iterator>   // istream_iterator
#include <cstring>    // strcmp
#include <sys/stat.h> // struct stat
#include <string>
#include <vector>
#include "bookmark.hh"

using std::cerr;
using std::cout;
using std::endl;
using std::ios;
using std::istream_iterator;
using std::string;
using std::vector;

int                Bookmark::totalNrOfLines = 0;
vector<FileRecord> Bookmark::fileRecords;
string             Bookmark::totalString;

Bookmark::Bookmark(int i, const char* p): original(i), processed(p) {}

/**
 * Reports one instance of duplication and optionally prints the duplicated
 * string.
 */
void Bookmark::report(int aNrOfSame,
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

void Bookmark::clear() { this->processed = 0; }

bool Bookmark::isCleared() const { return this->processed == 0; }

bool Bookmark::operator<(const Bookmark& another) const // Used in sorting.
{
    return strcmp(another.processed, this->processed) < 0;
}

int Bookmark::nrOfSame(Bookmark b) const
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
bool Bookmark::sameAs(Bookmark b, int aNrOfCharacters, const char* anEnd) const
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

int Bookmark::getTotalNrOfLines() { return totalNrOfLines; }

void Bookmark::addFile(const char* fileName)
{
    if (fileName)
        addText(readFileIntoString(fileName));

    fileRecords.push_back(FileRecord(fileName, totalString.length()));
}

void Bookmark::addText(const string& text) { totalString += text; }

size_t Bookmark::totalLength() { return totalString.length(); }

const char& Bookmark::getChar(int i) { return totalString[i]; }

string Bookmark::readFileIntoString(const char* aFileName)
{
    struct stat s;
    if ((stat(aFileName, &s) == 0) && (s.st_mode & S_IFDIR))
    {
        cerr << "dupfind: " << aFileName << " is a directory.\n";
        exit(EXIT_FAILURE);
    }

    std::ifstream in(aFileName, ios::in);
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

int Bookmark::details(int aProcessedLength, DetailType  aType, bool wordMode)
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
const char* Bookmark::order(int aNumber)
{
    if      (aNumber % 10 == 1 && aNumber % 100 != 11) return "st";
    else if (aNumber % 10 == 2 && aNumber % 100 != 12) return "nd";
    else if (aNumber % 10 == 3 && aNumber % 100 != 13) return "rd";
    else                                               return "th";
}

int Bookmark::lineNr(const char* aBase, int anOffset, int anIndex)
{
    const int start = (anIndex == 0) ? 0 : fileRecords[anIndex - 1].endIx;
    int result = 1;
    for (int i = start; i < anOffset; ++i)
        if (aBase[i] == '\n')
            ++result;
    return result;
}

std::ostream& operator<<(std::ostream& os, const Bookmark& b)
{
    int recIx = 0;
    while (Bookmark::fileRecords[recIx].endIx <= b.original)
        ++recIx;

    os << Bookmark::fileRecords[recIx].fileName << ":"
       << Bookmark::lineNr(Bookmark::totalString.c_str(), b.original, recIx);
    return os;
}
