#include "bookmark.hh"
#include "bookmark_container.hh"

#include <algorithm>  // stable_sort

void BookmarkContainer::addBookmark(const Bookmark& bm)
{
    bookmarks.push_back(bm);
}

void BookmarkContainer::report(int i,
                               int aNrOfSame,
                               int anInstanceNr,
                               bool isVerbose_,
                               bool wordMode) const
{
    bookmarks[i].report(aNrOfSame, anInstanceNr, isVerbose_, wordMode);
}

size_t BookmarkContainer::size() const
{
    return bookmarks.size();
}

bool BookmarkContainer::isCleared(int i) const
{
    return bookmarks[i].isCleared();
}

bool BookmarkContainer::same(int a,
                             int b,
                             int longestSame,
                             const char* processedEnd) const
{
    return bookmarks[a].sameAs(bookmarks[b], longestSame, processedEnd);
}

int BookmarkContainer::nrOfSame(int a, int b) const
{
    return bookmarks[a].nrOfSame(bookmarks[b]);
}

void BookmarkContainer::sort()
{
    // std::stable_sort(), which is a merge sort, has proved to be much faster
    // than std::sort() in this context.
    std::stable_sort(bookmarks.begin(), bookmarks.end());
}

void BookmarkContainer::clearWithin(int indexOf1stInstance,
                                    int longestSame,
                                    int instances)
{
    for (int i = 0; i < instances; ++i)
    {
        const char* reportStart = bookmarks[indexOf1stInstance + i].processed;

        for (size_t ix = 0; ix < bookmarks.size() - 1; ++ix)
            if (bookmarks[ix].processed >= reportStart &&
                bookmarks[ix].processed < reportStart + longestSame)
            {
                bookmarks[ix].clear();
            }
    }
    getRidOfHoles();
}

/**
 * Removes all bookmarks where the "processed" field is null while maintaining
 * a sorted bookmark array. It uses a "two index fingers" algorithm looking for
 * null bookmarks with the left index finger (dest) and for non-null bookmarks
 * with the right index finger (source).
 */
void BookmarkContainer::getRidOfHoles()
{
    size_t source = 0;
    for (size_t dest = 0; dest < bookmarks.size() - 1; ++dest)
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
