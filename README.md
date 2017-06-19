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
              (several -x options can be given and -x must come before the -e
              option it applies to)
       -e:    search recursively from the current directory for files whose
              names end with the given ending (several -e options can be given)
       -p50:  use 50% proximity (more but shorter matches); 90% is default
       -t:    set -m100 and sum up the total duplication
       -T:    same as -t but accept any file (test code etc.)
```

## Description

The design goal of dupfind is to be a simple and very fast generic copy paste
detector for most programming languages and for other text files.

## Modes

The basic functionality is that there are two modes; normal mode and word mode,
the latter selected by giving the `-w` option.

### Normal Mode

In normal mode, which is suited for analyzing source code, the analyzed files
are read into memory and all white space and comments are removed from the
text. The comment removal works for all programming languages that use either
`/* ... */`, `// ...` or `# ...` as comments. This means that C, C++, C#, Java,
and similar languages are fully supported. Perl, Ruby, and Python are mostly
supported, but they also have other comment syntax that is not handled. Lisp
and SQL are not supported when it comes to comments.

### Word Mode

In word mode, only line breaks are removed and any sequence of whitespace is
replaced by a single space.

## Reporting

When the preprocessing is done, chunks of the text are compared to each other
and the longest common sequences of text are reported.
