/* MIT License
 *
 * Copyright (c) 2016-2022 INRIA, CMU and Microsoft Corporation
 * Copyright (c) 2022-2023 HACL* Contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include "internal/Hacl_Hash_MD5.h"

#include "Hacl_Streaming_Types.h"
#include "internal/Hacl_Streaming_Types.h"

static uint32_t _h0[4U] = { 0x67452301U, 0xefcdab89U, 0x98badcfeU, 0x10325476U };

static uint32_t
_t[64U] =
  {
    0xd76aa478U, 0xe8c7b756U, 0x242070dbU, 0xc1bdceeeU, 0xf57c0fafU, 0x4787c62aU, 0xa8304613U,
    0xfd469501U, 0x698098d8U, 0x8b44f7afU, 0xffff5bb1U, 0x895cd7beU, 0x6b901122U, 0xfd987193U,
    0xa679438eU, 0x49b40821U, 0xf61e2562U, 0xc040b340U, 0x265e5a51U, 0xe9b6c7aaU, 0xd62f105dU,
    0x02441453U, 0xd8a1e681U, 0xe7d3fbc8U, 0x21e1cde6U, 0xc33707d6U, 0xf4d50d87U, 0x455a14edU,
    0xa9e3e905U, 0xfcefa3f8U, 0x676f02d9U, 0x8d2a4c8aU, 0xfffa3942U, 0x8771f681U, 0x6d9d6122U,
    0xfde5380cU, 0xa4beea44U, 0x4bdecfa9U, 0xf6bb4b60U, 0xbebfbc70U, 0x289b7ec6U, 0xeaa127faU,
    0xd4ef3085U, 0x4881d05U, 0xd9d4d039U, 0xe6db99e5U, 0x1fa27cf8U, 0xc4ac5665U, 0xf4292244U,
    0x432aff97U, 0xab9423a7U, 0xfc93a039U, 0x655b59c3U, 0x8f0ccc92U, 0xffeff47dU, 0x85845dd1U,
    0x6fa87e4fU, 0xfe2ce6e0U, 0xa3014314U, 0x4e0811a1U, 0xf7537e82U, 0xbd3af235U, 0x2ad7d2bbU,
    0xeb86d391U
  };

void Hacl_Hash_MD5_init(uint32_t *s)
{
  KRML_MAYBE_FOR4(i, 0U, 4U, 1U, s[i] = _h0[i];);
}

static void update(uint32_t *abcd, uint8_t *x)
{
  uint32_t aa = abcd[0U];
  uint32_t bb = abcd[1U];
  uint32_t cc = abcd[2U];
  uint32_t dd = abcd[3U];
  uint32_t va = abcd[0U];
  uint32_t vb0 = abcd[1U];
  uint32_t vc0 = abcd[2U];
  uint32_t vd0 = abcd[3U];
  uint8_t *b0 = x;
  uint32_t u = load32_le(b0);
  uint32_t xk = u;
  uint32_t ti0 = _t[0U];
  uint32_t
  v =
    vb0 +
      ((va + ((vb0 & vc0) | (~vb0 & vd0)) + xk + ti0) << 7U |
        (va + ((vb0 & vc0) | (~vb0 & vd0)) + xk + ti0) >> 25U);
  abcd[0U] = v;
  uint32_t va0 = abcd[3U];
  uint32_t vb1 = abcd[0U];
  uint32_t vc1 = abcd[1U];
  uint32_t vd1 = abcd[2U];
  uint8_t *b1 = x + 4U;
  uint32_t u0 = load32_le(b1);
  uint32_t xk0 = u0;
  uint32_t ti1 = _t[1U];
  uint32_t
  v0 =
    vb1 +
      ((va0 + ((vb1 & vc1) | (~vb1 & vd1)) + xk0 + ti1) << 12U |
        (va0 + ((vb1 & vc1) | (~vb1 & vd1)) + xk0 + ti1) >> 20U);
  abcd[3U] = v0;
  uint32_t va1 = abcd[2U];
  uint32_t vb2 = abcd[3U];
  uint32_t vc2 = abcd[0U];
  uint32_t vd2 = abcd[1U];
  uint8_t *b2 = x + 8U;
  uint32_t u1 = load32_le(b2);
  uint32_t xk1 = u1;
  uint32_t ti2 = _t[2U];
  uint32_t
  v1 =
    vb2 +
      ((va1 + ((vb2 & vc2) | (~vb2 & vd2)) + xk1 + ti2) << 17U |
        (va1 + ((vb2 & vc2) | (~vb2 & vd2)) + xk1 + ti2) >> 15U);
  abcd[2U] = v1;
  uint32_t va2 = abcd[1U];
  uint32_t vb3 = abcd[2U];
  uint32_t vc3 = abcd[3U];
  uint32_t vd3 = abcd[0U];
  uint8_t *b3 = x + 12U;
  uint32_t u2 = load32_le(b3);
  uint32_t xk2 = u2;
  uint32_t ti3 = _t[3U];
  uint32_t
  v2 =
    vb3 +
      ((va2 + ((vb3 & vc3) | (~vb3 & vd3)) + xk2 + ti3) << 22U |
        (va2 + ((vb3 & vc3) | (~vb3 & vd3)) + xk2 + ti3) >> 10U);
  abcd[1U] = v2;
  uint32_t va3 = abcd[0U];
  uint32_t vb4 = abcd[1U];
  uint32_t vc4 = abcd[2U];
  uint32_t vd4 = abcd[3U];
  uint8_t *b4 = x + 16U;
  uint32_t u3 = load32_le(b4);
  uint32_t xk3 = u3;
  uint32_t ti4 = _t[4U];
  uint32_t
  v3 =
    vb4 +
      ((va3 + ((vb4 & vc4) | (~vb4 & vd4)) + xk3 + ti4) << 7U |
        (va3 + ((vb4 & vc4) | (~vb4 & vd4)) + xk3 + ti4) >> 25U);
  abcd[0U] = v3;
  uint32_t va4 = abcd[3U];
  uint32_t vb5 = abcd[0U];
  uint32_t vc5 = abcd[1U];
  uint32_t vd5 = abcd[2U];
  uint8_t *b5 = x + 20U;
  uint32_t u4 = load32_le(b5);
  uint32_t xk4 = u4;
  uint32_t ti5 = _t[5U];
  uint32_t
  v4 =
    vb5 +
      ((va4 + ((vb5 & vc5) | (~vb5 & vd5)) + xk4 + ti5) << 12U |
        (va4 + ((vb5 & vc5) | (~vb5 & vd5)) + xk4 + ti5) >> 20U);
  abcd[3U] = v4;
  uint32_t va5 = abcd[2U];
  uint32_t vb6 = abcd[3U];
  uint32_t vc6 = abcd[0U];
  uint32_t vd6 = abcd[1U];
  uint8_t *b6 = x + 24U;
  uint32_t u5 = load32_le(b6);
  uint32_t xk5 = u5;
  uint32_t ti6 = _t[6U];
  uint32_t
  v5 =
    vb6 +
      ((va5 + ((vb6 & vc6) | (~vb6 & vd6)) + xk5 + ti6) << 17U |
        (va5 + ((vb6 & vc6) | (~vb6 & vd6)) + xk5 + ti6) >> 15U);
  abcd[2U] = v5;
  uint32_t va6 = abcd[1U];
  uint32_t vb7 = abcd[2U];
  uint32_t vc7 = abcd[3U];
  uint32_t vd7 = abcd[0U];
  uint8_t *b7 = x + 28U;
  uint32_t u6 = load32_le(b7);
  uint32_t xk6 = u6;
  uint32_t ti7 = _t[7U];
  uint32_t
  v6 =
    vb7 +
      ((va6 + ((vb7 & vc7) | (~vb7 & vd7)) + xk6 + ti7) << 22U |
        (va6 + ((vb7 & vc7) | (~vb7 & vd7)) + xk6 + ti7) >> 10U);
  abcd[1U] = v6;
  uint32_t va7 = abcd[0U];
  uint32_t vb8 = abcd[1U];
  uint32_t vc8 = abcd[2U];
  uint32_t vd8 = abcd[3U];
  uint8_t *b8 = x + 32U;
  uint32_t u7 = load32_le(b8);
  uint32_t xk7 = u7;
  uint32_t ti8 = _t[8U];
  uint32_t
  v7 =
    vb8 +
      ((va7 + ((vb8 & vc8) | (~vb8 & vd8)) + xk7 + ti8) << 7U |
        (va7 + ((vb8 & vc8) | (~vb8 & vd8)) + xk7 + ti8) >> 25U);
  abcd[0U] = v7;
  uint32_t va8 = abcd[3U];
  uint32_t vb9 = abcd[0U];
  uint32_t vc9 = abcd[1U];
  uint32_t vd9 = abcd[2U];
  uint8_t *b9 = x + 36U;
  uint32_t u8 = load32_le(b9);
  uint32_t xk8 = u8;
  uint32_t ti9 = _t[9U];
  uint32_t
  v8 =
    vb9 +
      ((va8 + ((vb9 & vc9) | (~vb9 & vd9)) + xk8 + ti9) << 12U |
        (va8 + ((vb9 & vc9) | (~vb9 & vd9)) + xk8 + ti9) >> 20U);
  abcd[3U] = v8;
  uint32_t va9 = abcd[2U];
  uint32_t vb10 = abcd[3U];
  uint32_t vc10 = abcd[0U];
  uint32_t vd10 = abcd[1U];
  uint8_t *b10 = x + 40U;
  uint32_t u9 = load32_le(b10);
  uint32_t xk9 = u9;
  uint32_t ti10 = _t[10U];
  uint32_t
  v9 =
    vb10 +
      ((va9 + ((vb10 & vc10) | (~vb10 & vd10)) + xk9 + ti10) << 17U |
        (va9 + ((vb10 & vc10) | (~vb10 & vd10)) + xk9 + ti10) >> 15U);
  abcd[2U] = v9;
  uint32_t va10 = abcd[1U];
  uint32_t vb11 = abcd[2U];
  uint32_t vc11 = abcd[3U];
  uint32_t vd11 = abcd[0U];
  uint8_t *b11 = x + 44U;
  uint32_t u10 = load32_le(b11);
  uint32_t xk10 = u10;
  uint32_t ti11 = _t[11U];
  uint32_t
  v10 =
    vb11 +
      ((va10 + ((vb11 & vc11) | (~vb11 & vd11)) + xk10 + ti11) << 22U |
        (va10 + ((vb11 & vc11) | (~vb11 & vd11)) + xk10 + ti11) >> 10U);
  abcd[1U] = v10;
  uint32_t va11 = abcd[0U];
  uint32_t vb12 = abcd[1U];
  uint32_t vc12 = abcd[2U];
  uint32_t vd12 = abcd[3U];
  uint8_t *b12 = x + 48U;
  uint32_t u11 = load32_le(b12);
  uint32_t xk11 = u11;
  uint32_t ti12 = _t[12U];
  uint32_t
  v11 =
    vb12 +
      ((va11 + ((vb12 & vc12) | (~vb12 & vd12)) + xk11 + ti12) << 7U |
        (va11 + ((vb12 & vc12) | (~vb12 & vd12)) + xk11 + ti12) >> 25U);
  abcd[0U] = v11;
  uint32_t va12 = abcd[3U];
  uint32_t vb13 = abcd[0U];
  uint32_t vc13 = abcd[1U];
  uint32_t vd13 = abcd[2U];
  uint8_t *b13 = x + 52U;
  uint32_t u12 = load32_le(b13);
  uint32_t xk12 = u12;
  uint32_t ti13 = _t[13U];
  uint32_t
  v12 =
    vb13 +
      ((va12 + ((vb13 & vc13) | (~vb13 & vd13)) + xk12 + ti13) << 12U |
        (va12 + ((vb13 & vc13) | (~vb13 & vd13)) + xk12 + ti13) >> 20U);
  abcd[3U] = v12;
  uint32_t va13 = abcd[2U];
  uint32_t vb14 = abcd[3U];
  uint32_t vc14 = abcd[0U];
  uint32_t vd14 = abcd[1U];
  uint8_t *b14 = x + 56U;
  uint32_t u13 = load32_le(b14);
  uint32_t xk13 = u13;
  uint32_t ti14 = _t[14U];
  uint32_t
  v13 =
    vb14 +
      ((va13 + ((vb14 & vc14) | (~vb14 & vd14)) + xk13 + ti14) << 17U |
        (va13 + ((vb14 & vc14) | (~vb14 & vd14)) + xk13 + ti14) >> 15U);
  abcd[2U] = v13;
  uint32_t va14 = abcd[1U];
  uint32_t vb15 = abcd[2U];
  uint32_t vc15 = abcd[3U];
  uint32_t vd15 = abcd[0U];
  uint8_t *b15 = x + 60U;
  uint32_t u14 = load32_le(b15);
  uint32_t xk14 = u14;
  uint32_t ti15 = _t[15U];
  uint32_t
  v14 =
    vb15 +
      ((va14 + ((vb15 & vc15) | (~vb15 & vd15)) + xk14 + ti15) << 22U |
        (va14 + ((vb15 & vc15) | (~vb15 & vd15)) + xk14 + ti15) >> 10U);
  abcd[1U] = v14;
  uint32_t va15 = abcd[0U];
  uint32_t vb16 = abcd[1U];
  uint32_t vc16 = abcd[2U];
  uint32_t vd16 = abcd[3U];
  uint8_t *b16 = x + 4U;
  uint32_t u15 = load32_le(b16);
  uint32_t xk15 = u15;
  uint32_t ti16 = _t[16U];
  uint32_t
  v15 =
    vb16 +
      ((va15 + ((vb16 & vd16) | (vc16 & ~vd16)) + xk15 + ti16) << 5U |
        (va15 + ((vb16 & vd16) | (vc16 & ~vd16)) + xk15 + ti16) >> 27U);
  abcd[0U] = v15;
  uint32_t va16 = abcd[3U];
  uint32_t vb17 = abcd[0U];
  uint32_t vc17 = abcd[1U];
  uint32_t vd17 = abcd[2U];
  uint8_t *b17 = x + 24U;
  uint32_t u16 = load32_le(b17);
  uint32_t xk16 = u16;
  uint32_t ti17 = _t[17U];
  uint32_t
  v16 =
    vb17 +
      ((va16 + ((vb17 & vd17) | (vc17 & ~vd17)) + xk16 + ti17) << 9U |
        (va16 + ((vb17 & vd17) | (vc17 & ~vd17)) + xk16 + ti17) >> 23U);
  abcd[3U] = v16;
  uint32_t va17 = abcd[2U];
  uint32_t vb18 = abcd[3U];
  uint32_t vc18 = abcd[0U];
  uint32_t vd18 = abcd[1U];
  uint8_t *b18 = x + 44U;
  uint32_t u17 = load32_le(b18);
  uint32_t xk17 = u17;
  uint32_t ti18 = _t[18U];
  uint32_t
  v17 =
    vb18 +
      ((va17 + ((vb18 & vd18) | (vc18 & ~vd18)) + xk17 + ti18) << 14U |
        (va17 + ((vb18 & vd18) | (vc18 & ~vd18)) + xk17 + ti18) >> 18U);
  abcd[2U] = v17;
  uint32_t va18 = abcd[1U];
  uint32_t vb19 = abcd[2U];
  uint32_t vc19 = abcd[3U];
  uint32_t vd19 = abcd[0U];
  uint8_t *b19 = x;
  uint32_t u18 = load32_le(b19);
  uint32_t xk18 = u18;
  uint32_t ti19 = _t[19U];
  uint32_t
  v18 =
    vb19 +
      ((va18 + ((vb19 & vd19) | (vc19 & ~vd19)) + xk18 + ti19) << 20U |
        (va18 + ((vb19 & vd19) | (vc19 & ~vd19)) + xk18 + ti19) >> 12U);
  abcd[1U] = v18;
  uint32_t va19 = abcd[0U];
  uint32_t vb20 = abcd[1U];
  uint32_t vc20 = abcd[2U];
  uint32_t vd20 = abcd[3U];
  uint8_t *b20 = x + 20U;
  uint32_t u19 = load32_le(b20);
  uint32_t xk19 = u19;
  uint32_t ti20 = _t[20U];
  uint32_t
  v19 =
    vb20 +
      ((va19 + ((vb20 & vd20) | (vc20 & ~vd20)) + xk19 + ti20) << 5U |
        (va19 + ((vb20 & vd20) | (vc20 & ~vd20)) + xk19 + ti20) >> 27U);
  abcd[0U] = v19;
  uint32_t va20 = abcd[3U];
  uint32_t vb21 = abcd[0U];
  uint32_t vc21 = abcd[1U];
  uint32_t vd21 = abcd[2U];
  uint8_t *b21 = x + 40U;
  uint32_t u20 = load32_le(b21);
  uint32_t xk20 = u20;
  uint32_t ti21 = _t[21U];
  uint32_t
  v20 =
    vb21 +
      ((va20 + ((vb21 & vd21) | (vc21 & ~vd21)) + xk20 + ti21) << 9U |
        (va20 + ((vb21 & vd21) | (vc21 & ~vd21)) + xk20 + ti21) >> 23U);
  abcd[3U] = v20;
  uint32_t va21 = abcd[2U];
  uint32_t vb22 = abcd[3U];
  uint32_t vc22 = abcd[0U];
  uint32_t vd22 = abcd[1U];
  uint8_t *b22 = x + 60U;
  uint32_t u21 = load32_le(b22);
  uint32_t xk21 = u21;
  uint32_t ti22 = _t[22U];
  uint32_t
  v21 =
    vb22 +
      ((va21 + ((vb22 & vd22) | (vc22 & ~vd22)) + xk21 + ti22) << 14U |
        (va21 + ((vb22 & vd22) | (vc22 & ~vd22)) + xk21 + ti22) >> 18U);
  abcd[2U] = v21;
  uint32_t va22 = abcd[1U];
  uint32_t vb23 = abcd[2U];
  uint32_t vc23 = abcd[3U];
  uint32_t vd23 = abcd[0U];
  uint8_t *b23 = x + 16U;
  uint32_t u22 = load32_le(b23);
  uint32_t xk22 = u22;
  uint32_t ti23 = _t[23U];
  uint32_t
  v22 =
    vb23 +
      ((va22 + ((vb23 & vd23) | (vc23 & ~vd23)) + xk22 + ti23) << 20U |
        (va22 + ((vb23 & vd23) | (vc23 & ~vd23)) + xk22 + ti23) >> 12U);
  abcd[1U] = v22;
  uint32_t va23 = abcd[0U];
  uint32_t vb24 = abcd[1U];
  uint32_t vc24 = abcd[2U];
  uint32_t vd24 = abcd[3U];
  uint8_t *b24 = x + 36U;
  uint32_t u23 = load32_le(b24);
  uint32_t xk23 = u23;
  uint32_t ti24 = _t[24U];
  uint32_t
  v23 =
    vb24 +
      ((va23 + ((vb24 & vd24) | (vc24 & ~vd24)) + xk23 + ti24) << 5U |
        (va23 + ((vb24 & vd24) | (vc24 & ~vd24)) + xk23 + ti24) >> 27U);
  abcd[0U] = v23;
  uint32_t va24 = abcd[3U];
  uint32_t vb25 = abcd[0U];
  uint32_t vc25 = abcd[1U];
  uint32_t vd25 = abcd[2U];
  uint8_t *b25 = x + 56U;
  uint32_t u24 = load32_le(b25);
  uint32_t xk24 = u24;
  uint32_t ti25 = _t[25U];
  uint32_t
  v24 =
    vb25 +
      ((va24 + ((vb25 & vd25) | (vc25 & ~vd25)) + xk24 + ti25) << 9U |
        (va24 + ((vb25 & vd25) | (vc25 & ~vd25)) + xk24 + ti25) >> 23U);
  abcd[3U] = v24;
  uint32_t va25 = abcd[2U];
  uint32_t vb26 = abcd[3U];
  uint32_t vc26 = abcd[0U];
  uint32_t vd26 = abcd[1U];
  uint8_t *b26 = x + 12U;
  uint32_t u25 = load32_le(b26);
  uint32_t xk25 = u25;
  uint32_t ti26 = _t[26U];
  uint32_t
  v25 =
    vb26 +
      ((va25 + ((vb26 & vd26) | (vc26 & ~vd26)) + xk25 + ti26) << 14U |
        (va25 + ((vb26 & vd26) | (vc26 & ~vd26)) + xk25 + ti26) >> 18U);
  abcd[2U] = v25;
  uint32_t va26 = abcd[1U];
  uint32_t vb27 = abcd[2U];
  uint32_t vc27 = abcd[3U];
  uint32_t vd27 = abcd[0U];
  uint8_t *b27 = x + 32U;
  uint32_t u26 = load32_le(b27);
  uint32_t xk26 = u26;
  uint32_t ti27 = _t[27U];
  uint32_t
  v26 =
    vb27 +
      ((va26 + ((vb27 & vd27) | (vc27 & ~vd27)) + xk26 + ti27) << 20U |
        (va26 + ((vb27 & vd27) | (vc27 & ~vd27)) + xk26 + ti27) >> 12U);
  abcd[1U] = v26;
  uint32_t va27 = abcd[0U];
  uint32_t vb28 = abcd[1U];
  uint32_t vc28 = abcd[2U];
  uint32_t vd28 = abcd[3U];
  uint8_t *b28 = x + 52U;
  uint32_t u27 = load32_le(b28);
  uint32_t xk27 = u27;
  uint32_t ti28 = _t[28U];
  uint32_t
  v27 =
    vb28 +
      ((va27 + ((vb28 & vd28) | (vc28 & ~vd28)) + xk27 + ti28) << 5U |
        (va27 + ((vb28 & vd28) | (vc28 & ~vd28)) + xk27 + ti28) >> 27U);
  abcd[0U] = v27;
  uint32_t va28 = abcd[3U];
  uint32_t vb29 = abcd[0U];
  uint32_t vc29 = abcd[1U];
  uint32_t vd29 = abcd[2U];
  uint8_t *b29 = x + 8U;
  uint32_t u28 = load32_le(b29);
  uint32_t xk28 = u28;
  uint32_t ti29 = _t[29U];
  uint32_t
  v28 =
    vb29 +
      ((va28 + ((vb29 & vd29) | (vc29 & ~vd29)) + xk28 + ti29) << 9U |
        (va28 + ((vb29 & vd29) | (vc29 & ~vd29)) + xk28 + ti29) >> 23U);
  abcd[3U] = v28;
  uint32_t va29 = abcd[2U];
  uint32_t vb30 = abcd[3U];
  uint32_t vc30 = abcd[0U];
  uint32_t vd30 = abcd[1U];
  uint8_t *b30 = x + 28U;
  uint32_t u29 = load32_le(b30);
  uint32_t xk29 = u29;
  uint32_t ti30 = _t[30U];
  uint32_t
  v29 =
    vb30 +
      ((va29 + ((vb30 & vd30) | (vc30 & ~vd30)) + xk29 + ti30) << 14U |
        (va29 + ((vb30 & vd30) | (vc30 & ~vd30)) + xk29 + ti30) >> 18U);
  abcd[2U] = v29;
  uint32_t va30 = abcd[1U];
  uint32_t vb31 = abcd[2U];
  uint32_t vc31 = abcd[3U];
  uint32_t vd31 = abcd[0U];
  uint8_t *b31 = x + 48U;
  uint32_t u30 = load32_le(b31);
  uint32_t xk30 = u30;
  uint32_t ti31 = _t[31U];
  uint32_t
  v30 =
    vb31 +
      ((va30 + ((vb31 & vd31) | (vc31 & ~vd31)) + xk30 + ti31) << 20U |
        (va30 + ((vb31 & vd31) | (vc31 & ~vd31)) + xk30 + ti31) >> 12U);
  abcd[1U] = v30;
  uint32_t va31 = abcd[0U];
  uint32_t vb32 = abcd[1U];
  uint32_t vc32 = abcd[2U];
  uint32_t vd32 = abcd[3U];
  uint8_t *b32 = x + 20U;
  uint32_t u31 = load32_le(b32);
  uint32_t xk31 = u31;
  uint32_t ti32 = _t[32U];
  uint32_t
  v31 =
    vb32 +
      ((va31 + (vb32 ^ (vc32 ^ vd32)) + xk31 + ti32) << 4U |
        (va31 + (vb32 ^ (vc32 ^ vd32)) + xk31 + ti32) >> 28U);
  abcd[0U] = v31;
  uint32_t va32 = abcd[3U];
  uint32_t vb33 = abcd[0U];
  uint32_t vc33 = abcd[1U];
  uint32_t vd33 = abcd[2U];
  uint8_t *b33 = x + 32U;
  uint32_t u32 = load32_le(b33);
  uint32_t xk32 = u32;
  uint32_t ti33 = _t[33U];
  uint32_t
  v32 =
    vb33 +
      ((va32 + (vb33 ^ (vc33 ^ vd33)) + xk32 + ti33) << 11U |
        (va32 + (vb33 ^ (vc33 ^ vd33)) + xk32 + ti33) >> 21U);
  abcd[3U] = v32;
  uint32_t va33 = abcd[2U];
  uint32_t vb34 = abcd[3U];
  uint32_t vc34 = abcd[0U];
  uint32_t vd34 = abcd[1U];
  uint8_t *b34 = x + 44U;
  uint32_t u33 = load32_le(b34);
  uint32_t xk33 = u33;
  uint32_t ti34 = _t[34U];
  uint32_t
  v33 =
    vb34 +
      ((va33 + (vb34 ^ (vc34 ^ vd34)) + xk33 + ti34) << 16U |
        (va33 + (vb34 ^ (vc34 ^ vd34)) + xk33 + ti34) >> 16U);
  abcd[2U] = v33;
  uint32_t va34 = abcd[1U];
  uint32_t vb35 = abcd[2U];
  uint32_t vc35 = abcd[3U];
  uint32_t vd35 = abcd[0U];
  uint8_t *b35 = x + 56U;
  uint32_t u34 = load32_le(b35);
  uint32_t xk34 = u34;
  uint32_t ti35 = _t[35U];
  uint32_t
  v34 =
    vb35 +
      ((va34 + (vb35 ^ (vc35 ^ vd35)) + xk34 + ti35) << 23U |
        (va34 + (vb35 ^ (vc35 ^ vd35)) + xk34 + ti35) >> 9U);
  abcd[1U] = v34;
  uint32_t va35 = abcd[0U];
  uint32_t vb36 = abcd[1U];
  uint32_t vc36 = abcd[2U];
  uint32_t vd36 = abcd[3U];
  uint8_t *b36 = x + 4U;
  uint32_t u35 = load32_le(b36);
  uint32_t xk35 = u35;
  uint32_t ti36 = _t[36U];
  uint32_t
  v35 =
    vb36 +
      ((va35 + (vb36 ^ (vc36 ^ vd36)) + xk35 + ti36) << 4U |
        (va35 + (vb36 ^ (vc36 ^ vd36)) + xk35 + ti36) >> 28U);
  abcd[0U] = v35;
  uint32_t va36 = abcd[3U];
  uint32_t vb37 = abcd[0U];
  uint32_t vc37 = abcd[1U];
  uint32_t vd37 = abcd[2U];
  uint8_t *b37 = x + 16U;
  uint32_t u36 = load32_le(b37);
  uint32_t xk36 = u36;
  uint32_t ti37 = _t[37U];
  uint32_t
  v36 =
    vb37 +
      ((va36 + (vb37 ^ (vc37 ^ vd37)) + xk36 + ti37) << 11U |
        (va36 + (vb37 ^ (vc37 ^ vd37)) + xk36 + ti37) >> 21U);
  abcd[3U] = v36;
  uint32_t va37 = abcd[2U];
  uint32_t vb38 = abcd[3U];
  uint32_t vc38 = abcd[0U];
  uint32_t vd38 = abcd[1U];
  uint8_t *b38 = x + 28U;
  uint32_t u37 = load32_le(b38);
  uint32_t xk37 = u37;
  uint32_t ti38 = _t[38U];
  uint32_t
  v37 =
    vb38 +
      ((va37 + (vb38 ^ (vc38 ^ vd38)) + xk37 + ti38) << 16U |
        (va37 + (vb38 ^ (vc38 ^ vd38)) + xk37 + ti38) >> 16U);
  abcd[2U] = v37;
  uint32_t va38 = abcd[1U];
  uint32_t vb39 = abcd[2U];
  uint32_t vc39 = abcd[3U];
  uint32_t vd39 = abcd[0U];
  uint8_t *b39 = x + 40U;
  uint32_t u38 = load32_le(b39);
  uint32_t xk38 = u38;
  uint32_t ti39 = _t[39U];
  uint32_t
  v38 =
    vb39 +
      ((va38 + (vb39 ^ (vc39 ^ vd39)) + xk38 + ti39) << 23U |
        (va38 + (vb39 ^ (vc39 ^ vd39)) + xk38 + ti39) >> 9U);
  abcd[1U] = v38;
  uint32_t va39 = abcd[0U];
  uint32_t vb40 = abcd[1U];
  uint32_t vc40 = abcd[2U];
  uint32_t vd40 = abcd[3U];
  uint8_t *b40 = x + 52U;
  uint32_t u39 = load32_le(b40);
  uint32_t xk39 = u39;
  uint32_t ti40 = _t[40U];
  uint32_t
  v39 =
    vb40 +
      ((va39 + (vb40 ^ (vc40 ^ vd40)) + xk39 + ti40) << 4U |
        (va39 + (vb40 ^ (vc40 ^ vd40)) + xk39 + ti40) >> 28U);
  abcd[0U] = v39;
  uint32_t va40 = abcd[3U];
  uint32_t vb41 = abcd[0U];
  uint32_t vc41 = abcd[1U];
  uint32_t vd41 = abcd[2U];
  uint8_t *b41 = x;
  uint32_t u40 = load32_le(b41);
  uint32_t xk40 = u40;
  uint32_t ti41 = _t[41U];
  uint32_t
  v40 =
    vb41 +
      ((va40 + (vb41 ^ (vc41 ^ vd41)) + xk40 + ti41) << 11U |
        (va40 + (vb41 ^ (vc41 ^ vd41)) + xk40 + ti41) >> 21U);
  abcd[3U] = v40;
  uint32_t va41 = abcd[2U];
  uint32_t vb42 = abcd[3U];
  uint32_t vc42 = abcd[0U];
  uint32_t vd42 = abcd[1U];
  uint8_t *b42 = x + 12U;
  uint32_t u41 = load32_le(b42);
  uint32_t xk41 = u41;
  uint32_t ti42 = _t[42U];
  uint32_t
  v41 =
    vb42 +
      ((va41 + (vb42 ^ (vc42 ^ vd42)) + xk41 + ti42) << 16U |
        (va41 + (vb42 ^ (vc42 ^ vd42)) + xk41 + ti42) >> 16U);
  abcd[2U] = v41;
  uint32_t va42 = abcd[1U];
  uint32_t vb43 = abcd[2U];
  uint32_t vc43 = abcd[3U];
  uint32_t vd43 = abcd[0U];
  uint8_t *b43 = x + 24U;
  uint32_t u42 = load32_le(b43);
  uint32_t xk42 = u42;
  uint32_t ti43 = _t[43U];
  uint32_t
  v42 =
    vb43 +
      ((va42 + (vb43 ^ (vc43 ^ vd43)) + xk42 + ti43) << 23U |
        (va42 + (vb43 ^ (vc43 ^ vd43)) + xk42 + ti43) >> 9U);
  abcd[1U] = v42;
  uint32_t va43 = abcd[0U];
  uint32_t vb44 = abcd[1U];
  uint32_t vc44 = abcd[2U];
  uint32_t vd44 = abcd[3U];
  uint8_t *b44 = x + 36U;
  uint32_t u43 = load32_le(b44);
  uint32_t xk43 = u43;
  uint32_t ti44 = _t[44U];
  uint32_t
  v43 =
    vb44 +
      ((va43 + (vb44 ^ (vc44 ^ vd44)) + xk43 + ti44) << 4U |
        (va43 + (vb44 ^ (vc44 ^ vd44)) + xk43 + ti44) >> 28U);
  abcd[0U] = v43;
  uint32_t va44 = abcd[3U];
  uint32_t vb45 = abcd[0U];
  uint32_t vc45 = abcd[1U];
  uint32_t vd45 = abcd[2U];
  uint8_t *b45 = x + 48U;
  uint32_t u44 = load32_le(b45);
  uint32_t xk44 = u44;
  uint32_t ti45 = _t[45U];
  uint32_t
  v44 =
    vb45 +
      ((va44 + (vb45 ^ (vc45 ^ vd45)) + xk44 + ti45) << 11U |
        (va44 + (vb45 ^ (vc45 ^ vd45)) + xk44 + ti45) >> 21U);
  abcd[3U] = v44;
  uint32_t va45 = abcd[2U];
  uint32_t vb46 = abcd[3U];
  uint32_t vc46 = abcd[0U];
  uint32_t vd46 = abcd[1U];
  uint8_t *b46 = x + 60U;
  uint32_t u45 = load32_le(b46);
  uint32_t xk45 = u45;
  uint32_t ti46 = _t[46U];
  uint32_t
  v45 =
    vb46 +
      ((va45 + (vb46 ^ (vc46 ^ vd46)) + xk45 + ti46) << 16U |
        (va45 + (vb46 ^ (vc46 ^ vd46)) + xk45 + ti46) >> 16U);
  abcd[2U] = v45;
  uint32_t va46 = abcd[1U];
  uint32_t vb47 = abcd[2U];
  uint32_t vc47 = abcd[3U];
  uint32_t vd47 = abcd[0U];
  uint8_t *b47 = x + 8U;
  uint32_t u46 = load32_le(b47);
  uint32_t xk46 = u46;
  uint32_t ti47 = _t[47U];
  uint32_t
  v46 =
    vb47 +
      ((va46 + (vb47 ^ (vc47 ^ vd47)) + xk46 + ti47) << 23U |
        (va46 + (vb47 ^ (vc47 ^ vd47)) + xk46 + ti47) >> 9U);
  abcd[1U] = v46;
  uint32_t va47 = abcd[0U];
  uint32_t vb48 = abcd[1U];
  uint32_t vc48 = abcd[2U];
  uint32_t vd48 = abcd[3U];
  uint8_t *b48 = x;
  uint32_t u47 = load32_le(b48);
  uint32_t xk47 = u47;
  uint32_t ti48 = _t[48U];
  uint32_t
  v47 =
    vb48 +
      ((va47 + (vc48 ^ (vb48 | ~vd48)) + xk47 + ti48) << 6U |
        (va47 + (vc48 ^ (vb48 | ~vd48)) + xk47 + ti48) >> 26U);
  abcd[0U] = v47;
  uint32_t va48 = abcd[3U];
  uint32_t vb49 = abcd[0U];
  uint32_t vc49 = abcd[1U];
  uint32_t vd49 = abcd[2U];
  uint8_t *b49 = x + 28U;
  uint32_t u48 = load32_le(b49);
  uint32_t xk48 = u48;
  uint32_t ti49 = _t[49U];
  uint32_t
  v48 =
    vb49 +
      ((va48 + (vc49 ^ (vb49 | ~vd49)) + xk48 + ti49) << 10U |
        (va48 + (vc49 ^ (vb49 | ~vd49)) + xk48 + ti49) >> 22U);
  abcd[3U] = v48;
  uint32_t va49 = abcd[2U];
  uint32_t vb50 = abcd[3U];
  uint32_t vc50 = abcd[0U];
  uint32_t vd50 = abcd[1U];
  uint8_t *b50 = x + 56U;
  uint32_t u49 = load32_le(b50);
  uint32_t xk49 = u49;
  uint32_t ti50 = _t[50U];
  uint32_t
  v49 =
    vb50 +
      ((va49 + (vc50 ^ (vb50 | ~vd50)) + xk49 + ti50) << 15U |
        (va49 + (vc50 ^ (vb50 | ~vd50)) + xk49 + ti50) >> 17U);
  abcd[2U] = v49;
  uint32_t va50 = abcd[1U];
  uint32_t vb51 = abcd[2U];
  uint32_t vc51 = abcd[3U];
  uint32_t vd51 = abcd[0U];
  uint8_t *b51 = x + 20U;
  uint32_t u50 = load32_le(b51);
  uint32_t xk50 = u50;
  uint32_t ti51 = _t[51U];
  uint32_t
  v50 =
    vb51 +
      ((va50 + (vc51 ^ (vb51 | ~vd51)) + xk50 + ti51) << 21U |
        (va50 + (vc51 ^ (vb51 | ~vd51)) + xk50 + ti51) >> 11U);
  abcd[1U] = v50;
  uint32_t va51 = abcd[0U];
  uint32_t vb52 = abcd[1U];
  uint32_t vc52 = abcd[2U];
  uint32_t vd52 = abcd[3U];
  uint8_t *b52 = x + 48U;
  uint32_t u51 = load32_le(b52);
  uint32_t xk51 = u51;
  uint32_t ti52 = _t[52U];
  uint32_t
  v51 =
    vb52 +
      ((va51 + (vc52 ^ (vb52 | ~vd52)) + xk51 + ti52) << 6U |
        (va51 + (vc52 ^ (vb52 | ~vd52)) + xk51 + ti52) >> 26U);
  abcd[0U] = v51;
  uint32_t va52 = abcd[3U];
  uint32_t vb53 = abcd[0U];
  uint32_t vc53 = abcd[1U];
  uint32_t vd53 = abcd[2U];
  uint8_t *b53 = x + 12U;
  uint32_t u52 = load32_le(b53);
  uint32_t xk52 = u52;
  uint32_t ti53 = _t[53U];
  uint32_t
  v52 =
    vb53 +
      ((va52 + (vc53 ^ (vb53 | ~vd53)) + xk52 + ti53) << 10U |
        (va52 + (vc53 ^ (vb53 | ~vd53)) + xk52 + ti53) >> 22U);
  abcd[3U] = v52;
  uint32_t va53 = abcd[2U];
  uint32_t vb54 = abcd[3U];
  uint32_t vc54 = abcd[0U];
  uint32_t vd54 = abcd[1U];
  uint8_t *b54 = x + 40U;
  uint32_t u53 = load32_le(b54);
  uint32_t xk53 = u53;
  uint32_t ti54 = _t[54U];
  uint32_t
  v53 =
    vb54 +
      ((va53 + (vc54 ^ (vb54 | ~vd54)) + xk53 + ti54) << 15U |
        (va53 + (vc54 ^ (vb54 | ~vd54)) + xk53 + ti54) >> 17U);
  abcd[2U] = v53;
  uint32_t va54 = abcd[1U];
  uint32_t vb55 = abcd[2U];
  uint32_t vc55 = abcd[3U];
  uint32_t vd55 = abcd[0U];
  uint8_t *b55 = x + 4U;
  uint32_t u54 = load32_le(b55);
  uint32_t xk54 = u54;
  uint32_t ti55 = _t[55U];
  uint32_t
  v54 =
    vb55 +
      ((va54 + (vc55 ^ (vb55 | ~vd55)) + xk54 + ti55) << 21U |
        (va54 + (vc55 ^ (vb55 | ~vd55)) + xk54 + ti55) >> 11U);
  abcd[1U] = v54;
  uint32_t va55 = abcd[0U];
  uint32_t vb56 = abcd[1U];
  uint32_t vc56 = abcd[2U];
  uint32_t vd56 = abcd[3U];
  uint8_t *b56 = x + 32U;
  uint32_t u55 = load32_le(b56);
  uint32_t xk55 = u55;
  uint32_t ti56 = _t[56U];
  uint32_t
  v55 =
    vb56 +
      ((va55 + (vc56 ^ (vb56 | ~vd56)) + xk55 + ti56) << 6U |
        (va55 + (vc56 ^ (vb56 | ~vd56)) + xk55 + ti56) >> 26U);
  abcd[0U] = v55;
  uint32_t va56 = abcd[3U];
  uint32_t vb57 = abcd[0U];
  uint32_t vc57 = abcd[1U];
  uint32_t vd57 = abcd[2U];
  uint8_t *b57 = x + 60U;
  uint32_t u56 = load32_le(b57);
  uint32_t xk56 = u56;
  uint32_t ti57 = _t[57U];
  uint32_t
  v56 =
    vb57 +
      ((va56 + (vc57 ^ (vb57 | ~vd57)) + xk56 + ti57) << 10U |
        (va56 + (vc57 ^ (vb57 | ~vd57)) + xk56 + ti57) >> 22U);
  abcd[3U] = v56;
  uint32_t va57 = abcd[2U];
  uint32_t vb58 = abcd[3U];
  uint32_t vc58 = abcd[0U];
  uint32_t vd58 = abcd[1U];
  uint8_t *b58 = x + 24U;
  uint32_t u57 = load32_le(b58);
  uint32_t xk57 = u57;
  uint32_t ti58 = _t[58U];
  uint32_t
  v57 =
    vb58 +
      ((va57 + (vc58 ^ (vb58 | ~vd58)) + xk57 + ti58) << 15U |
        (va57 + (vc58 ^ (vb58 | ~vd58)) + xk57 + ti58) >> 17U);
  abcd[2U] = v57;
  uint32_t va58 = abcd[1U];
  uint32_t vb59 = abcd[2U];
  uint32_t vc59 = abcd[3U];
  uint32_t vd59 = abcd[0U];
  uint8_t *b59 = x + 52U;
  uint32_t u58 = load32_le(b59);
  uint32_t xk58 = u58;
  uint32_t ti59 = _t[59U];
  uint32_t
  v58 =
    vb59 +
      ((va58 + (vc59 ^ (vb59 | ~vd59)) + xk58 + ti59) << 21U |
        (va58 + (vc59 ^ (vb59 | ~vd59)) + xk58 + ti59) >> 11U);
  abcd[1U] = v58;
  uint32_t va59 = abcd[0U];
  uint32_t vb60 = abcd[1U];
  uint32_t vc60 = abcd[2U];
  uint32_t vd60 = abcd[3U];
  uint8_t *b60 = x + 16U;
  uint32_t u59 = load32_le(b60);
  uint32_t xk59 = u59;
  uint32_t ti60 = _t[60U];
  uint32_t
  v59 =
    vb60 +
      ((va59 + (vc60 ^ (vb60 | ~vd60)) + xk59 + ti60) << 6U |
        (va59 + (vc60 ^ (vb60 | ~vd60)) + xk59 + ti60) >> 26U);
  abcd[0U] = v59;
  uint32_t va60 = abcd[3U];
  uint32_t vb61 = abcd[0U];
  uint32_t vc61 = abcd[1U];
  uint32_t vd61 = abcd[2U];
  uint8_t *b61 = x + 44U;
  uint32_t u60 = load32_le(b61);
  uint32_t xk60 = u60;
  uint32_t ti61 = _t[61U];
  uint32_t
  v60 =
    vb61 +
      ((va60 + (vc61 ^ (vb61 | ~vd61)) + xk60 + ti61) << 10U |
        (va60 + (vc61 ^ (vb61 | ~vd61)) + xk60 + ti61) >> 22U);
  abcd[3U] = v60;
  uint32_t va61 = abcd[2U];
  uint32_t vb62 = abcd[3U];
  uint32_t vc62 = abcd[0U];
  uint32_t vd62 = abcd[1U];
  uint8_t *b62 = x + 8U;
  uint32_t u61 = load32_le(b62);
  uint32_t xk61 = u61;
  uint32_t ti62 = _t[62U];
  uint32_t
  v61 =
    vb62 +
      ((va61 + (vc62 ^ (vb62 | ~vd62)) + xk61 + ti62) << 15U |
        (va61 + (vc62 ^ (vb62 | ~vd62)) + xk61 + ti62) >> 17U);
  abcd[2U] = v61;
  uint32_t va62 = abcd[1U];
  uint32_t vb = abcd[2U];
  uint32_t vc = abcd[3U];
  uint32_t vd = abcd[0U];
  uint8_t *b63 = x + 36U;
  uint32_t u62 = load32_le(b63);
  uint32_t xk62 = u62;
  uint32_t ti = _t[63U];
  uint32_t
  v62 =
    vb +
      ((va62 + (vc ^ (vb | ~vd)) + xk62 + ti) << 21U | (va62 + (vc ^ (vb | ~vd)) + xk62 + ti) >> 11U);
  abcd[1U] = v62;
  uint32_t a = abcd[0U];
  uint32_t b = abcd[1U];
  uint32_t c = abcd[2U];
  uint32_t d = abcd[3U];
  abcd[0U] = a + aa;
  abcd[1U] = b + bb;
  abcd[2U] = c + cc;
  abcd[3U] = d + dd;
}

static void pad(uint64_t len, uint8_t *dst)
{
  uint8_t *dst1 = dst;
  dst1[0U] = 0x80U;
  uint8_t *dst2 = dst + 1U;
  for (uint32_t i = 0U; i < (128U - (9U + (uint32_t)(len % (uint64_t)64U))) % 64U; i++)
  {
    dst2[i] = 0U;
  }
  uint8_t *dst3 = dst + 1U + (128U - (9U + (uint32_t)(len % (uint64_t)64U))) % 64U;
  store64_le(dst3, len << 3U);
}

void Hacl_Hash_MD5_finish(uint32_t *s, uint8_t *dst)
{
  KRML_MAYBE_FOR4(i, 0U, 4U, 1U, store32_le(dst + i * 4U, s[i]););
}

void Hacl_Hash_MD5_update_multi(uint32_t *s, uint8_t *blocks, uint32_t n_blocks)
{
  for (uint32_t i = 0U; i < n_blocks; i++)
  {
    uint32_t sz = 64U;
    uint8_t *block = blocks + sz * i;
    update(s, block);
  }
}

void
Hacl_Hash_MD5_update_last(uint32_t *s, uint64_t prev_len, uint8_t *input, uint32_t input_len)
{
  uint32_t blocks_n = input_len / 64U;
  uint32_t blocks_len = blocks_n * 64U;
  uint8_t *blocks = input;
  uint32_t rest_len = input_len - blocks_len;
  uint8_t *rest = input + blocks_len;
  Hacl_Hash_MD5_update_multi(s, blocks, blocks_n);
  uint64_t total_input_len = prev_len + (uint64_t)input_len;
  uint32_t pad_len = 1U + (128U - (9U + (uint32_t)(total_input_len % (uint64_t)64U))) % 64U + 8U;
  uint32_t tmp_len = rest_len + pad_len;
  uint8_t tmp_twoblocks[128U] = { 0U };
  uint8_t *tmp = tmp_twoblocks;
  uint8_t *tmp_rest = tmp;
  uint8_t *tmp_pad = tmp + rest_len;
  memcpy(tmp_rest, rest, rest_len * sizeof (uint8_t));
  pad(total_input_len, tmp_pad);
  Hacl_Hash_MD5_update_multi(s, tmp, tmp_len / 64U);
}

void Hacl_Hash_MD5_hash_oneshot(uint8_t *output, uint8_t *input, uint32_t input_len)
{
  uint32_t s[4U] = { 0x67452301U, 0xefcdab89U, 0x98badcfeU, 0x10325476U };
  uint32_t blocks_n0 = input_len / 64U;
  uint32_t blocks_n1;
  if (input_len % 64U == 0U && blocks_n0 > 0U)
  {
    blocks_n1 = blocks_n0 - 1U;
  }
  else
  {
    blocks_n1 = blocks_n0;
  }
  uint32_t blocks_len0 = blocks_n1 * 64U;
  uint8_t *blocks0 = input;
  uint32_t rest_len0 = input_len - blocks_len0;
  uint8_t *rest0 = input + blocks_len0;
  uint32_t blocks_n = blocks_n1;
  uint32_t blocks_len = blocks_len0;
  uint8_t *blocks = blocks0;
  uint32_t rest_len = rest_len0;
  uint8_t *rest = rest0;
  Hacl_Hash_MD5_update_multi(s, blocks, blocks_n);
  Hacl_Hash_MD5_update_last(s, (uint64_t)blocks_len, rest, rest_len);
  Hacl_Hash_MD5_finish(s, output);
}

Hacl_Streaming_MD_state_32 *Hacl_Hash_MD5_malloc(void)
{
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC(64U, sizeof (uint8_t));
  if (buf == NULL)
  {
    return NULL;
  }
  uint8_t *buf1 = buf;
  uint32_t *b = (uint32_t *)KRML_HOST_CALLOC(4U, sizeof (uint32_t));
  Hacl_Streaming_Types_optional_32 block_state;
  if (b == NULL)
  {
    block_state = ((Hacl_Streaming_Types_optional_32){ .tag = Hacl_Streaming_Types_None });
  }
  else
  {
    block_state = ((Hacl_Streaming_Types_optional_32){ .tag = Hacl_Streaming_Types_Some, .v = b });
  }
  if (block_state.tag == Hacl_Streaming_Types_None)
  {
    KRML_HOST_FREE(buf1);
    return NULL;
  }
  if (block_state.tag == Hacl_Streaming_Types_Some)
  {
    uint32_t *block_state1 = block_state.v;
    Hacl_Streaming_Types_optional k_ = Hacl_Streaming_Types_Some;
    switch (k_)
    {
      case Hacl_Streaming_Types_None:
        {
          return NULL;
        }
      case Hacl_Streaming_Types_Some:
        {
          Hacl_Streaming_MD_state_32
          s = { .block_state = block_state1, .buf = buf1, .total_len = (uint64_t)0U };
          Hacl_Streaming_MD_state_32
          *p = (Hacl_Streaming_MD_state_32 *)KRML_HOST_MALLOC(sizeof (Hacl_Streaming_MD_state_32));
          if (p != NULL)
          {
            p[0U] = s;
          }
          if (p == NULL)
          {
            KRML_HOST_FREE(block_state1);
            KRML_HOST_FREE(buf1);
            return NULL;
          }
          Hacl_Hash_MD5_init(block_state1);
          return p;
        }
      default:
        {
          KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
          KRML_HOST_EXIT(253U);
        }
    }
  }
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "unreachable (pattern matches are exhaustive in F*)");
  KRML_HOST_EXIT(255U);
}

void Hacl_Hash_MD5_reset(Hacl_Streaming_MD_state_32 *state)
{
  Hacl_Streaming_MD_state_32 scrut = *state;
  uint8_t *buf = scrut.buf;
  uint32_t *block_state = scrut.block_state;
  Hacl_Hash_MD5_init(block_state);
  Hacl_Streaming_MD_state_32
  tmp = { .block_state = block_state, .buf = buf, .total_len = (uint64_t)0U };
  state[0U] = tmp;
}

/**
0 = success, 1 = max length exceeded
*/
Hacl_Streaming_Types_error_code
Hacl_Hash_MD5_update(Hacl_Streaming_MD_state_32 *state, uint8_t *chunk, uint32_t chunk_len)
{
  Hacl_Streaming_MD_state_32 s = *state;
  uint64_t total_len = s.total_len;
  if ((uint64_t)chunk_len > 2305843009213693951ULL - total_len)
  {
    return Hacl_Streaming_Types_MaximumLengthExceeded;
  }
  uint32_t sz;
  if (total_len % (uint64_t)64U == 0ULL && total_len > 0ULL)
  {
    sz = 64U;
  }
  else
  {
    sz = (uint32_t)(total_len % (uint64_t)64U);
  }
  if (chunk_len <= 64U - sz)
  {
    Hacl_Streaming_MD_state_32 s1 = *state;
    uint32_t *block_state1 = s1.block_state;
    uint8_t *buf = s1.buf;
    uint64_t total_len1 = s1.total_len;
    uint32_t sz1;
    if (total_len1 % (uint64_t)64U == 0ULL && total_len1 > 0ULL)
    {
      sz1 = 64U;
    }
    else
    {
      sz1 = (uint32_t)(total_len1 % (uint64_t)64U);
    }
    uint8_t *buf2 = buf + sz1;
    memcpy(buf2, chunk, chunk_len * sizeof (uint8_t));
    uint64_t total_len2 = total_len1 + (uint64_t)chunk_len;
    *state =
      (
        (Hacl_Streaming_MD_state_32){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len2
        }
      );
  }
  else if (sz == 0U)
  {
    Hacl_Streaming_MD_state_32 s1 = *state;
    uint32_t *block_state1 = s1.block_state;
    uint8_t *buf = s1.buf;
    uint64_t total_len1 = s1.total_len;
    uint32_t sz1;
    if (total_len1 % (uint64_t)64U == 0ULL && total_len1 > 0ULL)
    {
      sz1 = 64U;
    }
    else
    {
      sz1 = (uint32_t)(total_len1 % (uint64_t)64U);
    }
    if (!(sz1 == 0U))
    {
      Hacl_Hash_MD5_update_multi(block_state1, buf, 1U);
    }
    uint32_t ite;
    if ((uint64_t)chunk_len % (uint64_t)64U == 0ULL && (uint64_t)chunk_len > 0ULL)
    {
      ite = 64U;
    }
    else
    {
      ite = (uint32_t)((uint64_t)chunk_len % (uint64_t)64U);
    }
    uint32_t n_blocks = (chunk_len - ite) / 64U;
    uint32_t data1_len = n_blocks * 64U;
    uint32_t data2_len = chunk_len - data1_len;
    uint8_t *data1 = chunk;
    uint8_t *data2 = chunk + data1_len;
    Hacl_Hash_MD5_update_multi(block_state1, data1, data1_len / 64U);
    uint8_t *dst = buf;
    memcpy(dst, data2, data2_len * sizeof (uint8_t));
    *state =
      (
        (Hacl_Streaming_MD_state_32){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len1 + (uint64_t)chunk_len
        }
      );
  }
  else
  {
    uint32_t diff = 64U - sz;
    uint8_t *chunk1 = chunk;
    uint8_t *chunk2 = chunk + diff;
    Hacl_Streaming_MD_state_32 s1 = *state;
    uint32_t *block_state10 = s1.block_state;
    uint8_t *buf0 = s1.buf;
    uint64_t total_len10 = s1.total_len;
    uint32_t sz10;
    if (total_len10 % (uint64_t)64U == 0ULL && total_len10 > 0ULL)
    {
      sz10 = 64U;
    }
    else
    {
      sz10 = (uint32_t)(total_len10 % (uint64_t)64U);
    }
    uint8_t *buf2 = buf0 + sz10;
    memcpy(buf2, chunk1, diff * sizeof (uint8_t));
    uint64_t total_len2 = total_len10 + (uint64_t)diff;
    *state =
      (
        (Hacl_Streaming_MD_state_32){
          .block_state = block_state10,
          .buf = buf0,
          .total_len = total_len2
        }
      );
    Hacl_Streaming_MD_state_32 s10 = *state;
    uint32_t *block_state1 = s10.block_state;
    uint8_t *buf = s10.buf;
    uint64_t total_len1 = s10.total_len;
    uint32_t sz1;
    if (total_len1 % (uint64_t)64U == 0ULL && total_len1 > 0ULL)
    {
      sz1 = 64U;
    }
    else
    {
      sz1 = (uint32_t)(total_len1 % (uint64_t)64U);
    }
    if (!(sz1 == 0U))
    {
      Hacl_Hash_MD5_update_multi(block_state1, buf, 1U);
    }
    uint32_t ite;
    if
    ((uint64_t)(chunk_len - diff) % (uint64_t)64U == 0ULL && (uint64_t)(chunk_len - diff) > 0ULL)
    {
      ite = 64U;
    }
    else
    {
      ite = (uint32_t)((uint64_t)(chunk_len - diff) % (uint64_t)64U);
    }
    uint32_t n_blocks = (chunk_len - diff - ite) / 64U;
    uint32_t data1_len = n_blocks * 64U;
    uint32_t data2_len = chunk_len - diff - data1_len;
    uint8_t *data1 = chunk2;
    uint8_t *data2 = chunk2 + data1_len;
    Hacl_Hash_MD5_update_multi(block_state1, data1, data1_len / 64U);
    uint8_t *dst = buf;
    memcpy(dst, data2, data2_len * sizeof (uint8_t));
    *state =
      (
        (Hacl_Streaming_MD_state_32){
          .block_state = block_state1,
          .buf = buf,
          .total_len = total_len1 + (uint64_t)(chunk_len - diff)
        }
      );
  }
  return Hacl_Streaming_Types_Success;
}

