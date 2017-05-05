#ifndef BOOKMARK_HH
#define BOOKMARK_HH

#include <string>
#include <vector>

struct Duplication;
class Options;

/**
 * A bookmark is something that points at a position in the processed
 * text while also keeping track of the corresponding position in the
 * file where the text originally came from.
 */
class Bookmark
{
    struct FileRecord
    {
        FileRecord(const std::string& n, int i): fileName(n), endIx(i) {}
        std::string fileName;
        int         endIx; // Position right after the final char of the file.
    };

public:
    Bookmark(int i = 0, const char* p = 0): itsOriginalIndex(i),
                                            itsProcessedText(p) {}

    /**
     * Reports one instance of duplication and optionally prints the duplicated
     * string.
     */
    void report(const Duplication& duplication,
                int                instanceNr,
                const Options&     options) const;

    /**
     * Clear the bookmark, i.e. mark it for deletion.
     */
    void clear() { itsProcessedText = 0; }

    bool isCleared() const { return itsProcessedText == 0; }

    bool operator<(const Bookmark& another) const; // Used in sorting.

    /**
     * How many characters are equal when comparing the bookmark to
     * another bookmark?
     */
    int nrOfSame(Bookmark b) const;

    /**
     * Used for optimization purposes. By comparing strings backwards we can
     * find out quickly if the two strings are not equal in the given number of
     * characters and move on to the next comparison.
     */
    bool sameAs(Bookmark b, int nrOfCharacters, const char* end) const;

    static int getTotalNrOfLines() { return theirTotalNrOfLines; }

    static void addFile(const std::string& fileName);

    static size_t totalLength() { return theirOriginalString.length(); }

    static const char& getChar(int i) { return theirOriginalString[i]; }

private:
    friend std::ostream& operator<<(std::ostream& os, const Bookmark& b);
    friend class BookmarkContainer;

    enum DetailType { PRINT_LINES, COUNT_LINES };

    int
    details(int processedLength, DetailType detailType, bool wordMode) const;

    static int lineNr(int offset, int index);

    static int                     theirTotalNrOfLines;
    static std::vector<FileRecord> theirFileRecords;
    static std::string             theirOriginalString;

    int         itsOriginalIndex;
    const char* itsProcessedText;
};

#endif
