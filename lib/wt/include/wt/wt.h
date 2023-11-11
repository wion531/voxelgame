#ifndef WT_H
#define WT_H

#if defined(__clang__)
#  define WT_COMPILER_CLANG 1
#  if defined(_WIN32)
#    define WT_SYS_WIN32 1
#  elif defined(__gnu_linux__)
#    define WT_SYS_LINUX 1
#  elif defined(__APPLE__) && defined(__MACH__)
#    define WT_SYS_MACOS 1
#  else
#    error missing OS detection
#  endif
#  if defined(__amd64__)
#    define WT_ARCH_X64 1
#  elif defined(__i386__)
#    define WT_ARCH_X86 1
#  elif defined(__arm__)
#    define WT_ARCH_ARM 1
#  elif defined(__aarch64__)
#    define WT_ARCH_ARM64 1
#  else
#    error missing ARCH detection
#  endif
#elif defined(_MSC_VER)
#  define WT_COMPILER_MSVC 1
#  if defined(_WIN32)
#    define WT_SYS_WIN32 1
#  else
#    error missing OS detection
#  endif
#  if defined(_M_AMD64)
#    define WT_ARCH_X64 1
#  elif defined(_M_I86)
#    define WT_ARCH_X86 1
#  elif defined(_M_ARM)
#    define WT_ARCH_ARM 1
#  else
#    error missing ARCH detection
#  endif
#elif defined(__GNUC__)
#  define WT_COMPILER_GCC 1
#  if defined(_WIN32)
#    define WT_SYS_WIN32 1
#  elif defined(__gnu_linux__)
#    define WT_SYS_LINUX 1
#  elif defined(__APPLE__) && defined(__MACH__)
#    define WT_SYS_MACOS 1
#  else
#    error missing OS detection
#  endif
#  if defined(__amd64__)
#    define WT_ARCH_X64 1
#  elif defined(__i386__)
#    define WT_ARCH_X86 1
#  elif defined(__arm__)
#    define WT_ARCH_ARM 1
#  elif defined(__aarch64__)
#    define WT_ARCH_ARM64 1
#  else
#    error missing ARCH detection
#  endif
#elif defined(__TINYC__)
#  define WT_COMPILER_TCC 1
#  if defined(_WIN32)
#    define WT_SYS_WIN32 1
#  elif defined(__linux__)
#    define WT_SYS_LINUX 1
#  elif defined(__APPLE__) && defined(__MACH__)
#    define WT_SYS_MACOS 1
#  else
#    error missing OS detection
#  endif
#  define WT_ARCH_X64 1
#else
#  error no context cracking for this compiler
#endif

#if !defined(WT_COMPILER_MSVC)
#  define WT_COMPILER_MSVC 0
#endif
#if !defined(WT_COMPILER_CLANG)
#  define WT_COMPILER_CLANG 0
#endif
#if !defined(WT_COMPILER_GCC)
#  define WT_COMPILER_GCC 0
#endif
#if !defined(WT_SYS_WIN32)
#  define WT_SYS_WIN32 0
#endif
#if !defined(WT_SYS_LINUX)
#  define WT_SYS_LINUX 0
#endif
#if !defined(WT_SYS_MACOS)
#  define WT_SYS_MACOS 0
#endif
#if !defined(WT_ARCH_X64)
#  define WT_ARCH_X64 0
#endif
#if !defined(WT_ARCH_X86)
#  define WT_ARCH_X86 0
#endif
#if !defined(WT_ARCH_ARM)
#  define WT_ARCH_ARM 0
#endif
#if !defined(WT_ARCH_ARM64)
#  define WT_ARCH_ARM64 0
#endif

#include "macros.h"
#include "types.h"
#include "allocators.h"
#include "containers.h"
#include "hash.h"

#endif
