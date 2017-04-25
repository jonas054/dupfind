# dupfind
Duplication finder for source code and other text files

## Usage
```
       dupfind [-v] [-w] [-<n>|-m<n>] [-p<n>] [-x <substring>] [-e <ending> ...]
       dupfind [-v] [-w] [-<n>|-m<n>] [-p<n>] <files>
       dupfind -t|-T [-v] [-w] <files>
       -v:    verbose, print strings that are duplicated
       -w:    calculate duplication based on words rather than lines
       -10:   report the 10 longest duplications instead of 5, which is default
       -m300: report all duplications that are at least 300 characters long
       -x:    exclude paths matching substring when searching for files with -e
              (-x must come before the -e option it applies to)
       -e:    search recursively for files whose names end with the given ending
              (several -e options can be given)
       -p50:  use 50% proximity (more but shorter matches); 90% is default
       -t:    set -m100 and sum up the total duplication
       -T:    same as -t but accept any file (test code etc.)
```
