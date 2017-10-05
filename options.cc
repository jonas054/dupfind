#include "options.hh"
#include "bookmark.hh"

#include <algorithm>  // sort
#include <iostream>   // ostream, cout, cerr, endl
#include <cstring>    // strcmp, strncmp, strlen
#include <climits>    // INT_MAX
#include <sys/stat.h> // struct stat
#include <string>
#include <vector>
#include <dirent.h>   // DIR, opendir(), readdir(), closedir()

using std::cerr;
using std::endl;
using std::string;
using std::vector;

Options::Options(): nrOfWantedReports(5),
                    isVerbose(false),
                    totalReport(NO_TOTAL),
                    minLength(10),
                    proximityFactor(90),
                    wordMode(false)
{
}

void Options::parse(int argc, char* argv[])
{
    if (argc == 1)
        printUsageAndExit(Options::HIDE_EXT_FLAGS, EXIT_SUCCESS);

    for (int i = 1; i < argc; ++i)
        if (argv[i][0] == '-')
            i = processFlag(i, argc, argv);
        else
            processFileName(argv[i]);

    if (Bookmark::totalLength() == 0)
    {
        std::cout << "No files found" << std::endl;
        printUsageAndExit(Options::SHOW_EXT_FLAGS, EXIT_FAILURE);
    }
}

int Options::processFlag(int i, int argc, char* argv[])
{
    const char* arg = argv[i];
    const char flag = arg[1];

    if (isdigit(flag))
    {
        nrOfWantedReports = -atoi(arg);
        return i;
    }

    switch (flag)
    {
    case 't':
    case 'T':
        totalReport =
            (flag == 't') ? RESTRICTED_TOTAL : UNRESTRICTED_TOTAL;

        nrOfWantedReports = INT_MAX;
        minLength         = 100;
        proximityFactor   = 100;
        break;
    case 'e': {
        bool isRestrictedTotal = (totalReport == RESTRICTED_TOTAL);
        for (int k = 1; not isRestrictedTotal && k < argc; ++k)
            if (argv[k][0] == '-' && argv[k][1] == 't')
                isRestrictedTotal = true;
        findFiles(".", argv[++i]);
        std::sort(foundFiles.begin(), foundFiles.end());
        for (size_t ii = 0; ii < foundFiles.size(); ++ii)
        {
            if (not isRestrictedTotal ||
                foundFiles[ii].find("test") == string::npos)
            {
                Bookmark::addFile(foundFiles[ii]);
            }
        }
        foundFiles.clear();
        excludes.clear();
        break;
    }
    case 'v':
        isVerbose = true;
        break;
    case 'x':
        excludes.push_back(argv[++i]);
        break;
    case 'w':
        wordMode = true;
        break;
    case 'm':
        minLength = atoi((arg[2] == '\0') ? argv[++i] : &arg[2]);
        nrOfWantedReports = INT_MAX;
        break;
    case 'p':
        proximityFactor = atoi((arg[2] == '\0') ? argv[++i] : &arg[2]);
        if (proximityFactor < 1 || proximityFactor > 100)
        {
            cerr << "Proximity factor must be between 1 and 100 (inclusive)."
                 << endl;
            printUsageAndExit(SHOW_EXT_FLAGS, EXIT_FAILURE);
        }
        break;
    case 'h':
        printUsageAndExit(SHOW_EXT_FLAGS, EXIT_SUCCESS);
    default:
        printUsageAndExit(SHOW_EXT_FLAGS, EXIT_FAILURE);
    }
    return i;
}

void Options::findFiles(const string& dirName, const string& ending)
{
    DIR* dir = opendir(dirName.c_str());

    if (dir == NULL)
        return;

    for (struct dirent* entry = readdir(dir); entry; entry = readdir(dir))
    {
        const string entryName = entry->d_name;
        if (entryName == "." || entryName == "..")
            continue;
        const string path = dirName + "/" + entry->d_name;
        bool toBeExcluded = false;
        for (size_t e = 0; e < excludes.size(); ++e)
            if (path.find(excludes[e]) != string::npos)
                toBeExcluded = true;
        if (toBeExcluded)
            continue;
        if (path.length() > ending.length() &&
            path.substr(path.length() - ending.length()) == ending)
        {
            foundFiles.push_back(path);
        }
        findFiles(path, ending);
    }
    closedir(dir);
}

void Options::processFileName(const string& arg)
{
    if (totalReport == RESTRICTED_TOTAL && arg.find("test") != string::npos)
    {
        cerr << "The file " << arg << " is not included in the total "
             << "duplication calculations. Use -T if you want to include it."
             << endl;
    }
    else
        Bookmark::addFile(arg);
}

void Options::printUsageAndExit(ExtFlagMode extFlagMode, int exitCode)
{
    std::ostream& os = (exitCode == 0) ? std::cout : cerr;
    os << "Usage: dupfind [-v] [-w] [-<n>|-m<n>] "
       << (extFlagMode == SHOW_EXT_FLAGS ? "[-p<n>] " : "")
       << "[-x <substring>] [-e <ending> ...]\n"
       << "       dupfind [-v] [-w] [-<n>|-m<n>] "
       << (extFlagMode == SHOW_EXT_FLAGS ? "[-p<n>] " : "") << "<files>\n"
       << "       dupfind -t"
       << (extFlagMode == SHOW_EXT_FLAGS ? "|-T" : "")
       << " [-v] [-w] <files>\n"
       << "       -v:    verbose, print strings that are duplicated\n"
       << "       -w:    calculate duplication based on words rather than "
       << "lines\n"
       << "       -10:   report the 10 longest duplications instead of 5,"
       << " which is default\n"
       << "       -m300: report all duplications that are at least 300"
       << " characters long\n"
       << "       -x:    exclude paths matching substring when searching for"
       << " files with -e\n"
       << "              (several -x options can be given and -x must come "
       << "before the -e\n"
       << "              option it applies to)\n"
       << "       -e:    search recursively from the current directory for "
       << "files whose\n"
       << "              names end with the given ending (several -e options "
       << "can be given)\n";
    if (extFlagMode == SHOW_EXT_FLAGS)
        os << "       -p50:  use 50% proximity (more but shorter matches); "
           << "90% is default\n";
    os << "       -t:    set -m100 and sum up the total duplication\n";
    if (extFlagMode == SHOW_EXT_FLAGS)
        os << "       -T:    same as -t but accept any file (test code etc.)"
           << endl;
    exit(exitCode);
}
