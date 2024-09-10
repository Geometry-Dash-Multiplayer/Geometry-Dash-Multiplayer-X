#pragma once
#include <cassert>

#ifndef __INTELLISENSE__
  #define assertm(expr, msg) assert(((void)msg, expr))
#else
  #define assertm(expr, msg) assert(expr)
#endif
