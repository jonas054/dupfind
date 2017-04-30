#ifndef BOOKMARK_HH
#define BOOKMARK_HH

#include <string>
#include <vector>

class Bookmark
{
    struct FileRecord
    {
        FileRecord(const char* n, int i): fileName(n), endIx(i) {}
        const char* fileName;
        int         endIx; // Position right after the final char of the file.
    };

public:
    Bookmark(int i, const char* p): original(i), processed(p) {}

    /**
     * Reports one instance of duplication and optionally prints the duplicated
     * string.
     */
    void report(int aNrOfSame,
                int anInstanceNr,
                bool isVerbose_,
                bool wordMode) const;

    void clear() { this->processed = 0; }

    bool isCleared() const { return this->processed == 0; }

    bool operator<(const Bookmark& another) const; // Used in sorting.

    int nrOfSame(Bookmark b) const;

    /**
     * Used for optimization purposes. By comparing strings backwards we can
     * find out quickly if the two strings are not equal in the given number of
     * characters and move on to the next comparison.
     */
    bool sameAs(Bookmark b, int aNrOfCharacters, const char* anEnd) const;

    static int getTotalNrOfLines() { return totalNrOfLines; }

    static void addFile(const char* fileName);

    static size_t totalLength() { return totalString.length(); }

    static const char& getChar(int i) { return totalString[i]; }

private:
    friend std::ostream& operator<<(std::ostream& os, const Bookmark& b);
    friend class BookmarkContainer;

    enum DetailType { PRINT_LINES, COUNT_LINES };

    int details(int aProcessedLength, DetailType  aType, bool wordMode) const;

    static int lineNr(const char* aBase, int anOffset, int anIndex);

    static int                     totalNrOfLines;
    static std::vector<FileRecord> fileRecords;
    static std::string             totalString;

    int         original;
    const char* processed;
};

#endif
