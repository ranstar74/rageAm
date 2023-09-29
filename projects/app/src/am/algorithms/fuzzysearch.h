#pragma once

#include <cctype>
#include <malloc.h>
#include <cstring>

#include "common/types.h"

// Levenshtein distance allows to find similar strings, mainly used in search engines.
// - Max distance specifies how much different strings can be, for example
// 'Adder' and 'Ader' - 1; one letter missing (d)
// 'Adder' and 'Aderr' - 2; one letter missing (d), one new letter (r)
// - Min length specifies minimum required length to use fuzzy search, otherwise simple comparison is done
// This is useful in cases when we want user to specify few first letter right
// For example Threshold will be valid with 'T' but not 'A', although 'Ahresh' will be valid
// - Ignore length can be used as 'StartsWith'
inline bool FuzzyCompare(ConstString lhs, ConstString rhs, int maxDistance = 2, int fuzzyMinLength = 3, bool ignoreCase = true, bool ignoreLength = true)
{
	// Don't try to understand this mess, this is just example code copied from wikipedia
#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))
	unsigned int s1len, s2len, x, y, lastdiag, olddiag;
	s1len = (int)strlen(lhs);
	s2len = (int)strlen(rhs);

	unsigned int minLength = s1len < s2len ? s1len : s2len;
	if ((int)minLength < fuzzyMinLength)
	{
		if (ignoreCase)
			return _strnicmp(lhs, rhs, minLength) == 0;
		else
			return strncmp(lhs, rhs, minLength) == 0;
	}

	if (ignoreLength)
	{
		s1len = minLength;
		s2len = minLength;
	}

	unsigned int* column = (unsigned int*)alloca((s1len + 1) * sizeof(unsigned int));
	for (y = 1; y <= s1len; y++)
		column[y] = y;
	for (x = 1; x <= s2len; x++)
	{
		column[0] = x;
		for (y = 1, lastdiag = x - 1; y <= s1len; y++)
		{
			char cl = lhs[y - 1];
			char cr = rhs[x - 1];

			if (ignoreCase) cl = (char)tolower(cl);
			if (ignoreCase) cr = (char)tolower(cr);

			olddiag = column[y];
			column[y] = MIN3(
				column[y] + 1, column[y - 1] + 1, lastdiag + (cl == cr ? 0 : 1));
			lastdiag = olddiag;
		}
	}
	return (int)column[s1len] <= maxDistance;
#undef MIN3
}
