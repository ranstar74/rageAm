//
// File: glob.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

// https://en.wikipedia.org/wiki/Glob_(programming)
// https://github.com/Robert-van-Engelen/FastGlobbing

// Most basic implementation that supports only '*' and '?'
template<typename T>
constexpr bool GlobMatch(const T* text, const T* glob)
{
    const T* text_backup = NULL;
    const T* glob_backup = NULL;
    while (*text != 0)
    {
        if (*glob == '*')
        {
            // new star-loop: backup positions in pattern and text
            text_backup = text;
            glob_backup = ++glob;
        }
        else if ((*glob == '?' && *text != '/') || *glob == *text)
        {
            // ? matched any character except /, or we matched the current non-NUL character
            ++text;
            ++glob;
        }
        else
        {
            if (glob_backup == NULL || *text_backup == '/')
                return FALSE;
            // star-loop: backtrack to the last * but do not jump over /
            text = ++text_backup;
            glob = glob_backup;
        }
    }
    // ignore trailing stars
    while (*glob == '*')
        ++glob;
    // at end of text means success if nothing else is left to match
    return *glob == 0 ? TRUE : FALSE;
}
static_assert(GlobMatch("prop_chair.ydr", "*?hair*"));
static_assert(GlobMatch("prop_chair.ydr", "*?rop*"));
static_assert(GlobMatch("prop_chair.ydr", "*.ydr"));
static_assert(GlobMatch("prop_chair.ydr", "*.*"));
static_assert(GlobMatch("prop_chair.ydr", "*"));
static_assert(!GlobMatch("prop_chair.ydr", "*??rop*"));
