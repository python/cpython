import zlib
import sys

buf = open(sys.argv[0]).read() * 8

x = zlib.compress(buf)
y = zlib.decompress(x)
if buf != y:
    print "normal compression/decompression failed"

buf = buf * 16

co = zlib.compressobj(8, 8, -15)
x1 = co.compress(buf)
x2 = co.flush()
x = x1 + x2

dc = zlib.decompressobj(-15)
y1 = dc.decompress(x)
y2 = dc.flush()
y = y1 + y2
if buf != y:
    print "compress/decompression obj failed"

def ignore():
    """An empty function with a big string.

    Make the compression algorithm work a little harder.
    """

    """
LAERTES 

       O, fear me not.
       I stay too long: but here my father comes.

       Enter POLONIUS

       A double blessing is a double grace,
       Occasion smiles upon a second leave.

LORD POLONIUS 

       Yet here, Laertes! aboard, aboard, for shame!
       The wind sits in the shoulder of your sail,
       And you are stay'd for. There; my blessing with thee!
       And these few precepts in thy memory
       See thou character. Give thy thoughts no tongue,
       Nor any unproportioned thought his act.
       Be thou familiar, but by no means vulgar.
       Those friends thou hast, and their adoption tried,
       Grapple them to thy soul with hoops of steel;
       But do not dull thy palm with entertainment
       Of each new-hatch'd, unfledged comrade. Beware
       Of entrance to a quarrel, but being in,
       Bear't that the opposed may beware of thee.
       Give every man thy ear, but few thy voice;
       Take each man's censure, but reserve thy judgment.
       Costly thy habit as thy purse can buy,
       But not express'd in fancy; rich, not gaudy;
       For the apparel oft proclaims the man,
       And they in France of the best rank and station
       Are of a most select and generous chief in that.
       Neither a borrower nor a lender be;
       For loan oft loses both itself and friend,
       And borrowing dulls the edge of husbandry.
       This above all: to thine ownself be true,
       And it must follow, as the night the day,
       Thou canst not then be false to any man.
       Farewell: my blessing season this in thee!

LAERTES 

       Most humbly do I take my leave, my lord.

LORD POLONIUS 

       The time invites you; go; your servants tend.

LAERTES 

       Farewell, Ophelia; and remember well
       What I have said to you.

OPHELIA 

       'Tis in my memory lock'd,
       And you yourself shall keep the key of it.

LAERTES 

       Farewell.
"""

