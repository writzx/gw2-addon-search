#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#ifdef FINDER_EXPORTS
    #define FINDER_API __declspec(dllexport)
#else
    #define FINDER_API __declspec(dllimport)
#endif

extern HMODULE ADDON_MODULE;
