#include "parser.hh"
#include "file.hh" // SPECIAL_EOF
#include "bookmark.hh"
#include "bookmark_container.hh"

#include <iostream>
#include <map>
#include <string>
#include <cstring> // strncmp

using std::map;
using std::string;

struct Parser::Key
{
    Key(Language l, State s, char e): language(l), oldState(s), event(e) {}
    bool operator<(const Key& k) const
    {
        return (language < k.language ? true :
                language > k.language ? false :
                oldState < k.oldState ? true :
                oldState > k.oldState ? false :
                event < k.event);
    }
    Language language;
    State oldState;
    char  event;
};

struct Parser::Value
{
    State  newState;
    Action action;
};

static const char ANY = '\0';

const char* Parser::stateToString(Parser::State s)
{
    switch (s)
    {
    case NORMAL: return "NORMAL";
    case COMMENT_START: return "COMMENT_START";
    case C_COMMENT: return "C_COMMENT";
    case C_COMMENT_END: return "C_COMMENT_END";
    case REGEXP: return "REGEXP";
    case DOUBLE_QUOTE: return "DOUBLE_QUOTE";
    case DOUBLE_QUOTE_1: return "DOUBLE_QUOTE_1";
    case DOUBLE_QUOTE_2: return "DOUBLE_QUOTE_2";
    case DOUBLE_QUOTE_3: return "DOUBLE_QUOTE_3";
    case DOUBLE_QUOTE_4: return "DOUBLE_QUOTE_4";
    case DOUBLE_QUOTE_5: return "DOUBLE_QUOTE_5";
    case SINGLE_QUOTE_1: return "SINGLE_QUOTE_1";
    case SINGLE_QUOTE_2: return "SINGLE_QUOTE_2";
    case SINGLE_QUOTE_3: return "SINGLE_QUOTE_3";
    case SINGLE_QUOTE_4: return "SINGLE_QUOTE_4";
    case SINGLE_QUOTE_5: return "SINGLE_QUOTE_5";
    case SINGLE_QUOTE: return "SINGLE_QUOTE";
    case ESCAPE_DOUBLE: return "ESCAPE_DOUBLE";
    case ESCAPE_SINGLE: return "ESCAPE_SINGLE";
    case SKIP_TO_EOL: return "SKIP_TO_EOL";
    case SPACE: return "SPACE";
    case NO_STATE: return "NO_STATE";
    default: return "unknown";
    }
}

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
    {
        state = processChar(state, matrix, i);
        // std::cout << stateToString(state) << ' '
        //           << Bookmark::getChar(i) << "\n";
    }

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

    Language language = getLanguage(Bookmark::getFileName(i));
    Matrix::const_iterator it;
    if ((it = matrix.find({ language, state, c }))   != matrix.end() ||
        (it = matrix.find({ language, state, ANY })) != matrix.end() ||
        (it = matrix.find({ ALL, state, c }))   != matrix.end() ||
        (it = matrix.find({ ALL, state, ANY })) != matrix.end())
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

