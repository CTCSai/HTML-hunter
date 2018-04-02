#ifndef XGetopt_cpp
#define XGetopt_cpp



#ifdef WIN32


#include <windows.h>
#include <stdio.h>
#include <tchar.h>



#include "xgetopt.h"


/

TCHAR	*optarg;		// global argument pointer
int		optind = 0; 	// global argv index

int getopt(int argc, TCHAR *argv[], TCHAR *optstring)
{
	static TCHAR *next = NULL;
	if (optind == 0)
		next = NULL;

	optarg = NULL;

	if (next == NULL || *next == _T('\0'))
	{
		if (optind == 0)
			optind++;

		if (optind >= argc || argv[optind][0] != _T('-') || argv[optind][1] == _T('\0'))
		{
			optarg = NULL;
			if (optind < argc)
				optarg = argv[optind];
			return EOF;
		}

		if (_tcscmp(argv[optind], _T("--")) == 0)
		{
			optind++;
			optarg = NULL;
			if (optind < argc)
				optarg = argv[optind];
			return EOF;
		}

		next = argv[optind];
		next++;		// skip past -
		optind++;
	}

	TCHAR c = *next++;
	TCHAR *cp = _tcschr(optstring, c);

	if (cp == NULL || c == _T(':'))
		return _T('?');

	cp++;
	if (*cp == _T(':'))
	{
		if (*next != _T('\0'))
		{
			optarg = next;
			next = NULL;
		}
		else if (optind < argc)
		{
			optarg = argv[optind];
			optind++;
		}
		else
		{
			return _T('?');
		}
	}

	return c;
}

#endif
#endif


