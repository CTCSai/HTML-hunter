#ifndef XGETOPT_H
#define XGETOPT_H

extern int optind, opterr;
extern TCHAR *optarg;

int getopt(int argc, TCHAR*argv[], TCHAR*optstring);

#endif //XGETOPT_H
