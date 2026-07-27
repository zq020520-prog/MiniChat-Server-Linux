#pragma once

#include <cstring>


template<size_t N>
int strcpy_s(char(&dest)[N], const char* src)
{
    if (src == nullptr)
    {
        dest[0] = '\0';
        return 1;
    }


    std::strncpy(
        dest,
        src,
        N - 1);


    dest[N - 1] = '\0';


    return 0;
}


template<size_t N>
int strcpy_s(
    char(&dest)[N],
    size_t size,
    const char* src)
{
    if (src == nullptr)
    {
        dest[0] = '\0';
        return 1;
    }


    std::strncpy(
        dest,
        src,
        size - 1);


    dest[size - 1] = '\0';


    return 0;
}