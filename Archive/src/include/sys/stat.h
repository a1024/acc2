#pragma once
#ifndef SYS_STAT_H
#define SYS_STAT_H

#if !defined __linux__
//#if _MSC_VER<1800
#define	S_IFMT		00170000//octal
#define	S_IFREG		 0100000
//#endif
#define	S_ISREG(m)	(((m)&S_IFMT)==S_IFREG)
#endif

//typedef unsigned _dev_t;
//typedef unsigned short _ino_t;
//typedef long _off_t;
//typedef long long time_t;
struct stat
{
	unsigned st_dev;
	unsigned short st_ino;
	unsigned short st_mode;
	short st_nlink;
	short st_uid;
	short st_gid;
	unsigned st_rdev;
	long st_size;
	long long st_atime;
	long long st_mtime;
	long long st_ctime;
};
struct _stat64i32
{
	unsigned st_dev;
	unsigned short st_ino;
	unsigned short st_mode;
	short st_nlink;
	short st_uid;
	short st_gid;
	unsigned st_rdev;
	long st_size;
	long long st_atime;
	long long st_mtime;
	long long st_ctime;
};

#ifdef __cplusplus
extern "C"
{
#endif

int __cdecl _stat64i32(const char *filename, struct _stat64i32 *info);

#ifdef __cplusplus
}
#endif

#define STATIC_ASSERT(EXPR) typedef char __static_assert_t[EXPR]
inline int __cdecl stat(const char *filename, struct stat *info)
{
    STATIC_ASSERT(sizeof(struct stat)==sizeof(struct _stat64i32));
    return _stat64i32(filename, (struct _stat64i32*)info);
}

#endif