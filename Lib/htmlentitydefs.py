"""HTML character entity references."""

entitydefs = {
    'AElig':    '\306',         # latin capital letter AE = latin capital ligature AE, U+00C6 ISOlat1
    'Aacute':   '\301',         # latin capital letter A with acute, U+00C1 ISOlat1
    'Acirc':    '\302',         # latin capital letter A with circumflex, U+00C2 ISOlat1
    'Agrave':   '\300',         # latin capital letter A with grave = latin capital letter A grave, U+00C0 ISOlat1
    'Alpha':    '&#913;',       # greek capital letter alpha, U+0391
    'Aring':    '\305',         # latin capital letter A with ring above = latin capital letter A ring, U+00C5 ISOlat1
    'Atilde':   '\303',         # latin capital letter A with tilde, U+00C3 ISOlat1
    'Auml':     '\304',         # latin capital letter A with diaeresis, U+00C4 ISOlat1
    'Beta':     '&#914;',       # greek capital letter beta, U+0392
    'Ccedil':   '\307',         # latin capital letter C with cedilla, U+00C7 ISOlat1
    'Chi':      '&#935;',       # greek capital letter chi, U+03A7
    'Dagger':   '&#8225;',      # double dagger, U+2021 ISOpub
    'Delta':    '&#916;',       # greek capital letter delta, U+0394 ISOgrk3
    'ETH':      '\320',         # latin capital letter ETH, U+00D0 ISOlat1
    'Eacute':   '\311',         # latin capital letter E with acute, U+00C9 ISOlat1
    'Ecirc':    '\312',         # latin capital letter E with circumflex, U+00CA ISOlat1
    'Egrave':   '\310',         # latin capital letter E with grave, U+00C8 ISOlat1
    'Epsilon':  '&#917;',       # greek capital letter epsilon, U+0395
    'Eta':      '&#919;',       # greek capital letter eta, U+0397
    'Euml':     '\313',         # latin capital letter E with diaeresis, U+00CB ISOlat1
    'Gamma':    '&#915;',       # greek capital letter gamma, U+0393 ISOgrk3
    'Iacute':   '\315',         # latin capital letter I with acute, U+00CD ISOlat1
    'Icirc':    '\316',         # latin capital letter I with circumflex, U+00CE ISOlat1
    'Igrave':   '\314',         # latin capital letter I with grave, U+00CC ISOlat1
    'Iota':     '&#921;',       # greek capital letter iota, U+0399
    'Iuml':     '\317',         # latin capital letter I with diaeresis, U+00CF ISOlat1
    'Kappa':    '&#922;',       # greek capital letter kappa, U+039A
    'Lambda':   '&#923;',       # greek capital letter lambda, U+039B ISOgrk3
    'Mu':       '&#924;',       # greek capital letter mu, U+039C
    'Ntilde':   '\321',         # latin capital letter N with tilde, U+00D1 ISOlat1
    'Nu':       '&#925;',       # greek capital letter nu, U+039D
    'OElig':    '&#338;',       # latin capital ligature OE, U+0152 ISOlat2
    'Oacute':   '\323',         # latin capital letter O with acute, U+00D3 ISOlat1
    'Ocirc':    '\324',         # latin capital letter O with circumflex, U+00D4 ISOlat1
    'Ograve':   '\322',         # latin capital letter O with grave, U+00D2 ISOlat1
    'Omega':    '&#937;',       # greek capital letter omega, U+03A9 ISOgrk3
    'Omicron':  '&#927;',       # greek capital letter omicron, U+039F
    'Oslash':   '\330',         # latin capital letter O with stroke = latin capital letter O slash, U+00D8 ISOlat1
    'Otilde':   '\325',         # latin capital letter O with tilde, U+00D5 ISOlat1
    'Ouml':     '\326',         # latin capital letter O with diaeresis, U+00D6 ISOlat1
    'Phi':      '&#934;',       # greek capital letter phi, U+03A6 ISOgrk3
    'Pi':       '&#928;',       # greek capital letter pi, U+03A0 ISOgrk3
    'Prime':    '&#8243;',      # double prime = seconds = inches, U+2033 ISOtech
    'Psi':      '&#936;',       # greek capital letter psi, U+03A8 ISOgrk3
    'Rho':      '&#929;',       # greek capital letter rho, U+03A1
    'Scaron':   '&#352;',       # latin capital letter S with caron, U+0160 ISOlat2
    'Sigma':    '&#931;',       # greek capital letter sigma, U+03A3 ISOgrk3
    'THORN':    '\336',         # latin capital letter THORN, U+00DE ISOlat1
    'Tau':      '&#932;',       # greek capital letter tau, U+03A4
    'Theta':    '&#920;',       # greek capital letter theta, U+0398 ISOgrk3
    'Uacute':   '\332',         # latin capital letter U with acute, U+00DA ISOlat1
    'Ucirc':    '\333',         # latin capital letter U with circumflex, U+00DB ISOlat1
    'Ugrave':   '\331',         # latin capital letter U with grave, U+00D9 ISOlat1
    'Upsilon':  '&#933;',       # greek capital letter upsilon, U+03A5 ISOgrk3
    'Uuml':     '\334',         # latin capital letter U with diaeresis, U+00DC ISOlat1
    'Xi':       '&#926;',       # greek capital letter xi, U+039E ISOgrk3
    'Yacute':   '\335',         # latin capital letter Y with acute, U+00DD ISOlat1
    'Yuml':     '&#376;',       # latin capital letter Y with diaeresis, U+0178 ISOlat2
    'Zeta':     '&#918;',       # greek capital letter zeta, U+0396
    'aacute':   '\341',         # latin small letter a with acute, U+00E1 ISOlat1
    'acirc':    '\342',         # latin small letter a with circumflex, U+00E2 ISOlat1
    'acute':    '\264',         # acute accent = spacing acute, U+00B4 ISOdia
    'aelig':    '\346',         # latin small letter ae = latin small ligature ae, U+00E6 ISOlat1
    'agrave':   '\340',         # latin small letter a with grave = latin small letter a grave, U+00E0 ISOlat1
    'alefsym':  '&#8501;',      # alef symbol = first transfinite cardinal, U+2135 NEW
    'alpha':    '&#945;',       # greek small letter alpha, U+03B1 ISOgrk3
    'amp':      '\46',          # ampersand, U+0026 ISOnum
    'and':      '&#8743;',      # logical and = wedge, U+2227 ISOtech
    'ang':      '&#8736;',      # angle, U+2220 ISOamso
    'aring':    '\345',         # latin small letter a with ring above = latin small letter a ring, U+00E5 ISOlat1
    'asymp':    '&#8776;',      # almost equal to = asymptotic to, U+2248 ISOamsr
    'atilde':   '\343',         # latin small letter a with tilde, U+00E3 ISOlat1
    'auml':     '\344',         # latin small letter a with diaeresis, U+00E4 ISOlat1
    'bdquo':    '&#8222;',      # double low-9 quotation mark, U+201E NEW
    'beta':     '&#946;',       # greek small letter beta, U+03B2 ISOgrk3
    'brvbar':   '\246',         # broken bar = broken vertical bar, U+00A6 ISOnum
    'bull':     '&#8226;',      # bullet = black small circle, U+2022 ISOpub
    'cap':      '&#8745;',      # intersection = cap, U+2229 ISOtech
    'ccedil':   '\347',         # latin small letter c with cedilla, U+00E7 ISOlat1
    'cedil':    '\270',         # cedilla = spacing cedilla, U+00B8 ISOdia
    'cent':     '\242',         # cent sign, U+00A2 ISOnum
    'chi':      '&#967;',       # greek small letter chi, U+03C7 ISOgrk3
    'circ':     '&#710;',       # modifier letter circumflex accent, U+02C6 ISOpub
    'clubs':    '&#9827;',      # black club suit = shamrock, U+2663 ISOpub
    'cong':     '&#8773;',      # approximately equal to, U+2245 ISOtech
    'copy':     '\251',         # copyright sign, U+00A9 ISOnum
    'crarr':    '&#8629;',      # downwards arrow with corner leftwards = carriage return, U+21B5 NEW
    'cup':      '&#8746;',      # union = cup, U+222A ISOtech
    'curren':   '\244',         # currency sign, U+00A4 ISOnum
    'dArr':     '&#8659;',      # downwards double arrow, U+21D3 ISOamsa
    'dagger':   '&#8224;',      # dagger, U+2020 ISOpub
    'darr':     '&#8595;',      # downwards arrow, U+2193 ISOnum
    'deg':      '\260',         # degree sign, U+00B0 ISOnum
    'delta':    '&#948;',       # greek small letter delta, U+03B4 ISOgrk3
    'diams':    '&#9830;',      # black diamond suit, U+2666 ISOpub
    'divide':   '\367',         # division sign, U+00F7 ISOnum
    'eacute':   '\351',         # latin small letter e with acute, U+00E9 ISOlat1
    'ecirc':    '\352',         # latin small letter e with circumflex, U+00EA ISOlat1
    'egrave':   '\350',         # latin small letter e with grave, U+00E8 ISOlat1
    'empty':    '&#8709;',      # empty set = null set = diameter, U+2205 ISOamso
    'emsp':     '&#8195;',      # em space, U+2003 ISOpub
    'ensp':     '&#8194;',      # en space, U+2002 ISOpub
    'epsilon':  '&#949;',       # greek small letter epsilon, U+03B5 ISOgrk3
    'equiv':    '&#8801;',      # identical to, U+2261 ISOtech
    'eta':      '&#951;',       # greek small letter eta, U+03B7 ISOgrk3
    'eth':      '\360',         # latin small letter eth, U+00F0 ISOlat1
    'euml':     '\353',         # latin small letter e with diaeresis, U+00EB ISOlat1
    'euro':     '&#8364;',      # euro sign, U+20AC NEW
    'exist':    '&#8707;',      # there exists, U+2203 ISOtech
    'fnof':     '&#402;',       # latin small f with hook = function = florin, U+0192 ISOtech
    'forall':   '&#8704;',      # for all, U+2200 ISOtech
    'frac12':   '\275',         # vulgar fraction one half = fraction one half, U+00BD ISOnum
    'frac14':   '\274',         # vulgar fraction one quarter = fraction one quarter, U+00BC ISOnum
    'frac34':   '\276',         # vulgar fraction three quarters = fraction three quarters, U+00BE ISOnum
    'frasl':    '&#8260;',      # fraction slash, U+2044 NEW
    'gamma':    '&#947;',       # greek small letter gamma, U+03B3 ISOgrk3
    'ge':       '&#8805;',      # greater-than or equal to, U+2265 ISOtech
    'gt':       '\76',          # greater-than sign, U+003E ISOnum
    'hArr':     '&#8660;',      # left right double arrow, U+21D4 ISOamsa
    'harr':     '&#8596;',      # left right arrow, U+2194 ISOamsa
    'hearts':   '&#9829;',      # black heart suit = valentine, U+2665 ISOpub
    'hellip':   '&#8230;',      # horizontal ellipsis = three dot leader, U+2026 ISOpub
    'iacute':   '\355',         # latin small letter i with acute, U+00ED ISOlat1
    'icirc':    '\356',         # latin small letter i with circumflex, U+00EE ISOlat1
    'iexcl':    '\241',         # inverted exclamation mark, U+00A1 ISOnum
    'igrave':   '\354',         # latin small letter i with grave, U+00EC ISOlat1
    'image':    '&#8465;',      # blackletter capital I = imaginary part, U+2111 ISOamso
    'infin':    '&#8734;',      # infinity, U+221E ISOtech
    'int':      '&#8747;',      # integral, U+222B ISOtech
    'iota':     '&#953;',       # greek small letter iota, U+03B9 ISOgrk3
    'iquest':   '\277',         # inverted question mark = turned question mark, U+00BF ISOnum
    'isin':     '&#8712;',      # element of, U+2208 ISOtech
    'iuml':     '\357',         # latin small letter i with diaeresis, U+00EF ISOlat1
    'kappa':    '&#954;',       # greek small letter kappa, U+03BA ISOgrk3
    'lArr':     '&#8656;',      # leftwards double arrow, U+21D0 ISOtech
    'lambda':   '&#955;',       # greek small letter lambda, U+03BB ISOgrk3
    'lang':     '&#9001;',      # left-pointing angle bracket = bra, U+2329 ISOtech
    'laquo':    '\253',         # left-pointing double angle quotation mark = left pointing guillemet, U+00AB ISOnum
    'larr':     '&#8592;',      # leftwards arrow, U+2190 ISOnum
    'lceil':    '&#8968;',      # left ceiling = apl upstile, U+2308 ISOamsc
    'ldquo':    '&#8220;',      # left double quotation mark, U+201C ISOnum
    'le':       '&#8804;',      # less-than or equal to, U+2264 ISOtech
    'lfloor':   '&#8970;',      # left floor = apl downstile, U+230A ISOamsc
    'lowast':   '&#8727;',      # asterisk operator, U+2217 ISOtech
    'loz':      '&#9674;',      # lozenge, U+25CA ISOpub
    'lrm':      '&#8206;',      # left-to-right mark, U+200E NEW RFC 2070
    'lsaquo':   '&#8249;',      # single left-pointing angle quotation mark, U+2039 ISO proposed
    'lsquo':    '&#8216;',      # left single quotation mark, U+2018 ISOnum
    'lt':       '\74',          # less-than sign, U+003C ISOnum
    'macr':     '\257',         # macron = spacing macron = overline = APL overbar, U+00AF ISOdia
    'mdash':    '&#8212;',      # em dash, U+2014 ISOpub
    'micro':    '\265',         # micro sign, U+00B5 ISOnum
    'middot':   '\267',         # middle dot = Georgian comma = Greek middle dot, U+00B7 ISOnum
    'minus':    '&#8722;',      # minus sign, U+2212 ISOtech
    'mu':       '&#956;',       # greek small letter mu, U+03BC ISOgrk3
    'nabla':    '&#8711;',      # nabla = backward difference, U+2207 ISOtech
    'nbsp':     '\240',         # no-break space = non-breaking space, U+00A0 ISOnum
    'ndash':    '&#8211;',      # en dash, U+2013 ISOpub
    'ne':       '&#8800;',      # not equal to, U+2260 ISOtech
    'ni':       '&#8715;',      # contains as member, U+220B ISOtech
    'not':      '\254',         # not sign, U+00AC ISOnum
    'notin':    '&#8713;',      # not an element of, U+2209 ISOtech
    'nsub':     '&#8836;',      # not a subset of, U+2284 ISOamsn
    'ntilde':   '\361',         # latin small letter n with tilde, U+00F1 ISOlat1
    'nu':       '&#957;',       # greek small letter nu, U+03BD ISOgrk3
    'oacute':   '\363',         # latin small letter o with acute, U+00F3 ISOlat1
    'ocirc':    '\364',         # latin small letter o with circumflex, U+00F4 ISOlat1
    'oelig':    '&#339;',       # latin small ligature oe, U+0153 ISOlat2
    'ograve':   '\362',         # latin small letter o with grave, U+00F2 ISOlat1
    'oline':    '&#8254;',      # overline = spacing overscore, U+203E NEW
    'omega':    '&#969;',       # greek small letter omega, U+03C9 ISOgrk3
    'omicron':  '&#959;',       # greek small letter omicron, U+03BF NEW
    'oplus':    '&#8853;',      # circled plus = direct sum, U+2295 ISOamsb
    'or':       '&#8744;',      # logical or = vee, U+2228 ISOtech
    'ordf':     '\252',         # feminine ordinal indicator, U+00AA ISOnum
    'ordm':     '\272',         # masculine ordinal indicator, U+00BA ISOnum
    'oslash':   '\370',         # latin small letter o with stroke, = latin small letter o slash, U+00F8 ISOlat1
    'otilde':   '\365',         # latin small letter o with tilde, U+00F5 ISOlat1
    'otimes':   '&#8855;',      # circled times = vector product, U+2297 ISOamsb
    'ouml':     '\366',         # latin small letter o with diaeresis, U+00F6 ISOlat1
    'para':     '\266',         # pilcrow sign = paragraph sign, U+00B6 ISOnum
    'part':     '&#8706;',      # partial differential, U+2202 ISOtech
    'permil':   '&#8240;',      # per mille sign, U+2030 ISOtech
    'perp':     '&#8869;',      # up tack = orthogonal to = perpendicular, U+22A5 ISOtech
    'phi':      '&#966;',       # greek small letter phi, U+03C6 ISOgrk3
    'pi':       '&#960;',       # greek small letter pi, U+03C0 ISOgrk3
    'piv':      '&#982;',       # greek pi symbol, U+03D6 ISOgrk3
    'plusmn':   '\261',         # plus-minus sign = plus-or-minus sign, U+00B1 ISOnum
    'pound':    '\243',         # pound sign, U+00A3 ISOnum
    'prime':    '&#8242;',      # prime = minutes = feet, U+2032 ISOtech
    'prod':     '&#8719;',      # n-ary product = product sign, U+220F ISOamsb
    'prop':     '&#8733;',      # proportional to, U+221D ISOtech
    'psi':      '&#968;',       # greek small letter psi, U+03C8 ISOgrk3
    'quot':     '\42',          # quotation mark = APL quote, U+0022 ISOnum
    'rArr':     '&#8658;',      # rightwards double arrow, U+21D2 ISOtech
    'radic':    '&#8730;',      # square root = radical sign, U+221A ISOtech
    'rang':     '&#9002;',      # right-pointing angle bracket = ket, U+232A ISOtech
    'raquo':    '\273',         # right-pointing double angle quotation mark = right pointing guillemet, U+00BB ISOnum
    'rarr':     '&#8594;',      # rightwards arrow, U+2192 ISOnum
    'rceil':    '&#8969;',      # right ceiling, U+2309 ISOamsc
    'rdquo':    '&#8221;',      # right double quotation mark, U+201D ISOnum
    'real':     '&#8476;',      # blackletter capital R = real part symbol, U+211C ISOamso
    'reg':      '\256',         # registered sign = registered trade mark sign, U+00AE ISOnum
    'rfloor':   '&#8971;',      # right floor, U+230B ISOamsc
    'rho':      '&#961;',       # greek small letter rho, U+03C1 ISOgrk3
    'rlm':      '&#8207;',      # right-to-left mark, U+200F NEW RFC 2070
    'rsaquo':   '&#8250;',      # single right-pointing angle quotation mark, U+203A ISO proposed
    'rsquo':    '&#8217;',      # right single quotation mark, U+2019 ISOnum
    'sbquo':    '&#8218;',      # single low-9 quotation mark, U+201A NEW
    'scaron':   '&#353;',       # latin small letter s with caron, U+0161 ISOlat2
    'sdot':     '&#8901;',      # dot operator, U+22C5 ISOamsb
    'sect':     '\247',         # section sign, U+00A7 ISOnum
    'shy':      '\255',         # soft hyphen = discretionary hyphen, U+00AD ISOnum
    'sigma':    '&#963;',       # greek small letter sigma, U+03C3 ISOgrk3
    'sigmaf':   '&#962;',       # greek small letter final sigma, U+03C2 ISOgrk3
    'sim':      '&#8764;',      # tilde operator = varies with = similar to, U+223C ISOtech
    'spades':   '&#9824;',      # black spade suit, U+2660 ISOpub
    'sub':      '&#8834;',      # subset of, U+2282 ISOtech
    'sube':     '&#8838;',      # subset of or equal to, U+2286 ISOtech
    'sum':      '&#8721;',      # n-ary sumation, U+2211 ISOamsb
    'sup':      '&#8835;',      # superset of, U+2283 ISOtech
    'sup1':     '\271',         # superscript one = superscript digit one, U+00B9 ISOnum
    'sup2':     '\262',         # superscript two = superscript digit two = squared, U+00B2 ISOnum
    'sup3':     '\263',         # superscript three = superscript digit three = cubed, U+00B3 ISOnum
    'supe':     '&#8839;',      # superset of or equal to, U+2287 ISOtech
    'szlig':    '\337',         # latin small letter sharp s = ess-zed, U+00DF ISOlat1
    'tau':      '&#964;',       # greek small letter tau, U+03C4 ISOgrk3
    'there4':   '&#8756;',      # therefore, U+2234 ISOtech
    'theta':    '&#952;',       # greek small letter theta, U+03B8 ISOgrk3
    'thetasym': '&#977;',       # greek small letter theta symbol, U+03D1 NEW
    'thinsp':   '&#8201;',      # thin space, U+2009 ISOpub
    'thorn':    '\376',         # latin small letter thorn with, U+00FE ISOlat1
    'tilde':    '&#732;',       # small tilde, U+02DC ISOdia
    'times':    '\327',         # multiplication sign, U+00D7 ISOnum
    'trade':    '&#8482;',      # trade mark sign, U+2122 ISOnum
    'uArr':     '&#8657;',      # upwards double arrow, U+21D1 ISOamsa
    'uacute':   '\372',         # latin small letter u with acute, U+00FA ISOlat1
    'uarr':     '&#8593;',      # upwards arrow, U+2191 ISOnum
    'ucirc':    '\373',         # latin small letter u with circumflex, U+00FB ISOlat1
    'ugrave':   '\371',         # latin small letter u with grave, U+00F9 ISOlat1
    'uml':      '\250',         # diaeresis = spacing diaeresis, U+00A8 ISOdia
    'upsih':    '&#978;',       # greek upsilon with hook symbol, U+03D2 NEW
    'upsilon':  '&#965;',       # greek small letter upsilon, U+03C5 ISOgrk3
    'uuml':     '\374',         # latin small letter u with diaeresis, U+00FC ISOlat1
    'weierp':   '&#8472;',      # script capital P = power set = Weierstrass p, U+2118 ISOamso
    'xi':       '&#958;',       # greek small letter xi, U+03BE ISOgrk3
    'yacute':   '\375',         # latin small letter y with acute, U+00FD ISOlat1
    'yen':      '\245',         # yen sign = yuan sign, U+00A5 ISOnum
    'yuml':     '\377',         # latin small letter y with diaeresis, U+00FF ISOlat1
    'zeta':     '&#950;',       # greek small letter zeta, U+03B6 ISOgrk3
    'zwj':      '&#8205;',      # zero width joiner, U+200D NEW RFC 2070
    'zwnj':     '&#8204;',      # zero width non-joiner, U+200C NEW RFC 2070

}
