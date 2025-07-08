/**
 * @author Bénédikt Tran
 * @seealso @file Tools/cpuinfo/libcpuinfo/features/xsave.py
 *
 * XSAVE state components (XCR0 control register).
 *
 * See https://en.wikipedia.org/wiki/Control_register#XCR0_and_XSS.
 */

#ifndef Py_INTERNAL_CPUINFO_XSAVE_FEATURES_H
#define Py_INTERNAL_CPUINFO_XSAVE_FEATURES_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "Python.h"

/*[python input]
import os, sys
sys.path.insert(0, os.path.realpath(os.path.join(os.getcwd(), "Tools/cpuinfo")))
from libcpuinfo.features.xsave import make_xsave_features_constants
print(make_xsave_features_constants())
[python start generated code]*/
// clang-format off
/** Constants for XSAVE components */
#define _Py_XSAVE_MASK_XCR0_SSE                 0x00000002  // bit = 1
#define _Py_XSAVE_MASK_XCR0_AVX                 0x00000004  // bit = 2
#define _Py_XSAVE_MASK_XCR0_AVX512_OPMASK       0x00000020  // bit = 5
#define _Py_XSAVE_MASK_XCR0_AVX512_ZMM_HI256    0x00000040  // bit = 6
#define _Py_XSAVE_MASK_XCR0_AVX512_HI16_ZMM     0x00000080  // bit = 7
// clang-format on
/*[python end generated code: output=ac059b802b4317cb input=0a1b0774d3271477]*/

#ifdef __cplusplus
}
#endif

#endif // !Py_INTERNAL_CPUINFO_XSAVE_FEATURES_H
