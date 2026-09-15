from __future__ import annotations
USING_STRINGS = True

# dataclass_module_4.py and dataclass_module_4_str.py are identical
# except only the latter uses string annotations.

from dataclasses import dataclass
import dataclasses
import typing

class TypingProxy:
    class Nested:
        ClassVar = typing.ClassVar
        InitVar = dataclasses.InitVar

T_CV2 = TypingProxy.Nested.ClassVar[int]
T_CV3 = TypingProxy.Nested.ClassVar

T_IV2 = TypingProxy.Nested.InitVar[int]
T_IV3 = TypingProxy.Nested.InitVar

@dataclass
class CV:
    T_CV4 = TypingProxy.Nested.ClassVar
    cv0: TypingProxy.Nested.ClassVar[int] = 20
    cv1: TypingProxy.Nested.ClassVar = 30
    cv2: T_CV2
    cv3: T_CV3
    not_cv4: T_CV4  # When using string annotations, this field is not recognized as a ClassVar.

@dataclass
class IV:
    T_IV4 = TypingProxy.Nested.InitVar
    iv0: TypingProxy.Nested.InitVar[int]
    iv1: TypingProxy.Nested.InitVar
    iv2: T_IV2
    iv3: T_IV3
    not_iv4: T_IV4  # When using string annotations, this field is not recognized as an InitVar.
