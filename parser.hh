#ifndef PARSER_HH
#define PARSER_HH

class BookmarkContainer;

class Parser
{
public:
    static const char* process(BookmarkContainer& container, bool wordMode);
};

#endif
