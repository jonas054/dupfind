#ifndef OPTIONS_HH
#define OPTIONS_HH

#include <string>
#include <vector>

using std::string;
using std::vector;

class Options
{
public:
    enum TotalReport { NO_TOTAL, RESTRICTED_TOTAL, UNRESTRICTED_TOTAL };
    enum ExtFlagMode { SHOW_EXT_FLAGS, HIDE_EXT_FLAGS };

    int            nrOfWantedReports;
    bool           isVerbose;
    TotalReport    totalReport;
    int            minLength;
    int            proximityFactor;
    bool           wordMode;
    vector<string> foundFiles;
    vector<string> excludes;

  Options(int argc, char* argv[]);

  static void printUsageAndExit(ExtFlagMode anExtFlagMode, int anExitCode);

private:
  int processFlag(int i, int argc, char* argv[]);

  void processFileName(const char* arg);

  void findFiles(const string&         name,
                 const string&         ending,
                 const vector<string>& excludes,
                 vector<string>&       output);
};

#endif
