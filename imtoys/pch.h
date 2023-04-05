
#ifndef PCH_H
#define PCH_H

/* for vsprintf_s */
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif
// for min max dup define error when include windows.h
#define NOMINMAX

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>
#include <memory>
#include <functional>
#include <vector>
#include <tuple>
#include <map>

#endif