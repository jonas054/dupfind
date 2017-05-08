#include "parser.hh"
#include "file.hh" // SPECIAL_EOF
#include "bookmark.hh"
#include "bookmark_container.hh"

#include <map>
#include <string>
#include <cstring> // strncmp

using std::map;
using std::string;

struct Parser::Key
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

struct Parser::Value
{
    State  newState;
    Action action;
};

static const char ANY = '\0';

/**
 * Reads the original text into a processed text, which is returned. Also sets
 * the bookmarks to point into the two strings.
 */
const char* Parser::process(bool wordMode)
{
    const Matrix& matrix = wordMode ? textBehavior() : codeBehavior();

    itsProcessedText = new char[Bookmark::totalLength()];

    State state = NORMAL;
    for (size_t i = 0; i < Bookmark::totalLength(); ++i)
        state = processChar(state, matrix, i);

    addChar('\0', Bookmark::totalLength());

    return itsProcessedText;
}

static bool lookaheadIs(const string& s, const char& c)
{
    return strncmp(s.c_str(), &c, s.length()) == 0;
}

Parser::State Parser::processChar(State         state,
                                  const Matrix& matrix,
                                  size_t        i)
{
    static const string imports = "import";
    static const string usings = "using";
    const char& c = Bookmark::getChar(i);
    // Apparently there can be zeroes in the total string, but only when
    // running on some machines. Don't know why.
    if (c == '\0')
        return state;

    if (c == SPECIAL_EOF)
    {
        addChar(c, i);
        return NORMAL;
    }

    Matrix::const_iterator it;
    if ((it = matrix.find({ state, c }))   != matrix.end() ||
        (it = matrix.find({ state, ANY })) != matrix.end())
    {
        performAction(it->second.action, c, i);
        return it->second.newState;
    }
    if (state == NORMAL)
    { // Handle state/event pair that can't be handled by The Matrix.
        if (timeForNewBookmark && c != '}')
            if (lookaheadIs(imports, c) || lookaheadIs(usings, c))
                state = SKIP_TO_EOL;
            else
                itsContainer.addBookmark(addChar(c, i));
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
            itsContainer.addBookmark(bm);
        timeForNewBookmark = false;
        break;
    }
    case ADD_BOOKMARK:
        timeForNewBookmark = true;
        break;
    case ADD_SPACE:
        addChar(' ', i);
        itsContainer.addBookmark(addChar(c, i));
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
    Bookmark bookmark(originalIndex, &itsProcessedText[procIx]);
    itsProcessedText[procIx++] = c;
    return bookmark;
}

const Parser::Matrix& Parser::codeBehavior() const
{
    static Matrix m = {
        //  oldState       event     newState       action
        { { NORMAL,        '/'  }, { COMMENT_START, NA           } },
        { { NORMAL,        '"'  }, { DOUBLE_QUOTE,  ADD_CHAR     } },
        { { NORMAL,        '\'' }, { SINGLE_QUOTE,  ADD_CHAR     } },
        { { NORMAL,        '\n' }, { NORMAL,        ADD_BOOKMARK } },
        { { NORMAL,        ' '  }, { NORMAL,        NA           } },
        { { NORMAL,        '\t' }, { NORMAL,        NA           } },
        { { NORMAL,        '#'  }, { SKIP_TO_EOL,   NA           } },
        // See special handling of NORMAL in code.

        { { DOUBLE_QUOTE,  '\\' }, { ESCAPE_DOUBLE, ADD_CHAR     } },
        { { DOUBLE_QUOTE,  '"'  }, { NORMAL,        ADD_CHAR     } },
        { { DOUBLE_QUOTE,  ANY  }, { DOUBLE_QUOTE,  ADD_CHAR     } },
        { { DOUBLE_QUOTE,  '\n' }, { NORMAL,        ADD_BOOKMARK } }, // (1)
        { { SINGLE_QUOTE,  '\\' }, { ESCAPE_SINGLE, ADD_CHAR     } },
        { { SINGLE_QUOTE,  '\'' }, { NORMAL,        ADD_CHAR     } },
        { { SINGLE_QUOTE,  ANY  }, { SINGLE_QUOTE,  ADD_CHAR     } },
        { { SINGLE_QUOTE,  '\n' }, { NORMAL,        ADD_BOOKMARK } }, // (1)
        { { ESCAPE_SINGLE, ANY  }, { SINGLE_QUOTE,  ADD_CHAR     } },
        { { ESCAPE_DOUBLE, ANY  }, { DOUBLE_QUOTE,  ADD_CHAR     } },
        // (1) probably a mistake if quote reaches end-of-line.

        { { COMMENT_START, '*'  }, { C_COMMENT,     NA                 } },
        { { COMMENT_START, '/'  }, { SKIP_TO_EOL,   NA                 } },
        { { COMMENT_START, ANY  }, { NORMAL,        ADD_SLASH_AND_CHAR } },
        { { SKIP_TO_EOL,   '\n' }, { NORMAL,        ADD_BOOKMARK       } },
        { { C_COMMENT,     '*'  }, { C_COMMENT_END, NA                 } },
        { { C_COMMENT_END, '/'  }, { NORMAL,        NA                 } },
        { { C_COMMENT_END, '*'  }, { C_COMMENT_END, NA                 } },
        { { C_COMMENT_END, ANY  }, { C_COMMENT,     NA                 } },

        { { NO_STATE,      ANY  }, { NO_STATE,      NA                 } }
    };
    return m;
}

const Parser::Matrix& Parser::textBehavior() const
{
    static Matrix m = {
        //  oldState  event     newState  action
        { { NORMAL,   ' '  }, { SPACE,    NA        } },
        { { NORMAL,   '\t' }, { SPACE,    NA        } },
        { { NORMAL,   '\r' }, { SPACE,    NA        } },
        { { NORMAL,   '\n' }, { SPACE,    NA        } },
        { { NORMAL,   '' }, { SPACE,    NA        } },
        { { NORMAL,   ANY  }, { NORMAL,   ADD_CHAR  } },

        { { SPACE,    ' '  }, { SPACE,    NA        } },
        { { SPACE,    '\t' }, { SPACE,    NA        } },
        { { SPACE,    '\r' }, { SPACE,    NA        } },
        { { SPACE,    '\n' }, { SPACE,    NA        } },
        { { SPACE,    '' }, { SPACE,    NA        } },
        { { SPACE,    ANY  }, { NORMAL,   ADD_SPACE } },
 
        { { NO_STATE, ANY  }, { NO_STATE, NA        } }
    };
    return m;
}