void Hacl_Hash_MD5_digest(Hacl_Streaming_MD_state_32 *state, uint8_t *output)
{
  Hacl_Streaming_MD_state_32 scrut = *state;
  uint32_t *block_state = scrut.block_state;
  uint8_t *buf_ = scrut.buf;
  uint64_t total_len = scrut.total_len;
  uint32_t r;
  if (total_len % (uint64_t)64U == 0ULL && total_len > 0ULL)
  {
    r = 64U;
  }
  else
  {
    r = (uint32_t)(total_len % (uint64_t)64U);
  }
  uint8_t *buf_1 = buf_;
  uint32_t tmp_block_state[4U] = { 0U };
  memcpy(tmp_block_state, block_state, 4U * sizeof (uint32_t));
  uint32_t ite;
  if (r % 64U == 0U && r > 0U)
  {
    ite = 64U;
  }
  else
  {
    ite = r % 64U;
  }
  uint8_t *buf_last = buf_1 + r - ite;
  uint8_t *buf_multi = buf_1;
  Hacl_Hash_MD5_update_multi(tmp_block_state, buf_multi, 0U);
  uint64_t prev_len_last = total_len - (uint64_t)r;
  Hacl_Hash_MD5_update_last(tmp_block_state, prev_len_last, buf_last, r);
  Hacl_Hash_MD5_finish(tmp_block_state, output);
}

