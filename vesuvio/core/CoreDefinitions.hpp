#pragma once


#if defined(_WIN32)
#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)
#else 
#define EXPORT
#define IMPORT
#endif