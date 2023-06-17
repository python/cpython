from typing import Set
import operator as op

variables = {}


class MyCode:
    index = 0

    def __hash__(self):
        MyCode.index += 1
        return MyCode.index

    def do(self):
        ...


class SetValue(MyCode):
    """赋值 a = 15 语句"""

    def __init__(self, name, value):
        self.name = name
        self.value = value

    def do(self):
        variables[self.name] = self.value

    def __str__(self):
        return "<SetValue>"


class GetValue(MyCode):
    def __init__(self, name):
        self.name = name

    def do(self):
        return variables[self.name]


class ChangeValue(MyCode):
    # i += 1
    def __init__(self, name, dv, op):
        self.name = name
        self.dv = dv
        self.op = op

    def do(self):
        variables[self.name] = self.op(variables[self.name], self.dv)


class Op(MyCode):
    # i < 1000
    def __init__(self, left, operator, value):
        self.left = left
        self.operator = operator
        self.value = value

    def do(self):
        if isinstance(self.left, str):
            return self.operator(variables[self.left], self.value)
        elif isinstance(self.left, MyCode):
            return self.operator(self.left.do(), self.value)
        else:
            return self.operator(self.left, self.value)


# class Judge(MyCode):
#     def __init__(self, name, value):
#         self.name = name
#         self.value = value
#
#     def do(self):
#         return variables[self.name] == self.value
#
#     def __str__(self):
#         return "<Judge>"


class If(MyCode):
    def __init__(self, *args, **kwargs):
        if len(args) == 4:  # if ((condition) {} else {})
            # if (() {})
            self.condition = args[0]
            self.doCode: Set[MyCode] = args[1]
            self.elseDo: Set[MyCode] = args[3]
        elif len(args) == 2:
            self.condition = args[0]
            self.doCode: Set[MyCode] = args[1]
            self.elseDo: Set[MyCode] = set()

    def do(self):
        if self.condition.do():
            for code in self.doCode:
                code.do()  # bug
        elif self.elseDo:
            for code in self.elseDo:
                code.do()

    def __str__(self):
        return "<IF>"


class For(MyCode):
    def __init__(self, initContent, judgeContent, loopContent, loopCode):
        self.initContent = initContent
        self.judgeContent = judgeContent
        self.loopContent = loopContent
        self.loopCode: Set[MyCode] = loopCode

    def do(self):
        self.initContent.do()
        while self.judgeContent.do():
            for code in self.loopCode:
                code.do()
            self.loopContent.do()


class Print(MyCode):
    def __init__(self, string):
        self.string = string

    def do(self):
        if isinstance(self.string, str):
            print(self.string)
        elif isinstance(self.string, MyCode):
            print(self.string.do())


class NameSpace:
    def __init__(self, *args):
        self.arr = args

    def run(self):
        for code in self.arr:
            code.do()


# 打印出0~1000内左右的偶数
ns = NameSpace(
    For(SetValue("i", 0), Op(GetValue("i"), op.lt, 1000), ChangeValue("i", 1, op.add), {
        # (i % 2) == 0
        If((Op(Op("i", op.mod, 2), op.eq, 0)), {
            Print(GetValue("i"))
        })
    })
)

def main():
    ns.run()
    # print(variables)


if __name__ == "__main__":
    main()
