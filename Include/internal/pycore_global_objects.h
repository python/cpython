#ifndef Py_INTERNAL_GLOBAL_OBJECTS_H
#define Py_INTERNAL_GLOBAL_OBJECTS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


#define _PY_NSMALLPOSINTS           257
#define _PY_NSMALLNEGINTS           5

#define _PyLong_NEG_INIT(val) \
    { PyVarObject_HEAD_INIT(&PyLong_Type, -1) .ob_digit = { -(val) }, }
#define _PyLong_ZERO_INIT \
    { PyVarObject_HEAD_INIT(&PyLong_Type, 0) .ob_digit = { 0 }, }
#define _PyLong_POS_INIT(val) \
    { PyVarObject_HEAD_INIT(&PyLong_Type, 1) .ob_digit = { val }, }


// Only immutable objects should be considered runtime-global.
// All others must be per-interpreter.

#define _Py_GLOBAL_OBJECT(NAME) \
    _PyRuntime.global_objects.NAME
#define _Py_SINGLETON(NAME) \
    _Py_GLOBAL_OBJECT(singletons.NAME)

struct _Py_global_objects {
    struct {
        /* Small integers are preallocated in this array so that they
         * can be shared.
         * The integers that are preallocated are those in the range
         *-_PY_NSMALLNEGINTS (inclusive) to _PY_NSMALLPOSINTS (not inclusive).
         */
        PyLongObject small_ints[_PY_NSMALLNEGINTS + _PY_NSMALLPOSINTS];
    } singletons;
};

