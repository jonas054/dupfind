#ifndef BOOKMARK_CONTAINER_HH
#define BOOKMARK_CONTAINER_HH

#include "bookmark.hh"

class BookmarkContainer
{
public:
    void addBookmark(const Bookmark& bm);

    void report(int i,
                int aNrOfSame,
                int anInstanceNr,
                bool isVerbose_,
                bool wordMode);

    size_t size() const;

    bool isCleared(int i) const;

    bool same(int a, int b, int longestSame, const char* processedEnd);

    int nrOfSame(int a, int b) const;

    void sort();

    void clearWithin(int indexOf1stInstance,
                     int longestSame,
                     int instances);
private:
    /**
     * Removes all bookmarks where the "processed" field is null while maintaining
     * a sorted bookmark array. It uses a "two index fingers" algorithm looking for
     * null bookmarks with the left index finger (dest) and for non-null bookmarks
     * with the right index finger (source).
     */
    void getRidOfHoles();

    vector<Bookmark> bookmarks;
};

#endif
