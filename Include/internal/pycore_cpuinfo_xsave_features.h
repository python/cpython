/**
 * @author Bénédikt Tran
 * @seealso @file Tools/cpuinfo/xsave_features_gen.py
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

// fmt: off
/*[python input]
import os, sys
sys.path.insert(0, os.path.realpath(os.path.join(os.getcwd(), "Tools")))
from cpuinfo.xsave_features_gen import generate_xsave_features_enum
print(generate_xsave_features_enum("_Py_xsave_feature_mask"))
[python start generated code]*/
// fmt: off
/** Enumeration for XSAVE components */
enum _Py_xsave_feature_mask_e {
    _Py_XSAVE_MASK_XCR0_SSE                 = 0x00000002,    // bit = 1
    _Py_XSAVE_MASK_XCR0_AVX                 = 0x00000004,    // bit = 2
    _Py_XSAVE_MASK_XCR0_AVX512_OPMASK       = 0x00000020,    // bit = 5
    _Py_XSAVE_MASK_XCR0_AVX512_ZMM_HI256    = 0x00000040,    // bit = 6
    _Py_XSAVE_MASK_XCR0_AVX512_HI16_ZMM     = 0x00000080,    // bit = 7
};
// fmt: on
/*[python end generated code: output=35ea9a165938f8ef input=336793a305515376]*/

#endif // !Py_INTERNAL_CPUINFO_XSAVE_FEATURES_H
