/*
 * @author Bénédikt Tran
 * @seealso Tools/cpuinfo/xsave_features_gen.py
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
import importlib
import os
import sys

ROOT = os.getcwd()
TOOL = os.path.join(ROOT, "Tools/cpuinfo/xsave_features_gen.py")
TOOL = os.path.realpath(TOOL)

if not os.path.exists(TOOL):
    raise FileNotFoundError(TOOL)

sys.path.insert(0, os.path.dirname(os.path.dirname(TOOL)))
module = importlib.import_module("cpuinfo.xsave_features_gen")
print(module.generate_xsave_features_enum("py_xsave_feature_mask"))
[python start generated code]*/
typedef enum py_xsave_feature_mask {
    Py_XSAVE_MASK_XCR0_SSE              = 0x00000002,    // bit = 1
    Py_XSAVE_MASK_XCR0_AVX              = 0x00000004,    // bit = 2
    Py_XSAVE_MASK_XCR0_AVX512_OPMASK    = 0x00000020,    // bit = 5
    Py_XSAVE_MASK_XCR0_AVX512_ZMM_HI256 = 0x00000040,    // bit = 6
    Py_XSAVE_MASK_XCR0_AVX512_HI16_ZMM  = 0x00000080,    // bit = 7
} py_xsave_feature_mask;
/*[python end generated code: output=9a476ed0abbc617b input=41f35058299c0118]*/
// fmt: on

#endif // !Py_INTERNAL_CPUINFO_XSAVE_FEATURES_H
