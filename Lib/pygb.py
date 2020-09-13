"""pygb.py - The RGB-ARGB and color library"""

MAXRGB = 255 # Maximum value of rgb.
MINRGB = 0 # Minimum value of rgb.

class Color:
    # Members.

    A = MAXRGB # Aplha.
    R = MINRGB # Red.
    G = MINRGB # Green.
    B = MINRGB # Blue.

    # Constructor.
    def __init__(self, A = MAXRGB, R = MINRGB, G = MINRGB, B = MINRGB) -> None:
        self.A = A
        self.R = R
        self.G = G
        self.B = B
        pass

    # Fix color values.
    def fix(self) -> None:
        self.A = MAXRGB if self.A is None else self.A
        self.A = MAXRGB if self.A > MAXRGB else self.A
        self.A = MINRGB if self.A < MINRGB else self.A

        self.R = MINRGB if self.R is None else self.R
        self.R = MAXRGB if self.R > MAXRGB else self.R
        self.R = MINRGB if self.R < MINRGB else self.R

        self.G = MINRGB if self.G is None else self.G
        self.G = MAXRGB if self.G > MAXRGB else self.G
        self.G = MINRGB if self.G < MINRGB else self.G

        self.B = MINRGB if self.B is None else self.B
        self.B = MAXRGB if self.B > MAXRGB else self.B
        self.B = MINRGB if self.B < MINRGB else self.B
        pass

    # Reverse color.
    def reverse(self):
        self.fix()
        self.R = MAXRGB - self.R
        self.G = MAXRGB - self.G
        self.B = MAXRGB - self.B
        pass

    # Reverse color with alpha.
    def reverseWA(self) -> None:
        self.reverse()
        self.A = MAXRGB - self.A
        pass

    # Set color by another color object.
    def setBy(self, color) -> None:
        if isinstance(color, Color) == False:
            raise TypeError("This type is not 'Color'!")
        self.A = color.A
        self.R = color.R
        self.G = color.G
        self.B = color.B
        pass

    # Returns string color code.
    def strcode(self) -> str:
        self.fix()
        return f"{self.A}, {self.R}, {self.G}, {self.B}"
        pass
    pass
