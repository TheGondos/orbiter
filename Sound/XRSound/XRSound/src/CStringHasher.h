// ==============================================================
// CString hasher class
//
// Copyright (c) 2018-2021 Douglas Beachy
// Licensed under the MIT License
// ==============================================================

#pragma once

#include <unordered_map>

//=========================================================================
// The following class defines a hash function for string objects.
// Similar to algorithm in http://stackoverflow.com/a/15811185/2347831
//=========================================================================
class CStringHasher
{
public:
    // Returns hashcode for the supplied std::string
    size_t operator() (const std::string &csKey) const
    {
        size_t hash = 0;
        for (int i = 0; i < csKey.length(); i++)
            hash += (71 * hash + csKey[i]) % 5;

        return hash;
    }

    // Compares two std::string objects for equality; returns true if strings match
    bool operator() (const std::string &s1, const std::string &s2) const
    {
        return (s1 == s2);
    }
};
