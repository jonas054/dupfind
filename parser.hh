#ifndef PARSER_HH
#define PARSER_HH

#include <cstdlib> // size_t
#include <map>

class Bookmark;
class BookmarkContainer;

class Parser
{
    enum State
    {
        NORMAL, COMMENT_START, C_COMMENT, C_COMMENT_END, DOUBLE_QUOTE,
        SINGLE_QUOTE, ESCAPE_DOUBLE, ESCAPE_SINGLE, SKIP_TO_EOL, SPACE,
        NO_STATE
    };

    enum Action { NA, ADD_CHAR, ADD_SLASH_AND_CHAR, ADD_BOOKMARK, ADD_SPACE };

    struct Cell { State oldState; char event; State newState; Action action; };

    struct Value { State newState; Action action; };

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

public:
    Parser(): timeForNewBookmark(true) {}

    const char* process(BookmarkContainer& container, bool wordMode);

private:
    State       processChar(State                       state,
                            const std::map<Key, Value>& matrix,
                            size_t                      i);
    void        performAction(Action action, char c, size_t i);
    Bookmark    addChar(char c, int originalIndex);
    const Cell* codeBehavior() const;
    const Cell* textBehavior() const;

    bool               timeForNewBookmark;
    BookmarkContainer* itsContainer;
    char*              itsProcessedText;
};

#endif