#define _Py_global_objects_INIT { \
    .singletons = { \
        .small_ints = { \
            _PyLong_NEG_INIT(-5), \
            _PyLong_NEG_INIT(-4), \
            _PyLong_NEG_INIT(-3), \
            _PyLong_NEG_INIT(-2), \
            _PyLong_NEG_INIT(-1), \
            _PyLong_ZERO_INIT, \
            _PyLong_POS_INIT(1), \
            _PyLong_POS_INIT(2), \
            _PyLong_POS_INIT(3), \
            _PyLong_POS_INIT(4), \
            _PyLong_POS_INIT(5), \
            _PyLong_POS_INIT(6), \
            _PyLong_POS_INIT(7), \
            _PyLong_POS_INIT(8), \
            _PyLong_POS_INIT(9), \
            _PyLong_POS_INIT(10), \
            _PyLong_POS_INIT(11), \
            _PyLong_POS_INIT(12), \
            _PyLong_POS_INIT(13), \
            _PyLong_POS_INIT(14), \
            _PyLong_POS_INIT(15), \
            _PyLong_POS_INIT(16), \
            _PyLong_POS_INIT(17), \
            _PyLong_POS_INIT(18), \
            _PyLong_POS_INIT(19), \
            _PyLong_POS_INIT(20), \
            _PyLong_POS_INIT(21), \
            _PyLong_POS_INIT(22), \
            _PyLong_POS_INIT(23), \
            _PyLong_POS_INIT(24), \
            _PyLong_POS_INIT(25), \
            _PyLong_POS_INIT(26), \
            _PyLong_POS_INIT(27), \
            _PyLong_POS_INIT(28), \
            _PyLong_POS_INIT(29), \
            _PyLong_POS_INIT(30), \
            _PyLong_POS_INIT(31), \
            _PyLong_POS_INIT(32), \
            _PyLong_POS_INIT(33), \
            _PyLong_POS_INIT(34), \
            _PyLong_POS_INIT(35), \
            _PyLong_POS_INIT(36), \
            _PyLong_POS_INIT(37), \
            _PyLong_POS_INIT(38), \
            _PyLong_POS_INIT(39), \
            _PyLong_POS_INIT(40), \
            _PyLong_POS_INIT(41), \
            _PyLong_POS_INIT(42), \
            _PyLong_POS_INIT(43), \
            _PyLong_POS_INIT(44), \
            _PyLong_POS_INIT(45), \
            _PyLong_POS_INIT(46), \
            _PyLong_POS_INIT(47), \
            _PyLong_POS_INIT(48), \
            _PyLong_POS_INIT(49), \
            _PyLong_POS_INIT(50), \
            _PyLong_POS_INIT(51), \
            _PyLong_POS_INIT(52), \
            _PyLong_POS_INIT(53), \
            _PyLong_POS_INIT(54), \
            _PyLong_POS_INIT(55), \
            _PyLong_POS_INIT(56), \
            _PyLong_POS_INIT(57), \
            _PyLong_POS_INIT(58), \
            _PyLong_POS_INIT(59), \
            _PyLong_POS_INIT(60), \
            _PyLong_POS_INIT(61), \
            _PyLong_POS_INIT(62), \
            _PyLong_POS_INIT(63), \
            _PyLong_POS_INIT(64), \
            _PyLong_POS_INIT(65), \
            _PyLong_POS_INIT(66), \
            _PyLong_POS_INIT(67), \
            _PyLong_POS_INIT(68), \
            _PyLong_POS_INIT(69), \
            _PyLong_POS_INIT(70), \
            _PyLong_POS_INIT(71), \
            _PyLong_POS_INIT(72), \
            _PyLong_POS_INIT(73), \
            _PyLong_POS_INIT(74), \
            _PyLong_POS_INIT(75), \
            _PyLong_POS_INIT(76), \
            _PyLong_POS_INIT(77), \
            _PyLong_POS_INIT(78), \
            _PyLong_POS_INIT(79), \
            _PyLong_POS_INIT(80), \
            _PyLong_POS_INIT(81), \
            _PyLong_POS_INIT(82), \
            _PyLong_POS_INIT(83), \
            _PyLong_POS_INIT(84), \
            _PyLong_POS_INIT(85), \
            _PyLong_POS_INIT(86), \
            _PyLong_POS_INIT(87), \
            _PyLong_POS_INIT(88), \
            _PyLong_POS_INIT(89), \
            _PyLong_POS_INIT(90), \
            _PyLong_POS_INIT(91), \
            _PyLong_POS_INIT(92), \
            _PyLong_POS_INIT(93), \
            _PyLong_POS_INIT(94), \
            _PyLong_POS_INIT(95), \
            _PyLong_POS_INIT(96), \
            _PyLong_POS_INIT(97), \
            _PyLong_POS_INIT(98), \
            _PyLong_POS_INIT(99), \
            _PyLong_POS_INIT(100), \
            _PyLong_POS_INIT(101), \
            _PyLong_POS_INIT(102), \
            _PyLong_POS_INIT(103), \
            _PyLong_POS_INIT(104), \
            _PyLong_POS_INIT(105), \
            _PyLong_POS_INIT(106), \
            _PyLong_POS_INIT(107), \
            _PyLong_POS_INIT(108), \
            _PyLong_POS_INIT(109), \
            _PyLong_POS_INIT(110), \
            _PyLong_POS_INIT(111), \
            _PyLong_POS_INIT(112), \
            _PyLong_POS_INIT(113), \
            _PyLong_POS_INIT(114), \
            _PyLong_POS_INIT(115), \
            _PyLong_POS_INIT(116), \
            _PyLong_POS_INIT(117), \
            _PyLong_POS_INIT(118), \
            _PyLong_POS_INIT(119), \
            _PyLong_POS_INIT(120), \
            _PyLong_POS_INIT(121), \
            _PyLong_POS_INIT(122), \
            _PyLong_POS_INIT(123), \
            _PyLong_POS_INIT(124), \
            _PyLong_POS_INIT(125), \
            _PyLong_POS_INIT(126), \
            _PyLong_POS_INIT(127), \
            _PyLong_POS_INIT(128), \
            _PyLong_POS_INIT(129), \
            _PyLong_POS_INIT(130), \
            _PyLong_POS_INIT(131), \
            _PyLong_POS_INIT(132), \
            _PyLong_POS_INIT(133), \
            _PyLong_POS_INIT(134), \
            _PyLong_POS_INIT(135), \
            _PyLong_POS_INIT(136), \
            _PyLong_POS_INIT(137), \
            _PyLong_POS_INIT(138), \
            _PyLong_POS_INIT(139), \
            _PyLong_POS_INIT(140), \
            _PyLong_POS_INIT(141), \
            _PyLong_POS_INIT(142), \
            _PyLong_POS_INIT(143), \
            _PyLong_POS_INIT(144), \
            _PyLong_POS_INIT(145), \
            _PyLong_POS_INIT(146), \
            _PyLong_POS_INIT(147), \
            _PyLong_POS_INIT(148), \
            _PyLong_POS_INIT(149), \
            _PyLong_POS_INIT(150), \
            _PyLong_POS_INIT(151), \
            _PyLong_POS_INIT(152), \
            _PyLong_POS_INIT(153), \
            _PyLong_POS_INIT(154), \
            _PyLong_POS_INIT(155), \
            _PyLong_POS_INIT(156), \
            _PyLong_POS_INIT(157), \
            _PyLong_POS_INIT(158), \
            _PyLong_POS_INIT(159), \
            _PyLong_POS_INIT(160), \
            _PyLong_POS_INIT(161), \
            _PyLong_POS_INIT(162), \
            _PyLong_POS_INIT(163), \
            _PyLong_POS_INIT(164), \
            _PyLong_POS_INIT(165), \
            _PyLong_POS_INIT(166), \
            _PyLong_POS_INIT(167), \
            _PyLong_POS_INIT(168), \
            _PyLong_POS_INIT(169), \
            _PyLong_POS_INIT(170), \
            _PyLong_POS_INIT(171), \
            _PyLong_POS_INIT(172), \
            _PyLong_POS_INIT(173), \
            _PyLong_POS_INIT(174), \
            _PyLong_POS_INIT(175), \
            _PyLong_POS_INIT(176), \
            _PyLong_POS_INIT(177), \
            _PyLong_POS_INIT(178), \
            _PyLong_POS_INIT(179), \
            _PyLong_POS_INIT(180), \
            _PyLong_POS_INIT(181), \
            _PyLong_POS_INIT(182), \
            _PyLong_POS_INIT(183), \
            _PyLong_POS_INIT(184), \
            _PyLong_POS_INIT(185), \
            _PyLong_POS_INIT(186), \
            _PyLong_POS_INIT(187), \
            _PyLong_POS_INIT(188), \
            _PyLong_POS_INIT(189), \
            _PyLong_POS_INIT(190), \
            _PyLong_POS_INIT(191), \
            _PyLong_POS_INIT(192), \
            _PyLong_POS_INIT(193), \
            _PyLong_POS_INIT(194), \
            _PyLong_POS_INIT(195), \
            _PyLong_POS_INIT(196), \
            _PyLong_POS_INIT(197), \
            _PyLong_POS_INIT(198), \
            _PyLong_POS_INIT(199), \
            _PyLong_POS_INIT(200), \
            _PyLong_POS_INIT(201), \
            _PyLong_POS_INIT(202), \
            _PyLong_POS_INIT(203), \
            _PyLong_POS_INIT(204), \
            _PyLong_POS_INIT(205), \
            _PyLong_POS_INIT(206), \
            _PyLong_POS_INIT(207), \
            _PyLong_POS_INIT(208), \
            _PyLong_POS_INIT(209), \
            _PyLong_POS_INIT(210), \
            _PyLong_POS_INIT(211), \
            _PyLong_POS_INIT(212), \
            _PyLong_POS_INIT(213), \
            _PyLong_POS_INIT(214), \
            _PyLong_POS_INIT(215), \
            _PyLong_POS_INIT(216), \
            _PyLong_POS_INIT(217), \
            _PyLong_POS_INIT(218), \
            _PyLong_POS_INIT(219), \
            _PyLong_POS_INIT(220), \
            _PyLong_POS_INIT(221), \
            _PyLong_POS_INIT(222), \
            _PyLong_POS_INIT(223), \
            _PyLong_POS_INIT(224), \
            _PyLong_POS_INIT(225), \
            _PyLong_POS_INIT(226), \
            _PyLong_POS_INIT(227), \
            _PyLong_POS_INIT(228), \
            _PyLong_POS_INIT(229), \
            _PyLong_POS_INIT(230), \
            _PyLong_POS_INIT(231), \
            _PyLong_POS_INIT(232), \
            _PyLong_POS_INIT(233), \
            _PyLong_POS_INIT(234), \
            _PyLong_POS_INIT(235), \
            _PyLong_POS_INIT(236), \
            _PyLong_POS_INIT(237), \
            _PyLong_POS_INIT(238), \
            _PyLong_POS_INIT(239), \
            _PyLong_POS_INIT(240), \
            _PyLong_POS_INIT(241), \
            _PyLong_POS_INIT(242), \
            _PyLong_POS_INIT(243), \
            _PyLong_POS_INIT(244), \
            _PyLong_POS_INIT(245), \
            _PyLong_POS_INIT(246), \
            _PyLong_POS_INIT(247), \
            _PyLong_POS_INIT(248), \
            _PyLong_POS_INIT(249), \
            _PyLong_POS_INIT(250), \
            _PyLong_POS_INIT(251), \
            _PyLong_POS_INIT(252), \
            _PyLong_POS_INIT(253), \
            _PyLong_POS_INIT(254), \
            _PyLong_POS_INIT(255), \
            _PyLong_POS_INIT(256), \
        }, \
    }, \
}

static inline void
_Py_global_objects_reset(struct _Py_global_objects *objects)
{
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GLOBAL_OBJECTS_H */
