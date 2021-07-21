#pragma once
#ifndef ASSERT_H
#define ASSERT_H

#ifdef __cplusplus
extern "C"
{
#endif

int		crash(const char *file, const char *function, int line, const char *expr, const char *msg, ...);

#ifdef __cplusplus
}
#endif

#ifdef RELEASE
#define assert(EXPR)
#else
#define assert(EXPR)			((EXPR)!=0||crash(__FILE__, __FUNCTION__, __LINE__, #EXPR, nullptr))
#define assert2(EXPR, MSG, ...)	((EXPR)!=0||crash(__FILE__, __FUNCTION__, __LINE__, #EXPR, MSG,##__VA_ARGS__))
#endif

#endif//ASSERT_H