void Hacl_Hash_MD5_free(Hacl_Streaming_MD_state_32 *state)
{
  Hacl_Streaming_MD_state_32 scrut = *state;
  uint8_t *buf = scrut.buf;
  uint32_t *block_state = scrut.block_state;
  KRML_HOST_FREE(block_state);
  KRML_HOST_FREE(buf);
  KRML_HOST_FREE(state);
}

Hacl_Streaming_MD_state_32 *Hacl_Hash_MD5_copy(Hacl_Streaming_MD_state_32 *state)
{
  Hacl_Streaming_MD_state_32 scrut = *state;
  uint32_t *block_state0 = scrut.block_state;
  uint8_t *buf0 = scrut.buf;
  uint64_t total_len0 = scrut.total_len;
  uint8_t *buf = (uint8_t *)KRML_HOST_CALLOC(64U, sizeof (uint8_t));
  if (buf == NULL)
  {
    return NULL;
  }
  memcpy(buf, buf0, 64U * sizeof (uint8_t));
  uint32_t *b = (uint32_t *)KRML_HOST_CALLOC(4U, sizeof (uint32_t));
  Hacl_Streaming_Types_optional_32 block_state;
  if (b == NULL)
  {
    block_state = ((Hacl_Streaming_Types_optional_32){ .tag = Hacl_Streaming_Types_None });
  }
  else
  {
    block_state = ((Hacl_Streaming_Types_optional_32){ .tag = Hacl_Streaming_Types_Some, .v = b });
  }
  if (block_state.tag == Hacl_Streaming_Types_None)
  {
    KRML_HOST_FREE(buf);
    return NULL;
  }
  if (block_state.tag == Hacl_Streaming_Types_Some)
  {
    uint32_t *block_state1 = block_state.v;
    memcpy(block_state1, block_state0, 4U * sizeof (uint32_t));
    Hacl_Streaming_Types_optional k_ = Hacl_Streaming_Types_Some;
    switch (k_)
    {
      case Hacl_Streaming_Types_None:
        {
          return NULL;
        }
      case Hacl_Streaming_Types_Some:
        {
          Hacl_Streaming_MD_state_32
          s = { .block_state = block_state1, .buf = buf, .total_len = total_len0 };
          Hacl_Streaming_MD_state_32
          *p = (Hacl_Streaming_MD_state_32 *)KRML_HOST_MALLOC(sizeof (Hacl_Streaming_MD_state_32));
          if (p != NULL)
          {
            p[0U] = s;
          }
          if (p == NULL)
          {
            KRML_HOST_FREE(block_state1);
            KRML_HOST_FREE(buf);
            return NULL;
          }
          return p;
        }
      default:
        {
          KRML_HOST_EPRINTF("KaRaMeL incomplete match at %s:%d\n", __FILE__, __LINE__);
          KRML_HOST_EXIT(253U);
        }
    }
  }
  KRML_HOST_EPRINTF("KaRaMeL abort at %s:%d\n%s\n",
    __FILE__,
    __LINE__,
    "unreachable (pattern matches are exhaustive in F*)");
  KRML_HOST_EXIT(255U);
}

void Hacl_Hash_MD5_hash(uint8_t *output, uint8_t *input, uint32_t input_len)
{
  Hacl_Hash_MD5_hash_oneshot(output, input, input_len);
}

