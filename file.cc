#include "file.hh"
#include <iostream>   // cerr
#include <vector>
#include <sys/stat.h> // struct stat
#include <cstdlib>    // EXIT_FAILURE
#include <fstream>    // ifstream
#include <iterator>   // istream_iterator

using std::string;
using std::cerr;
using std::ios;
using std::istream_iterator;

// We use ASCII code 7 as a special value denoting EOF.
const char SPECIAL_EOF = '\x7';

extern string readFileIntoString(const char* fileName)
{
    struct stat s;
    if ((stat(fileName, &s) == 0) && (s.st_mode & S_IFDIR))
    {
        cerr << "dupfind: " << fileName << " is a directory.\n";
        exit(EXIT_FAILURE);
    }

    std::ifstream in(fileName, ios::in);
    if (!in.good())
    {
        cerr << "dupfind: File " << fileName << " not found.\n";
        exit(EXIT_FAILURE);
    }
    in.unsetf(ios::skipws);
    in.seekg(0, ios::end);
    std::streampos fsize = in.tellg() + (std::streamoff)1;
    in.seekg(0, ios::beg);

    std::vector<char> vec;
    vec.reserve(fsize);
    copy(istream_iterator<char>(in), istream_iterator<char>(),
         back_inserter(vec));

    vec.push_back(SPECIAL_EOF);

    return string(vec.begin(), vec.end());
}
