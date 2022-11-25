.. _tut-informal:

**********************************
Python 速览
**********************************

在下面的例子中，输入和输出是根据是否存在提示符(:term:`>>>` and :term:`...`)来区分的: 输入例子中的代码时，当提示符出现时，你必须在提示符后面输入所有内容;不以提示符开头的行是解释器的输出。注意，在一个例子中，次要提示符本身就意味着你必须输入一个空行;用于结束多行命令。

.. index:: single: # (hash); comment

本手册中的许多示例，甚至是那些在交互式提示下输入的示例，都包含注释。Python 中的注释以字符``#``开始，并延伸到物理行末尾。注释可以出现在行首或空格或代码之后，但不能出现在字符串文字中。字符串中的井号就是井号。因为注释是用来阐明代码的，不会被Python解释，所以在输入示例时可以省略注释。

几个例子：

   # this is the first comment
   spam = 1 # and this is the second comment
   # ... and now a third!
   text = "# This is not a comment because it's inside quotes."


.. _tut-calculator:

Python 用作计算器
============================

现在，尝试一些简单的 Python 命令。启动解释器，等待主提示符（>>> ）出现。

让我们尝试一些简单的Python命令。启动解释器并等待主提示符``>>>``出现。(很快就出现了。)



.. _tut-numbers:

数字
-------

解释器扮演一个简单的计算器：输入表达式，就会给出答案。表达式的语法很直接：运算符 ``+``, ``-``, ``*`` 和 ``/`` 的用法和其他大多数语言一样（比如，Pascal 或 C）；括号 (``()``) 用来分组。例如：

   >>> 2 + 2
   4
   >>> 50 - 5*6
   20
   >>> (50 - 5*6) / 4
   5.0
   >>> 8 / 5  # 除法总是返回浮点数
   1.6

整数 (如, ``2``, ``4``, ``20``) 的类型为 :class:`int`,
带小数 (如, ``5.0``, ``1.6``) 的类型为
:class:`float`.  本教程后面会介绍更多数字类型。

<<<<<<< HEAD
Division (``/``) always returns a float.  To do :term:`floor division` and
get an integer result you can use the ``//`` operator; to calculate
the remainder you can use ``%``::
=======
除法运算 (``/``) 返回浮点数。  用 ``//`` 运算符执行 :term:`floor division` 的结果为整数（忽略小数）; 计算余数用 ``%``:
>>>>>>> main

   >>> 17 / 3  # 除法运算返回浮点数
   5.666666666666667
   >>>
   >>> 17 // 3  # 整除运算去掉小数点部分
   5
   >>> 17 % 3  # 取模 % 运算返回除法的余数
   2
   >>> 5 * 3 + 2  # 结果 * 除数 + 余数
   17

