from __future__ import annotations
USING_STRINGS = True

# dataclass_module_3.py and dataclass_module_2_str.py are identical
# except only the latter uses string annotations.

from dataclasses import dataclass
import test.test_dataclasses._types_proxy as tp

T_CV2 = tp.ClassVar[int]
T_CV3 = tp.ClassVar

T_IV2 = tp.InitVar[int]
T_IV3 = tp.InitVar

@dataclass
class CV:
    T_CV4 = tp.ClassVar
    cv0: tp.ClassVar[int] = 20
    cv1: tp.ClassVar = 30
    cv2: T_CV2
    cv3: T_CV3
    not_cv4: T_CV4  # When using string annotations, this field is not recognized as a ClassVar.

@dataclass
class IV:
    T_IV4 = tp.InitVar
    iv0: tp.InitVar[int]
    iv1: tp.InitVar
    iv2: T_IV2
    iv3: T_IV3
    not_iv4: T_IV4  # When using string annotations, this field is not recognized as an InitVar.
