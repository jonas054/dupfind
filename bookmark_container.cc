#include "bookmark.hh"
#include "bookmark_container.hh"

#include <algorithm> // stable_sort, remove_if, mem_fun_ref

void BookmarkContainer::addBookmark(const Bookmark& bm)
{
    bookmarks.push_back(bm);
}

void BookmarkContainer::report(int                bookmarkIx,
                               const Duplication& duplication,
                               int                instanceNr,
                               const Options&     options) const
{
    bookmarks[bookmarkIx].report(duplication, instanceNr, options);
}

size_t BookmarkContainer::size() const
{
    return bookmarks.size();
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
 * a sorted bookmark array.
 */
void BookmarkContainer::getRidOfHoles()
{
    std::vector<Bookmark>::iterator newEnd =
        std::remove_if(bookmarks.begin(), bookmarks.end(),
                       std::mem_fun_ref(&Bookmark::isCleared));
    bookmarks.resize(newEnd - bookmarks.begin());
}