static bool endsWith(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() &&
      str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

Parser::Language Parser::getLanguage(const string& fileName)
{
    if (endsWith(fileName, ".c") or
        endsWith(fileName, ".cc") or
        endsWith(fileName, ".h") or
        endsWith(fileName, ".hh") or
        endsWith(fileName, ".hpp") or
        endsWith(fileName, ".cpp") or
        endsWith(fileName, ".java"))
    {
        return C_FAMILY;
    }
    if (endsWith(fileName, ".erl") or
        endsWith(fileName, ".hrl"))
    {
        return ERLANG;
    }
    if (endsWith(fileName, ".rb") or
        endsWith(fileName, ".sh") or
        endsWith(fileName, ".js") or
        endsWith(fileName, ".pl"))
    {
        return SCRIPT;
    }
    if (endsWith(fileName, ".py"))
    {
        return PYTHON;
    }
    return ALL;
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
    itsProcessedText[procIx] = c;
    return Bookmark(originalIndex, &itsProcessedText[procIx++]);
}

const Parser::Matrix& Parser::codeBehavior() const
{
    static Matrix m = {
        // language,  oldState    event     newState        action
        { { ALL,      NORMAL,     '/'  }, { COMMENT_START,  NA           } },
        { { ALL,      NORMAL,     '"'  }, { DOUBLE_QUOTE,   ADD_CHAR     } },
        { { ALL,      NORMAL,     '\'' }, { SINGLE_QUOTE,   ADD_CHAR     } },
        { { ALL,      NORMAL,     '\n' }, { NORMAL,         ADD_BOOKMARK } },
        { { ALL,      NORMAL,     ' '  }, { NORMAL,         NA           } },
        { { ALL,      NORMAL,     '\t' }, { NORMAL,         NA           } },
        { { ALL,      NORMAL,     '#'  }, { SKIP_TO_EOL,    NA           } },
        { { ERLANG,   NORMAL,     '#'  }, { NORMAL,         NA           } },
        { { ERLANG,   NORMAL,     '%'  }, { SKIP_TO_EOL,    NA           } },
        { { PYTHON,   NORMAL,     '"'  }, { DOUBLE_QUOTE_1, NA           } },
        { { PYTHON,   NORMAL,     '\'' }, { SINGLE_QUOTE_1, NA           } },
        // See special handling of NORMAL in code.

        { { PYTHON, DOUBLE_QUOTE_1, '"' }, { DOUBLE_QUOTE_2, NA       } },
        { { PYTHON, DOUBLE_QUOTE_2, '"' }, { DOUBLE_QUOTE_3, NA       } },
        { { PYTHON, DOUBLE_QUOTE_3, '"' }, { DOUBLE_QUOTE_4, NA       } },
        { { PYTHON, DOUBLE_QUOTE_4, '"' }, { DOUBLE_QUOTE_5, NA       } },
        { { PYTHON, DOUBLE_QUOTE_5, '"' }, { NORMAL,         NA       } },
        { { PYTHON, DOUBLE_QUOTE_1, ANY }, { DOUBLE_QUOTE,   ADD_CHAR } },
        { { PYTHON, DOUBLE_QUOTE_2, ANY }, { NORMAL,         ADD_CHAR } },
        { { PYTHON, DOUBLE_QUOTE_3, ANY }, { DOUBLE_QUOTE_3, NA       } },
        { { PYTHON, DOUBLE_QUOTE_4, ANY }, { DOUBLE_QUOTE_3, NA       } },
        { { PYTHON, DOUBLE_QUOTE_5, ANY }, { DOUBLE_QUOTE_3, NA       } },

        { { PYTHON, SINGLE_QUOTE_1, '\'' }, { SINGLE_QUOTE_2, NA       } },
        { { PYTHON, SINGLE_QUOTE_2, '\'' }, { SINGLE_QUOTE_3, NA       } },
        { { PYTHON, SINGLE_QUOTE_3, '\'' }, { SINGLE_QUOTE_4, NA       } },
        { { PYTHON, SINGLE_QUOTE_4, '\'' }, { SINGLE_QUOTE_5, NA       } },
        { { PYTHON, SINGLE_QUOTE_5, '\'' }, { NORMAL,         NA       } },
        { { PYTHON, SINGLE_QUOTE_1, ANY  }, { SINGLE_QUOTE,   ADD_CHAR } },
        { { PYTHON, SINGLE_QUOTE_2, ANY  }, { NORMAL,         ADD_CHAR } },
        { { PYTHON, SINGLE_QUOTE_3, ANY  }, { SINGLE_QUOTE_3, NA       } },
        { { PYTHON, SINGLE_QUOTE_4, ANY  }, { SINGLE_QUOTE_3, NA       } },
        { { PYTHON, SINGLE_QUOTE_5, ANY  }, { SINGLE_QUOTE_3, NA       } },

        { { ALL, DOUBLE_QUOTE,  '\\' }, { ESCAPE_DOUBLE, ADD_CHAR     } },
        { { ALL, DOUBLE_QUOTE,  '"'  }, { NORMAL,        ADD_CHAR     } },
        { { ALL, DOUBLE_QUOTE,  ANY  }, { DOUBLE_QUOTE,  ADD_CHAR     } },
        { { ALL, DOUBLE_QUOTE,  '\n' }, { NORMAL,        ADD_BOOKMARK } }, // 1
        { { ALL, SINGLE_QUOTE,  '\\' }, { ESCAPE_SINGLE, ADD_CHAR     } },
        { { ALL, SINGLE_QUOTE,  '\'' }, { NORMAL,        ADD_CHAR     } },
        { { ALL, SINGLE_QUOTE,  ANY  }, { SINGLE_QUOTE,  ADD_CHAR     } },
        { { ALL, SINGLE_QUOTE,  '\n' }, { NORMAL,        ADD_BOOKMARK } }, // 1
        { { ALL, ESCAPE_SINGLE, ANY  }, { SINGLE_QUOTE,  ADD_CHAR     } },
        { { ALL, ESCAPE_DOUBLE, ANY  }, { DOUBLE_QUOTE,  ADD_CHAR     } },
        // 1: probably a mistake if quote reaches end-of-line.

        { { SCRIPT, NORMAL, '/'  }, { REGEXP,      ADD_CHAR } },
        { { SCRIPT, REGEXP, '/'  }, { SKIP_TO_EOL, NA       } },
        { { SCRIPT, REGEXP, '*'  }, { C_COMMENT,   NA       } },
        { { SCRIPT, REGEXP, '\n' }, { NORMAL,      ADD_CHAR } },

        { { ALL, COMMENT_START, '*'  }, { C_COMMENT,     NA                 } },
        { { ALL, COMMENT_START, '/'  }, { SKIP_TO_EOL,   NA                 } },
        { { ALL, COMMENT_START, ANY  }, { NORMAL,        ADD_SLASH_AND_CHAR } },
        { { ALL, SKIP_TO_EOL,   '\n' }, { NORMAL,        ADD_BOOKMARK       } },
        { { ALL, C_COMMENT,     '*'  }, { C_COMMENT_END, NA                 } },
        { { ALL, C_COMMENT_END, '/'  }, { NORMAL,        NA                 } },
        { { ALL, C_COMMENT_END, '*'  }, { C_COMMENT_END, NA                 } },
        { { ALL, C_COMMENT_END, ANY  }, { C_COMMENT,     NA                 } },

        { { ALL, NO_STATE,      ANY  }, { NO_STATE,      NA                 } }
    };
    return m;
}

const Parser::Matrix& Parser::textBehavior() const
{
    static Matrix m = {
        //  oldState  event     newState  action
        { { ALL, NORMAL,   ' '  }, { SPACE,    NA        } },
        { { ALL, NORMAL,   '\t' }, { SPACE,    NA        } },
        { { ALL, NORMAL,   '\r' }, { SPACE,    NA        } },
        { { ALL, NORMAL,   '\n' }, { SPACE,    NA        } },
        { { ALL, NORMAL,   '' }, { SPACE,    NA        } },
        { { ALL, NORMAL,   ANY  }, { NORMAL,   ADD_CHAR  } },

        { { ALL, SPACE,    ' '  }, { SPACE,    NA        } },
        { { ALL, SPACE,    '\t' }, { SPACE,    NA        } },
        { { ALL, SPACE,    '\r' }, { SPACE,    NA        } },
        { { ALL, SPACE,    '\n' }, { SPACE,    NA        } },
        { { ALL, SPACE,    '' }, { SPACE,    NA        } },
        { { ALL, SPACE,    ANY  }, { NORMAL,   ADD_SPACE } },

        { { ALL, NO_STATE, ANY  }, { NO_STATE, NA        } }
    };
    return m;
}
