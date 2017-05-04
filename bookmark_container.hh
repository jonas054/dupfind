#ifndef BOOKMARK_CONTAINER_HH
#define BOOKMARK_CONTAINER_HH

#include <vector>

class Bookmark;
class Options;
struct Duplication;

class BookmarkContainer
{
public:
    void addBookmark(const Bookmark& bm);

    void report(int                bookmarkIx,
                const Duplication& duplication,
                int                instanceNr,
                const Options&     options) const;

    size_t size() const;

    bool same(int a, int b, int longestSame, const char* processedEnd) const;

    int nrOfSame(int a, int b) const;

    void sort();

    void clearWithin(const Duplication& duplication);

private:
    void getRidOfHoles();

    std::vector<Bookmark> itsBookmarks;
};

#endif
