/*
 *
 *  Copyright (C) 1997-2025, OFFIS e.V.
 *  All rights reserved.  See COPYRIGHT file for details.
 *
 *  This software and supporting documentation were developed by
 *
 *    OFFIS e.V.
 *    R&D Division Health
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *
 *  Module:  ofstd
 *
 *  Author:  Andreas Barth
 *
 *  Purpose:
 *      Defines some C++ standard types that are not consistently
 *      supported by all C++ Compilers
 *
 */

#ifndef OFTYPES_H
#define OFTYPES_H

#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */
#include "dcmtk/ofstd/ofdefine.h"

// some C++ implementations only define printf format macros like PRIu32
// when this macro is defined before <inttypes.h> or <cinttypes> are included.
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

// include this file in doxygen documentation

/** @file oftypes.h
 *  @brief Definition of standard types used throughout the toolkit
 */

#include "dcmtk/ofstd/ofstdinc.h"

// use native types if C++11 is supported
#ifdef HAVE_CXX11
#include <cstdint>
#include <cstddef>
#include <ostream>
#include <cinttypes>

#define OFTypename typename
#define OFlonglong long long
#define OFulonglong unsigned long long

using Sint8 = std::int8_t;
using Uint8 = std::uint8_t;
using Sint16 = std::int16_t;
using Uint16 = std::uint16_t;
using Sint32 = std::int32_t;
using Uint32 = std::uint32_t;
using Sint64 = std::int64_t;
using Uint64 = std::uint64_t;
using Float32 = float; // single precision floating point type. Usually IEEE-754 32 bit floating point type
using Float64 = double; // double precision floating point type. Usually IEEE-754 64 bit floating point type
using OFintptr_t = std::intptr_t;
using OFuintptr_t = std::uintptr_t;
using OFnullptr_t = std::nullptr_t;
using OFBool = bool;
constexpr OFnullptr_t OFnullptr = nullptr;
constexpr const OFBool OFTrue = true;
constexpr const OFBool OFFalse = false;

inline std::ostream& operator<<( std::ostream& o, OFnullptr_t /* unused */ )
{
    return o << "nullptr";
}

#else // fallback definitions

#include <cstddef>
BEGIN_EXTERN_C
#include <stdint.h>
#include <inttypes.h>
END_EXTERN_C

#include "dcmtk/ofstd/ofstream.h"

typedef signed char     Sint8;
typedef unsigned char   Uint8;

#ifdef PRIu32 /* if that macro exists, we also have int32_t et al */
typedef int32_t         Sint32;
typedef uint32_t        Uint32;
#else /* defined(PRIu32) */
#if SIZEOF_LONG == 8
typedef signed int      Sint32;
typedef unsigned int    Uint32;
#else /* SIZEOF_LONG == 8 */
typedef signed long     Sint32;
typedef unsigned long   Uint32;
#endif /* SIZEOF_LONG == 8 */
#endif /* defined(PRIu32) */

typedef signed short    Sint16;
typedef unsigned short  Uint16;

typedef float           Float32;    /* 32 Bit Floating Point Single */
typedef double          Float64;    /* 64 Bit Floating Point Double */

#ifdef HAVE_LONG_LONG
#define OFlonglong long long
#elif defined(_WIN32)
#define OFlonglong __int64
#endif

#ifdef HAVE_UNSIGNED_LONG_LONG
#define OFulonglong unsigned long long
#elif defined(_WIN32)
#define OFulonglong unsigned __int64
#endif

#ifdef HAVE_INT64_T
/* many platforms define int64_t in <cstdint> */
typedef int64_t       Sint64;
#elif SIZEOF_LONG == 8
/* on some platforms, long is 64 bits */
typedef long          Sint64;
#elif defined(OFlonglong)
/* assume OFlonglong is 64 bits */
typedef OFlonglong    Sint64;
#else
/* we have not found any 64-bit signed integer type */
#error unsupported platform (no 64-bit signed integer type)
#endif

#ifdef HAVE_UINT64_T
/* many platforms define uint64_t in <cstdint> */
typedef uint64_t      Uint64;
#elif SIZEOF_LONG == 8
/* on some platforms, long is 64 bits */
typedef unsigned long Uint64;
#elif defined(OFulonglong)
/* assume OFulonglong is 64 bits */
typedef OFulonglong   Uint64;
#else
/* we have not found any 64-bit unsigned integer type */
#error unsupported platform (no 64-bit unsigned integer type)
#endif

#if SIZEOF_VOID_P == 2
typedef Sint16 OFintptr_t;
typedef Uint16 OFuintptr_t;
#elif SIZEOF_VOID_P == 4
typedef Sint32 OFintptr_t;
typedef Uint32 OFuintptr_t;
#elif SIZEOF_VOID_P == 8
typedef Sint64 OFintptr_t;
typedef Uint64 OFuintptr_t;
#else
#error Unsupported platform (invalid pointer size)
#endif

// Definition of type OFBool

typedef bool OFBool;
#define OFTrue true
#define OFFalse false

#define OFTypename typename

#ifndef DOXYGEN
struct DCMTK_OFSTD_EXPORT OFnullptr_t
{
    friend STD_NAMESPACE ostream& operator<<( STD_NAMESPACE ostream& o, OFnullptr_t /* unused */ )
    {
        return o << "nullptr";
    }

    template<typename T>
    operator T*()
    {
        return NULL;
    }

    template<typename C,typename T>
    operator T C::*()
    {
        return NULL;
    }
};

DCMTK_OFSTD_EXPORT extern OFnullptr_t OFnullptr;
#else // DOXYGEN
/// OFnullptr_t is the type of the null pointer literal, OFnullptr.
typedef unspecified OFnullptr_t;
#endif // DOXYGEN

#endif // C++11

#endif // OFTYPES_H
