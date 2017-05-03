#include "parser.hh"
#include "file.hh" // SPECIAL_EOF
#include "bookmark.hh"
#include "bookmark_container.hh"

#include <map>
#include <cstring> // strncmp

using std::map;

static const char ANY = '\0';

/**
 * Reads the original text into a processed text, which is returned. Also sets
 * the bookmarks to point into the two strings.
 */
const char* Parser::process(BookmarkContainer& bc, bool wordMode)
{
    container = &bc;

    map<Key, Value> matrix;
    const Cell* cells = wordMode ? textBehavior() : codeBehavior();

    for (int i = 0; cells[i].oldState != NO_STATE; ++i)
    {
        Key k(cells[i].oldState, cells[i].event);
        Value v;
        v.newState = cells[i].newState;
        v.action   = cells[i].action;
        matrix[k] = v;
    }

    processed = new char[Bookmark::totalLength()];

    State state = NORMAL;
    for (size_t i = 0; i < Bookmark::totalLength(); ++i)
        state = processChar(state, matrix, i);

    addChar('\0', Bookmark::totalLength());

    return processed;
}

Parser::State Parser::processChar(State                  state,
                                  const map<Key, Value>& matrix,
                                  size_t                 i)
{
    const char c = Bookmark::getChar(i);
    // Apparently there can be zeroes in the total string, but only when
    // running on some machines. Don't know why.
    if (c == '\0')
        return state;

    if (c == SPECIAL_EOF)
    {
        addChar(c, i);
        return NORMAL;
    }

    map<Key, Value>::const_iterator it;
    if ((it = matrix.find(Key(state, c)))   != matrix.end() ||
        (it = matrix.find(Key(state, ANY))) != matrix.end())
    {
        performAction(it->second.action, c, i);
        return it->second.newState;
    }
    if (state == NORMAL && not isspace(c))
    { // Handle state/event pair that can't be handled by The Matrix.
        if (timeForNewBookmark && c != '}')
            if (c == '#' ||
                strncmp("import", &Bookmark::getChar(i), 6) == 0 ||
                strncmp("using",  &Bookmark::getChar(i), 5) == 0)
            {
                state = SKIP_TO_EOL;
            }
            else
                container->addBookmark(addChar(c, i));
        else
            addChar(c, i);

        timeForNewBookmark = false;
    }
    return state;
}

void Parser::performAction(Action action, char c, size_t i)
{
    switch (action)
    {
    case ADD_SLASH_AND_CHAR:
        addChar('/', i-1);
        if (not isspace(c))
            addChar(c, i);
        break;
    case ADD_CHAR:
    {
        const Bookmark bm = addChar(c, i);
        if (timeForNewBookmark)
        {
            container->addBookmark(bm);
            timeForNewBookmark = false;
        }
        break;
    }
    case ADD_BOOKMARK:
        timeForNewBookmark = true;
        break;
    case ADD_SPACE:
        addChar(' ', i);
        container->addBookmark(addChar(c, i));
        break;
    case NA:
        break;
    }
}

/**
 * Adds a character to the processed string and sets a bookmark, which is then
 * returned.
 */
Bookmark Parser::addChar(char c, int originalIndex)
{
    static int procIx;
    Bookmark bookmark(originalIndex, &processed[procIx]);
    processed[procIx++] = c;
    return bookmark;
}

const Parser::Cell* Parser::codeBehavior() const
{
    static Cell c[] = {
        // oldState      event newState       action
        { NORMAL,        '/',  COMMENT_START, NA                 },
        { NORMAL,        '"',  DOUBLE_QUOTE,  ADD_CHAR           },
        { NORMAL,        '\'', SINGLE_QUOTE,  ADD_CHAR           },
        { NORMAL,        '\n', NORMAL,        ADD_BOOKMARK       },
        // See special handling of NORMAL in code further down.

        { DOUBLE_QUOTE,  '\\', ESCAPE_DOUBLE, ADD_CHAR           },
        { DOUBLE_QUOTE,  '"',  NORMAL,        ADD_CHAR           },
        { DOUBLE_QUOTE,  ANY,  DOUBLE_QUOTE,  ADD_CHAR           },
        { DOUBLE_QUOTE,  '\n', NORMAL,        ADD_BOOKMARK       }, // (1)
        { SINGLE_QUOTE,  '\\', ESCAPE_SINGLE, ADD_CHAR           },
        { SINGLE_QUOTE,  '\'', NORMAL,        ADD_CHAR           },
        { SINGLE_QUOTE,  ANY,  SINGLE_QUOTE,  ADD_CHAR           },
        { SINGLE_QUOTE,  '\n', NORMAL,        ADD_BOOKMARK       }, // (1)
        { ESCAPE_SINGLE, ANY,  SINGLE_QUOTE,  ADD_CHAR           },
        { ESCAPE_DOUBLE, ANY,  DOUBLE_QUOTE,  ADD_CHAR           },
        // (1) probably a mistake if quote reaches end-of-line.

        { COMMENT_START, '*',  C_COMMENT,     NA                 },
        { COMMENT_START, '/',  SKIP_TO_EOL,   NA                 },
        { COMMENT_START, ANY,  NORMAL,        ADD_SLASH_AND_CHAR },
        { SKIP_TO_EOL,   '\n', NORMAL,        ADD_BOOKMARK       },
        { C_COMMENT,     '*',  C_COMMENT_END, NA                 },
        { C_COMMENT_END, '/',  NORMAL,        NA                 },
        { C_COMMENT_END, '*',  C_COMMENT_END, NA                 },
        { C_COMMENT_END, ANY,  C_COMMENT,     NA                 },

        { NO_STATE,      ANY,  NO_STATE,      NA                 }
    };
    return c;
}

const Parser::Cell* Parser::textBehavior() const
{
    static Cell c[] = {
        // oldState event newState action
        { NORMAL,   ' ',  SPACE,    NA       },
        { NORMAL,   '\t', SPACE,    NA       },
        { NORMAL,   '\r', SPACE,    NA       },
        { NORMAL,   '\n', SPACE,    NA       },
        { NORMAL,   '', SPACE,    NA       },
        { NORMAL,   ANY,  NORMAL,   ADD_CHAR },

        { SPACE,    ' ',  SPACE,    NA        },
        { SPACE,    '\t', SPACE,    NA        },
        { SPACE,    '\r', SPACE,    NA        },
        { SPACE,    '\n', SPACE,    NA        },
        { SPACE,    '', SPACE,    NA        },
        { SPACE,    ANY,  NORMAL,   ADD_SPACE },

        { NO_STATE, ANY,  NO_STATE, NA        }
    };
    return c;
}