Python 用 ``**`` 运算符计算乘方 [#]_::

   >>> 5 ** 2  # 5的平方
   25
   >>> 2 ** 7  # 2的7次方
   128

等号 (``=``) 用于给变量赋值。赋值后，下一个交互提示符的位置不显示任何结果::

   >>> width = 20
   >>> height = 5 * 9
   >>> width * height
   900

如果变量未定义（即未赋值），使用该变量会提示错误：::

   >>> n  # 尝试访问未定义的变量
   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   NameError: name 'n' is not defined

Python 全面支持浮点数；混合类型运算数的运算会把整数转换为浮点数::

   >>> 4 * 3.75 - 1
   14.0

交互模式下，上次输出的表达式会赋给变量
``_``.  这就意味着 Python 当作计算器时，用该变量实现下一步计算更简单，例如::

   >>> tax = 12.5 / 100
   >>> price = 100.50
   >>> price * tax
   12.5625
   >>> price + _
   113.0625
   >>> round(_, 2)
   113.06

最好将该变量视为只读的。不要为它显式赋值，否则会创建一个同名独立局部变量，该变量会用它的魔法行为屏蔽内置变量。

除了 :class:`int` 和 :class:`float` 之外， Python 还支持其他数字类型， 比如 :class:`~decimal.Decimal` and :class:`~fractions.Fraction`.
Python 还内置支持 :ref:`complex numbers <typesnumeric>`, 使用后缀 ``j`` 或 ``J`` 表示虚数 (如 ``3+5j``)。


.. _tut-strings:

字符串
-------

除了数字，Python 还可以操作字符串。字符串有多种表现形式，用单引号（``'...'``）或双引号（``"..."``）标注的结果相同 [#]_。 ``\`` 可用于转义：

   >>> 'spam eggs'  # 单引号
   'spam eggs'
   >>> 'doesn\'t'  # 使用 \' 转义单引号...
   "doesn't"
   >>> "doesn't"  # ...或使用双引号代替
   "doesn't"
   >>> '"Yes," they said.'
   '"Yes," they said.'
   >>> "\"Yes,\" they said."
   '"Yes," they said.'
   >>> '"Isn\'t," they said.'
   '"Isn\'t," they said.'

交互式解释器会为输出的字符串加注引号，特殊字符使用反斜杠转义。虽然，有时输出的字符串看起来与输入的字符串不一样（外加的引号可能会改变），但两个字符串是相同的。如果字符串中有单引号而没有双引号，该字符串外将加注双引号，反之，则加注单引号。print() 函数输出的内容更简洁易读，它会省略两边的引号，并输出转义后的特殊字符：

   >>> '"Isn\'t," they said.'
   '"Isn\'t," they said.'
   >>> print('"Isn\'t," they said.')
   "Isn't," they said.
   >>> s = 'First line.\nSecond line.'  # \n 意味着换行
   >>> s  # 没有 print() 时, \n 包含在输出行
   'First line.\nSecond line.'
   >>> print(s)  # 有 print() 时, \n 换行
   First line.
   Second line.

如果你不希望前 ``\`` 的字符转义成特殊字符, 可以使用 *原始字符串* ，在引号前添加 ``r`` 即可::

   >>> print('C:\some\name')  # 这儿 \n 意味着换行!
   C:\some
   ame
   >>> print(r'C:\some\name')  # 注意引号前的 r
   C:\some\name

字符串字面值可以实现跨行连续输入。实现方式是用三引号："""...""" 或 '''...'''，字符串行尾会自动加上回车换行，如果不需要回车换行，在行尾添加 ``\`` 即可。示例如下：

   print("""\
   Usage: thingy [OPTIONS]
   -h                        Display this usage message
   -H hostname               Hostname to connect to
   """)

输出如下 (注意，第一行没有换行):

.. code-block:: text

   Usage: thingy [OPTIONS]
        -h                        Display this usage message
        -H hostname               Hostname to connect to

字符串可以使用 ``+`` 进行合并（串连在一起)，也可以用  ``*``  进行重复：

   >>> # 重复3次 'un', 然后合并 'ium'
   >>> 3 * 'un' + 'ium'
   'unununium'

相邻的两个或多个 *字符串字面量* (如 引号标注的字符) 会自动合并 ::

   >>> 'Py' 'thon'
   'Python'

拆分长字符串时，这个功能特别实用::

   >>> text = ('Put several strings within parentheses '
   ...         'to have them joined together.')
   >>> text
   'Put several strings within parentheses to have them joined together.'

这项功能只能用于两个字面值，不能用于变量或表达式::

   >>> prefix = 'Py'
   >>> prefix 'thon'  # 不能连接变量和字符串字面量
     File "<stdin>", line 1
       prefix 'thon'
              ^^^^^^
   SyntaxError: invalid syntax
   >>> ('un' * 3) 'ium'
     File "<stdin>", line 1
       ('un' * 3) 'ium'
                  ^^^^^
   SyntaxError: invalid syntax

如果你想合并多个变量，或合并变量与字面值，使用  ``+`` 即可::

   >>> prefix + 'thon'
   'Python'

字符串支持 *索引* (下标访问), 第一个字符的索引是 0。单字符没有专用的类型，就是长度为一的字符串::

   >>> word = 'Python'
   >>> word[0]  # 位置为0的字符
   'P'
   >>> word[5]  # 位置为5的字符
   'n'

索引还支持负数，用负数索引时，从右边开始计数::

   >>> word[-1]  # 最后一个字符
   'n'
   >>> word[-2]  # 倒数第二个字符
   'o'
   >>> word[-6]
   'P'

注意，-0 和 0 一样，因此，负数索引从 -1 开始。

除了索引, 字符串还支持 *切片*。  索引可以提取单个字符，切片* 则提取子字符串::

   >>> word[0:2]  # 从位置0(包括)到位置2(不包括)的字符
   'Py'
   >>> word[2:5]  # 从位置2(包括)到位置5(不包括)的字符
   'tho'

注意，输出结果包含切片开始，但不包含切片结束。  这样
确保 ``s[:i] + s[i:]`` 一直等于 ``s``::

   >>> word[:2] + word[2:]
   'Python'
   >>> word[:4] + word[4:]
   'Python'

切片索引的默认值很有用；省略开始索引时，默认值为 0，省略结束索引时，默认为到字符串的结尾::

   >>> word[:2]   # 从开始到位置2(不包括)的字符
   'Py'
   >>> word[4:]   # 从位置4(包括)到结束的字符
   'on'
   >>> word[-2:]  # 从倒数第二个(包括)到结束的字符
   'on'

还可以这样理解切片，索引指向的是*字符之间* ，第一个字符的左侧标为 0，最后一个字符的右侧标为 n ，n 是字符串长度。例如::

    +---+---+---+---+---+---+
    | P | y | t | h | o | n |
    +---+---+---+---+---+---+
    0   1   2   3   4   5   6
   -6  -5  -4  -3  -2  -1

第一行数字是字符串中索引 0...6 的位置，第二行数字是对应的负数索引位置。*i* 到 *j* 的切片由 *i* 和 *j* 之间所有对应的字符组成。

对于使用非负索引的切片，如果两个索引都不越界，切片长度就是起止索引之差。例如，  ``word[1:3]`` 的长度是 2。

索引越界会报错::

   >>> word[42]  # 该单词仅有6个字符
   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   IndexError: string index out of range

然而, 切片会自动处理越界索引::

   >>> word[4:42]
   'on'
   >>> word[42:]
   ''

   Python 字符串不能修改 --- 它们 :term:`immutable`的。
   因此，为字符串中某个索引位置赋值会报错::

   >>> word[0] = 'J'
   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   TypeError: 'str' object does not support item assignment
   >>> word[2:] = 'py'
   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   TypeError: 'str' object does not support item assignment

要生成不同的字符串，应新建一个字符串::

   >>> 'J' + word[1:]
   'Jython'
   >>> word[:2] + 'py'
   'Pypy'

内置函数 :func:`len` 返回字符串的长度::

   >>> s = 'supercalifragilisticexpialidocious'
   >>> len(s)
   34


.. seealso::

   :ref:`textseq`
      字符串是 *序列类型*, 支持序列类型的各种操作。

   :ref:`string-methods`
      字符串支持很多变形与查找方法。

   :ref:`f-strings`
      内嵌表达式的字符串字面值。

   :ref:`formatstrings`
      使用 :meth:`str.format` 格式化字符串。

   :ref:`old-string-formatting`
      这里详述了用 ``%`` 运算符格式化字符串的操作。


.. _tut-lists:

列表
-----

Python 支持多种 *复合* 数据类型，可将不同值组合在一起。最通用的是 *列表*，是用方括号标注，逗号分隔的一组值。列表可以包含不同类型的元素，但通常下各个元素的类型相同：
列表可能包含不同类型的项，但这些项通常具有相同的类型。::

   >>> squares = [1, 4, 9, 16, 25]
   >>> squares
   [1, 4, 9, 16, 25]

和字符串（以及其他内置 :term:`sequence` 类型）一样，列表支持索引和切片::

   >>> squares[0]  # 索引返回项
   1
   >>> squares[-1]
   25
   >>> squares[-3:]  # 切片返回一个新列表
   [9, 16, 25]

所有的切片操作返回包含请求元素的新列表。以下切片操作会返回列表的
:ref:`shallow copy <shallow_vs_deep_copy>`::

   >>> squares[:]
   [1, 4, 9, 16, 25]

列表还支持合并操作::

   >>> squares + [36, 49, 64, 81, 100]
   [1, 4, 9, 16, 25, 36, 49, 64, 81, 100]

跟 :term:`immutable` 字符串不同, 列表是 :term:`mutable`
类型, 比如它的内容是可以改变的::

    >>> cubes = [1, 8, 27, 65, 125]  # 某个字符是错误的
    >>> 4 ** 3  # 4的3次方是64, 而不是65!
    64
    >>> cubes[3] = 64  # 替换错误的值
    >>> cubes
    [1, 8, 27, 64, 125]


通过使用 :meth:`~list.append` *method* (详见后文), 你也可以在列表末尾添加新元素::

   >>> cubes.append(216)  # 把6的立方加起来
   >>> cubes.append(7 ** 3)  # 添加7的立方
   >>> cubes
   [1, 8, 27, 64, 125, 216, 343]

为切片赋值可以改变列表大小，甚至清空整个列表::

   >>> letters = ['a', 'b', 'c', 'd', 'e', 'f', 'g']
   >>> letters
   ['a', 'b', 'c', 'd', 'e', 'f', 'g']
   >>> # 替换一些值
   >>> letters[2:5] = ['C', 'D', 'E']
   >>> letters
   ['a', 'b', 'C', 'D', 'E', 'f', 'g']
   >>> # 移除它们
   >>> letters[2:5] = []
   >>> letters
   ['a', 'b', 'f', 'g']
   >>> # 通过将所有元素替换为空列表来清除列表
   >>> letters[:] = []
   >>> letters
   []

内置函数 :func:`len` 也适用列表::

   >>> letters = ['a', 'b', 'c', 'd']
   >>> len(letters)
   4

还可以嵌套列表（创建包含其他列表的列表），例如::

   >>> a = ['a', 'b', 'c']
   >>> n = [1, 2, 3]
   >>> x = [a, n]
   >>> x
   [['a', 'b', 'c'], [1, 2, 3]]
   >>> x[0]
   ['a', 'b', 'c']
   >>> x[0][1]
   'b'

.. _tut-firststeps:

走向编程的第一步
===============================

当然，我可以使用 Python 完成比二加二更复杂的任务。 例如，可以编写 `Fibonacci series <https://en.wikipedia.org/wiki/Fibonacci_number>`_ 的初始子序列，如下所示::

   >>> #  斐波那契数列 :
   ... # 两个元素的和定义下一个元素
   ... a, b = 0, 1
   >>> while a < 10:
   ...     print(a)
   ...     a, b = b, a+b
   ...
   0
   1
   1
   2
   3
   5
   8

该例引入了几个新功能。

* 首行 *多重赋值* : 变量 ``a` 和 ``b`` 同时获得新值 0 和 1。 最后一行又用了一次多重赋值，这体现在右表达式在赋值前就已经求值了。右表达式求值顺序为从左到右。

* :keyword:`while` 循环只要条件（这里指：``a < 10``）为真就会一直执行。Python 和 C 一样，任何非零整数都为真，零为假。这个条件也可以是字符串或列表的值，事实上，任何序列都可以；长度非零就为真，空序列则为假。示例中的判断只是最简单的比较。比较操作符的标准写法和 C 语言一样： ``<`` （小于）、 ``>``  （大于）、 ``==``（等于）、 ``<=``（小于等于)、 ``>=``（大于等于）及 ``!=`` （不等于）。

* *循环体* 是 *缩进的* ：缩进是 Python组织语句的方式。在交互式命令行里，得为每个缩输入制表符或空格。使用文本编辑器可以实现更复杂的输入方式；所有像样的文本编辑器都支持自动缩进。交互式输入复合语句时, 要在最后输入空白行表示结束（因为解析器不知道哪一行代码是最后一行）。注意，同一块语句的每一行的缩进相同。

* :func:`print` 函数输出给定参数的值。与表达式不同（比如，之前计算器的例子），它能处理多个参数，包括浮点数与字符串。它输出的字符串不带引号，且各参数项之间会插入一个空格，这样可以实现更好的格式化操作。比如这样::

     >>> i = 256*256
     >>> print('The value of i is', i)
     The value of i is 65536

  关键字参数 *end* 可以取消输出后面的换行, 或用另一个字符串结尾：

     >>> a, b = 0, 1
     >>> while a < 1000:
     ...     print(a, end=',')
     ...     a, b = b, a+b
     ...
     0,1,1,2,3,5,8,13,21,34,55,89,144,233,377,610,987,


.. rubric:: Footnotes

.. [#] 由于 ``**``  比 ``-`` 的优先级更高, 所以 ``-3**2`` 会被解释成 ``-(3**2)`` 得到 ``-9``.  要避免这个问题，并且得到 ``9``, 可以用 ``(-3)**2``。

.. [#] 和其他语言不一样，特殊字符如 ``\n`` 在单引号（``'...'``）和双引号（``"..."``）里的意义一样。这两种引号唯一的区别是，不需要在单引号里转义双引号 ``"``，但必须把单引号转义成 ``\'``，反之亦然。
