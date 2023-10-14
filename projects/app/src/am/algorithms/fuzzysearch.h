#pragma once

#include "am/types.h"

inline int LevenshteinDistance(const char* source, const char* target)
{
	size_t sourceLen = strlen(source);
	size_t targetLen = strlen(target);

	if (sourceLen > targetLen) {
		return LevenshteinDistance(target, source);
	}

	const size_t min_size = sourceLen, max_size = targetLen;
	rageam::SmallList<char> lev_dist(min_size + 1);

	for (size_t i = 0; i <= min_size; ++i)
	{
		lev_dist[i] = i;
	}

	for (size_t j = 1; j <= max_size; ++j)
	{
		char previous_diagonal = lev_dist[0], previous_diagonal_save;
		++lev_dist[0];

		for (size_t i = 1; i <= min_size; ++i) 
		{
			previous_diagonal_save = lev_dist[i];
			if (source[i - 1] == target[j - 1]) 
			{
				lev_dist[i] = previous_diagonal;
			}
			else 
			{
				lev_dist[i] = std::min(std::min(lev_dist[i - 1], lev_dist[i]), previous_diagonal) + 1;
			}
			previous_diagonal = previous_diagonal_save;
		}
	}

	return lev_dist[min_size];
}
