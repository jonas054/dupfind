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
using std::ifstream;

// We use ASCII code 7 as a special value denoting EOF.
const char SPECIAL_EOF = '\x7';

extern string readFileIntoString(const char* aFileName)
{
    struct stat s;
    if ((stat(aFileName, &s) == 0) && (s.st_mode & S_IFDIR))
    {
        cerr << "dupfind: " << aFileName << " is a directory.\n";
        exit(EXIT_FAILURE);
    }

    std::ifstream in(aFileName, ios::in);
    if (!in.good())
    {
        cerr << "dupfind: File " << aFileName << " not found.\n";
        exit(EXIT_FAILURE);
    }
    in.unsetf(ios::skipws);
    in.seekg(0, ios::end);
    std::streampos fsize = in.tellg() + (std::streamoff)1;
    in.seekg(0, ios::beg);

    std::vector<char> vec;
    vec.reserve(fsize);
    copy(std::istream_iterator<char>(in), std::istream_iterator<char>(),
         back_inserter(vec));

    vec.push_back(SPECIAL_EOF);

    return string(vec.begin(), vec.end());
}
