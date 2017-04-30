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

Options::Options(int argc, char* argv[]): nrOfWantedReports(5),
                                          isVerbose(false),
                                          totalReport(NO_TOTAL),
                                          minLength(10),
                                          proximityFactor(90),
                                          wordMode(false)
{
    for (int i = 1; i < argc; ++i)
        if (argv[i][0] == '-')
            i = processFlag(i, argc, argv);
        else
            processFileName(argv[i]);
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
        for (int k = 1; !isRestrictedTotal && k < argc; ++k)
            if (argv[k][0] == '-' && argv[k][1] == 't')
                isRestrictedTotal = true;
        findFiles(".", argv[++i], excludes, foundFiles);
        std::sort(foundFiles.begin(), foundFiles.end());
        for (size_t ii = 0; ii < foundFiles.size(); ++ii)
        {
            if (!isRestrictedTotal ||
                foundFiles[ii].find("test") == string::npos)
            {
                Bookmark::addFile(foundFiles[ii].c_str());
            }
        }
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
        if (arg[2] == '\0')
            printUsageAndExit(HIDE_EXT_FLAGS, EXIT_FAILURE);

        nrOfWantedReports = INT_MAX;
        minLength         = atoi(&arg[2]);
        break;
    case 'p':
        if (arg[2] == '\0')
            printUsageAndExit(SHOW_EXT_FLAGS, EXIT_FAILURE);

        proximityFactor = atoi(&arg[2]);
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

void Options::findFiles(const string&         dirName,
                        const string&         ending,
                        const vector<string>& excludes,
                        vector<string>&       output)
{
    DIR* dir = opendir(dirName.c_str());

    if (!dir)
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
            output.push_back(path);
        }
        findFiles(path, ending, excludes, output);
    }
    closedir(dir);
}

void Options::processFileName(const char* arg)
{
    if (totalReport == RESTRICTED_TOTAL &&
        (string(arg).find("test") != string::npos ||
         (string(arg).find("_R") != string::npos &&
          isdigit(arg[string(arg).find("_R") + 2]))))
    {
        cerr << "The file " << arg << " is not included in the total "
             << "duplication calculations. Use -T if you want to include it."
             << endl;
    }
    else
        Bookmark::addFile(arg);
}

void Options::printUsageAndExit(ExtFlagMode anExtFlagMode, int anExitCode)
{
    std::ostream& os = (anExitCode == 0) ? std::cout : cerr;
    os << "Usage: dupfind [-v] [-w] [-<n>|-m<n>] "
       << (anExtFlagMode == SHOW_EXT_FLAGS ? "[-p<n>] " : "")
       << "[-x <substring>] [-e <ending> ...]\n"
       << "       dupfind [-v] [-w] [-<n>|-m<n>] "
       << (anExtFlagMode == SHOW_EXT_FLAGS ? "[-p<n>] " : "") << "<files>\n"
       << "       dupfind -t"
       << (anExtFlagMode == SHOW_EXT_FLAGS ? "|-T" : "")
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
       << "              (-x must come before the -e option it applies to)\n"
       << "       -e:    search recursively for files whose names end with"
       << " the given ending\n"
       << "              (several -e options can be given)\n";
    if (anExtFlagMode == SHOW_EXT_FLAGS)
    {
        os << "       -p50:  use 50% proximity (more but shorter matches); "
           << "90% is default\n";
    }
    os << "       -t:    set -m100 and sum up the total duplication\n";
    if (anExtFlagMode == SHOW_EXT_FLAGS)
    {
        os << "       -T:    same as -t but accept any file (test code etc.)"
           << endl;
    }
    exit(anExitCode);
}
