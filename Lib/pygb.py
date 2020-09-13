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
    def __init__(self, A = MAXRGB, R = MINRGB, G = MINRGB, B = MINRGB):
        self.A = A
        self.R = R
        self.G = G
        self.B = B
        pass

    # Fix color values.
    def fix(self):
        from Lib.parser import tryParseInt

        self.A = tryParseInt(self.A)
        self.A = MAXRGB if self.A is None else self.A
        self.A = MAXRGB if self.A > MAXRGB else self.A
        self.A = MINRGB if self.A < MINRGB else self.A

        self.R = tryParseInt(self.R)
        self.R = MINRGB if self.R is None else self.R
        self.R = MAXRGB if self.R > MAXRGB else self.R
        self.R = MINRGB if self.R < MINRGB else self.R

        self.G = tryParseInt(self.G)
        self.G = MINRGB if self.G is None else self.G
        self.G = MAXRGB if self.G > MAXRGB else self.G
        self.G = MINRGB if self.G < MINRGB else self.G
        
        self.B = tryParseInt(self.B)
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
    def reverseWA(self):
        self.reverse()
        self.A = MAXRGB - self.A
        pass

    # Set color by another color object.
    def setBy(self, color):
        if isinstance(color, Color) == False:
            raise TypeError("This type is not 'Color'!")
        self.A = color.A
        self.R = color.R
        self.G = color.G
        self.B = color.B
        pass

    # Returns string color code.
    def strcode(self):
        self.fix()
        return f"{self.A}, {self.R}, {self.G}, {self.B}"
        pass

    # Set the shell foreground color.
    def applyShell(self):
        from Lib.parser import toSHCCode
        from sys import stdout
        stdout.write(toSHCCode(self))
        pass
    pass

