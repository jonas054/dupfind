#include "bookmark.hh"
#include "bookmark_container.hh"
#include "duplication.hh"

#include <algorithm> // stable_sort, remove_if, mem_fun_ref

void BookmarkContainer::report(int                bookmarkIx,
                               const Duplication& duplication,
                               int                instanceNr,
                               const Options&     options) const
{
    itsBookmarks[bookmarkIx].report(duplication, instanceNr, options);
}

bool BookmarkContainer::same(int a,
                             int b,
                             int longestSame,
                             const char* processedEnd) const
{
    return itsBookmarks[a].sameAs(itsBookmarks[b], longestSame, processedEnd);
}

int BookmarkContainer::nrOfSame(int a, int b) const
{
    return itsBookmarks[a].nrOfSame(itsBookmarks[b]);
}

void BookmarkContainer::sort()
{
    // std::stable_sort(), which is a merge sort, has proved to be much faster
    // than std::sort() in this context.
    std::stable_sort(itsBookmarks.begin(), itsBookmarks.end());
}

void BookmarkContainer::clearWithin(const Duplication& d)
{
    for (int i = 0; i < d.instances; ++i)
    {
        const char* reportStart =
          itsBookmarks[d.indexOf1stInstance + i].itsProcessedText;

        for (size_t ix = 0; ix < itsBookmarks.size() - 1; ++ix)
        {
            const char* t = itsBookmarks[ix].itsProcessedText;
            if (t >= reportStart && t < reportStart + d.longestSame)
                itsBookmarks[ix].clear();
        }
    }
    // Remove all cleared bookmarks while maintaining a sorted array.
    std::vector<Bookmark>::iterator newEnd =
        std::remove_if(itsBookmarks.begin(), itsBookmarks.end(),
                       std::mem_fun_ref(&Bookmark::isCleared));
    itsBookmarks.resize(newEnd - itsBookmarks.begin());
}
