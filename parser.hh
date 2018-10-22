#ifndef PARSER_HH
#define PARSER_HH

#include <cstdlib> // size_t
#include <map>
#include <string>

class Bookmark;
class BookmarkContainer;

class Parser
{
    enum Language { C_FAMILY, SCRIPT, ERLANG, PYTHON, ALL };

    enum State
    {
        NORMAL, COMMENT_START, C_COMMENT, C_COMMENT_END, DOUBLE_QUOTE,
        DOUBLE_QUOTE_1, DOUBLE_QUOTE_2, DOUBLE_QUOTE_3,
        DOUBLE_QUOTE_4, DOUBLE_QUOTE_5,
        SINGLE_QUOTE_1, SINGLE_QUOTE_2, SINGLE_QUOTE_3,
        SINGLE_QUOTE_4, SINGLE_QUOTE_5,
        SINGLE_QUOTE, ESCAPE_DOUBLE, ESCAPE_SINGLE, SKIP_TO_EOL, SPACE,
        REGEXP, NO_STATE
    };

    enum Action { NA, ADD_CHAR, ADD_SLASH_AND_CHAR, ADD_BOOKMARK, ADD_SPACE };

    struct Value;
    struct Cell;
    struct Key;

    typedef std::map<Key, Value> Matrix;

public:
    Parser(BookmarkContainer& container): timeForNewBookmark(true),
                                          itsContainer(container) {}
    const char* process(bool wordMode);

private:
    State         processChar(State state, const Matrix& matrix, size_t i);
    void          performAction(Action action, char c, size_t i);
    Bookmark      addChar(char c, int originalIndex);
    const Matrix& codeBehavior() const;
    const Matrix& textBehavior() const;
    Language      getLanguage(const std::string& fileName);
    const char*   stateToString(State s);

    bool               timeForNewBookmark;
    BookmarkContainer& itsContainer;
    char*              itsProcessedText;
};

#endif