# Returns color by name.
def colorByName(name = "white"):
    A = MAXRGB
    R = MINRGB
    G = MINRGB
    B = MINRGB
    name = name.lower().strip() if name is not None else "white"
    # Pink colors.
    if name == "mediumvioletred":
        R = 199
        G = 21
        B = 133
        pass
    elif name == "deeppink":
        R = 255
        G = 20
        B = 147
        pass
    elif name == "palevioletred":
        R = 219
        G = 112
        B = 147
        pass
    elif name == "hotpink":
        R = 255
        G = 105
        B = 180
        pass
    elif name == "lightpink":
        R = 255
        G = 182
        B = 193
        pass
    elif name == "pink":
        R = 255
        G = 192
        B = 203
        pass
    # Red colors.
    elif name == "darkred":
        R = 139
        G = 0
        B = 0
        pass
    elif name == "red":
        R = 255
        G = 0
        B = 0
        pass
    elif name == "firebrick":
        R = 178
        G = 34
        B = 34
        pass
    elif name == "crimson":
        R = 220
        G = 20
        B = 60
        pass
    elif name == "indianred":
        R = 205
        G = 92
        B = 92
        pass
    elif name == "lightcoral":
        R = 240
        G = 128
        B = 128
        pass
    elif name == "salmon":
        R = 250
        G = 128
        B = 114
        pass
    elif name == "darksalmon":
        R = 233
        G = 150
        B = 122
        pass
    elif name == "lightsalmon":
        R = 255
        G = 160
        B = 122
        pass
    # Orange colors.
    elif name == "orangered":
        R = 255
        G = 69
        B = 0
        pass
    elif name == "tomato":
        R = 255
        G = 99
        B = 71
        pass
    elif name == "darkorange":
        R = 255
        G = 140
        B = 0
        pass
    elif name == "coral":
        R = 255
        G = 127
        B = 80
        pass
    elif name == "orange":
        R = 255
        G = 165
        B = 0
        pass
    # Yellow colors.
    elif name == "darkkhaki":
        R = 189
        G = 183
        B = 107
        pass
    elif name == "gold":
        R = 255
        G = 215
        B = 0
        pass
    elif name == "khaki":
        R = 240
        G = 230
        B = 140
        pass
    elif name == "peachpuff":
        R = 255
        G = 218
        B = 185
        pass
    elif name == "yellow":
        R = 255
        G = 255
        B = 0
        pass
    elif name == "palegoldenrod":
        R = 238
        G = 232
        B = 170
        pass
    elif name == "moccasin":
        R = 255
        G = 228
        B = 181
        pass
    elif name == "papayawhip":
        R = 255
        G = 239
        B = 213
        pass
    elif name == "lightgoldenrodyellow":
        R = 250
        G = 250
        B = 210
        pass
    elif name == "lemonchiffon":
        R = 255
        G = 250
        B = 205
        pass
    elif name == "lightyellow":
        R = 255
        G = 255
        B = 224
        pass
    # Brown colors.
    elif name == "maroon":
        R = 128
        G = 0
        B = 0
        pass
    elif name == "brown":
        R = 165
        G = 42
        B = 42
        pass
    elif name == "saddlebrown":
        R = 139
        G = 69
        B = 19
        pass
    elif name == "sienna":
        R = 160
        G = 82
        B = 45
        pass
    elif name == "chocolate":
        R = 210
        G = 105
        B = 30
        pass
    elif name == "darkgoldenrod":
        R = 184
        G = 134
        B = 11
        pass
    elif name == "peru":
        R = 205
        G = 133
        B = 63
        pass
    elif name == "rosybrown":
        R = 188
        G = 143
        B = 143
        pass
    elif name == "goldenrod":
        R = 218
        G = 165
        B = 32
        pass
    elif name == "sandybrown":
        R = 244
        G = 164
        B = 96
        pass
    elif name == "tan":
        R = 210
        G = 180
        B = 140
        pass
    elif name == "burlywood":
        R = 222
        G = 184
        B = 135
        pass
    elif name == "wheat":
        R = 245
        G = 222
        B = 179
        pass
    elif name == "navajowhite":
        R = 255
        G = 222
        B = 173
        pass
    elif name == "bisque":
        R = 255
        G = 228
        B = 196
        pass
    elif name == "blanchedalmond":
        R = 255
        G = 235
        B = 205
        pass
    elif name == "cornsilk":
        R = 255
        G = 248
        B = 220
        pass
    # Green colors.
    elif name == "darkgreen":
        R = 0
        G = 100
        B = 0
        pass
    elif name == "green":
        R = 0
        G = 128
        B = 0
        pass
    elif name == "darkolivegreen":
        R = 85
        G = 107
        B = 47
        pass
    elif name == "forestgreen":
        R = 34
        G = 139
        B = 34
        pass
    elif name == "seagreen":
        R = 46
        G = 139
        B = 87
        pass
    elif name == "olive":
        R = 128
        G = 128
        B = 0
        pass
    elif name == "olivedrab":
        R = 107
        G = 142
        B = 35
        pass
    elif name == "mediumseagreen":
        R = 60
        G = 179
        B = 113
        pass
    elif name == "limegreen":
        R = 50
        G = 205
        B = 50
        pass
    elif name == "lime":
        R = 0
        G = 255
        B = 0
        pass
    elif name == "springgreen":
        R = 0
        G = 255
        B = 127
        pass
    elif name == "mediumspringgreen":
        R = 0
        G = 250
        B = 154
        pass
    elif name == "darkseagreen":
        R = 143
        G = 188
        B = 143
        pass
    elif name == "mediumaquamarine":
        R = 102
        G = 205
        B = 170
        pass
    elif name == "yellowgreen":
        R = 154
        G = 205
        B = 50
        pass
    elif name == "lawngreen":
        R = 124
        G = 252
        B = 0
        pass
    elif name == "chartreuse":
        R = 127
        G = 255
        B = 0
        pass
    elif name == "lightgreen":
        R = 144
        G = 238
        B = 144
        pass
    elif name == "greenyellow":
        R = 173
        G = 255
        B = 47
        pass
    elif name == "palegreen":
        R = 152
        G = 251
        B = 152
        pass
    # Cyan colors.
    elif name == "teal":
        R = 0
        G = 128
        B = 128
        pass
    elif name == "darkcyan":
        R = 0
        G = 139
        B = 139
        pass
    elif name == "lightseagreen":
        R = 32
        G = 178
        B = 170
        pass
    elif name == "cadetblue":
        R = 95
        G = 158
        B = 160
        pass
    elif name == "darkturquoise":
        R = 0
        G = 206
        B = 209
        pass
    elif name == "mediumturquoise":
        R = 72
        G = 209
        B = 204
        pass
    elif name == "turquoise":
        R = 64
        G = 224
        B = 208
        pass
    elif name == "aqua" or name == "cyan":
        R = 0
        G = 255
        B = 255
        pass
    elif name == "aquamarine":
        R = 127
        G = 255
        B = 212
        pass
    elif name == "paleturquoise":
        R = 175
        G = 238
        B = 238
        pass
    elif name == "lightcyan":
        R = 224
        G = 255
        B = 255
        pass
    # Blue colors.
    elif name == "navy":
        R = 0
        G = 0
        B = 128
        pass
    elif name == "darkblue":
        R = 0
        G = 0
        B = 139
        pass
    elif name == "mediumblue":
        R = 0
        G = 0
        B = 205
        pass
    elif name == "blue":
        R = 0
        G = 0
        B = 255
        pass
    elif name == "midnightblue":
        R = 25
        G = 25
        B = 112
        pass
    elif name == "royalblue":
        R = 65
        G = 105
        B = 225
        pass
    elif name == "steelblue":
        R = 70
        G = 130
        B = 180
        pass
    elif name == "dodgerblue":
        R = 30
        G = 144
        B = 255
        pass
    elif name == "deepskyblue":
        R = 0
        G = 191
        B = 255
        pass
    elif name == "cornflowerblue":
        R = 100
        G = 149
        B = 237
        pass
    elif name == "skyblue":
        R = 135
        G = 206
        B = 235
        pass
    elif name == "lightskyblue":
        R = 135
        G = 206
        B = 250
        pass
    elif name == "lightsteelblue":
        R = 176
        G = 196
        B = 222
        pass
    elif name == "lightblue":
        R = 173
        G = 216
        B = 230
        pass
    elif name == "powderblue":
        R = 176
        G = 224
        B = 230
        pass
    # Purple, violet and magenta colors.
    elif name == "indigo":
        R = 75
        G = 0
        B = 130
        pass
    elif name == "purple":
        R = 128
        G = 0
        B = 128
        pass
    elif name == "darkmagenta":
        R = 139
        G = 0
        B = 139
        pass
    elif name == "darkviolet":
        R = 148
        G = 0
        B = 211
        pass
    elif name == "darkslateblue":
        R = 72
        G = 61
        B = 139
        pass
    elif name == "blueviolet":
        R = 138
        G = 43
        B = 226
        pass
    elif name == "darkorchid":
        R = 153
        G = 50
        B = 204
        pass
    elif name == "fuchsia" or name == "magenta":
        R = 255
        G = 0
        B = 255
        pass
    elif name == "slateblue":
        R = 106
        G = 90
        B = 205
        pass
    elif name == "mediumslateblue":
        R = 123
        G = 104
        B = 238
        pass
    elif name == "mediumorchid":
        R = 186
        G = 85
        B = 211
        pass
    elif name == "mediumpurple":
        R = 147
        G = 112
        B = 219
        pass
    elif name == "orchid":
        R = 218
        G = 112
        B = 214
        pass
    elif name == "violet":
        R = 238
        G = 130
        B = 238
        pass
    elif name == "plum":
        R = 221
        G = 160
        B = 221
        pass
    elif name == "thistle":
        R = 216
        G = 191
        B = 216
        pass
    elif name == "lavender":
        R = 230
        G = 230
        B = 250
        pass
    # White colors.
    elif name == "mistyrose":
        R = 255
        G = 228
        B = 225
        pass
    elif name == "antiquewhite":
        R = 250
        G = 235
        B = 215
        pass
    elif name == "linen":
        R = 250
        G = 240
        B = 230
        pass
    elif name == "beige":
        R = 245
        G = 245
        B = 220
        pass
    elif name == "whitesmoke":
        R = 245
        G = 245
        B = 245
        pass
    elif name == "lavenderblush":
        R = 255
        G = 240
        B = 245
        pass
    elif name == "oldlace":
        R = 253
        G = 245
        B = 230
        pass
    elif name == "aliceblue":
        R = 240
        G = 248
        B = 255
        pass
    elif name == "seashell":
        R = 255
        G = 245
        B = 238
        pass
    elif name == "ghostwhite":
        R = 248
        G = 248
        B = 255
        pass
    elif name == "honeydew":
        R = 240
        G = 255
        B = 240
        pass
    elif name == "floralwhite":
        R = 255
        G = 250
        B = 240
        pass
    elif name == "azure":
        R = 240
        G = 255
        B = 255
        pass
    elif name == "mintcream":
        R = 245
        G = 255
        B = 250
        pass
    elif name == "snow":
        R = 255
        G = 250
        B = 250
        pass
    elif name == "ivory":
        R = 255
        G = 255
        B = 240
        pass
    elif name == "white":
        R = MAXRGB
        G = MAXRGB
        B = MAXRGB
        pass
    # Gray and black colors.
    elif name == "darkslategray":
        R = 47
        G = 79
        B = 79
        pass
    elif name == "dimgray":
        R = 105
        G = 105
        B = 105
        pass
    elif name == "slategray":
        R = 112
        G = 128
        B = 144
        pass
    elif name == "gray":
        R = 128
        G = 128
        B = 128
        pass
    elif name == "lightslategray":
        R = 119
        G = 136
        B = 153
        pass
    elif name == "darkgray":
        R = 169
        G = 169
        B = 169
        pass
    elif name == "silver":
        R = 192
        G = 192
        B = 192
        pass
    elif name == "lightgray":
        R = 211
        G = 211
        B = 211
        pass
    elif name == "gainsboro":
        R = 220
        G = 220
        B = 220
        pass
    else:
        R = MINRGB
        G = MINRGB
        B = MINRGB
        pass

    return Color(A,R,G,B)
    pass
