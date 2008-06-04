========================================
:mod:`turtle` --- Turtle graphics for Tk
========================================

------------
Introduction
------------

Turtle graphics is a popular way for introducing programming to
kids. It was part of the original Logo programming language developed
by Wally Feurzig and Seymour Papert in 1966.

Imagine a robotic turtle starting at (0, 0) in the x-y plane. Give it
the command turtle.forward(15), and it moves (on-screen!) 15 pixels in
the direction it is facing, drawing a line as it moves. Give it the
command turtle.left(25), and it rotates in-place 25 degrees clockwise.

By combining together these and similar commands, intricate shapes and
pictures can easily be drawn.

The module turtle.py is an extended reimplementation of turtle.py from 
the Python standard distribution up to version Python 2.5. 

It tries to keep the merits of turtle.py and to be (nearly) 100%
compatible with it. This means in the first place to enable the
learning programmer to use all the commands, classes and methods
interactively when using the module from within IDLE run with
the -n switch.

The turtle module provides turtle graphics primitives, in both  
object-oriented and procedure-oriented ways. Because it uses Tkinter 
for the underlying graphics, it needs a version of python installed 
with Tk support. 

The objectoriented interface uses essentially two+two classes:

1. The TurtleScreen class defines graphics windows as a playground for the 
   drawing turtles. It's constructor needs a Tk-Canvas or a ScrolledCanvas
   as argument. It should be used when turtle.py is used as part of some 
   application.
   
   Derived from TurtleScreen is the subclass Screen. Screen is implemented
   as sort of singleton, so there can exist only one instance of Screen at a
   time. It should be used when turtle.py is used as a standalone tool for 
   doing graphics.
   
   All methods of TurtleScreen/Screen also exist as functions, i. e.
   as part of the procedure-oriented interface. 
   
2. RawTurtle (alias: RawPen) defines Turtle-objects which draw on a 
   TurtleScreen. It's constructor needs a Canvas/ScrolledCanvas/Turtlescreen
   as argument, so the RawTurtle objects know where to draw.
   
   Derived from RawTurtle is the subclass Turtle (alias: Pen), which
   draws on "the" Screen - instance which is automatically created,
   if not already present. 
   
   All methods of RawTurtle/Turtle also exist as functions, i. e.
   part of the procedure-oriented interface. 

The procedural interface uses functions which are derived from the methods
of the classes Screen and Turtle. They have the same names as the 
corresponding methods. A screen-object is automativally created
whenever a function derived form a Screen-method is called. An (unnamed)
turtle object is automatically created whenever any of the functions 
derived from a Turtle method is called. 

To use multiple turtles an a screen one has to use the objectoriented
interface.


IMPORTANT NOTE!

In the following documentation the argumentlist for functions is given.
--->> Methods, of course, have the additional first argument self    <<---
--->>                 which is omitted here.                         <<---


--------------------------------------------------
OVERVIEW over available Turtle and Screen methods:
--------------------------------------------------

(A) TURTLE METHODS:
===================

I.  TURTLE MOTION
-----------------

MOVE AND DRAW
      forward | fd
      back | bk | back
      right | rt
      left | lt
      goto | setpos | setposition
      setx
      sety
      setheading | seth
      home
      circle
      dot
      stamp
      clearstamp 
      clearstamps
      undo
      speed
      
TELL TURTLE'S STATE
      position | pos
      towards
      xcor
      ycor
      heading
      distance
      
SETTING AND MEASUREMENT
      degrees
      radians     

II. PEN CONTROL
---------------

DRAWING STATE
      pendown | pd | down
      penup | pu | up
      pensize | width
      pen
      isdown
      
COLOR CONTROL
      color
      pencolor
      fillcolor
      
FILLING
      fill
      begin_fill
      end_fill
      
MORE DRAWING CONTROL
      reset
      clear
      write
            
III. TURTLE STATE
-----------------

VISIBILITY
      showturtle | st
      hideturtle | ht
      isvisible

APPEARANCE
      shape
      resizemode
      shapesize | turtlesize  
      settiltangle
      tiltangle
      tilt
           
IV. USING EVENTS
----------------
      onclick        
      onrelease      
      ondrag              

V. SPECIAL TURTLE METHODS
-------------------------
      begin_poly
      end_poly
      get_poly
      clone
      getturtle | getpen   
      getscreen
      setundobuffer
      undobufferentries
      tracer
      window_width
      window_height
      
..EXCURSUS ABOUT THE USE OF COMPOUND SHAPES 
..-----------------------------------------     

(B) METHODS OF TurtleScreen/Screen
==================================

I. WINDOW CONTROL
-----------------
      bgcolor
      bgpic
      clear | clearscreen
      reset | resetscreen
      screensize
      setworldcoordinates
      
II. ANIMATION CONTROL
---------------------
      delay
      tracer
      update
      
III. USING SCREEN EVENTS
------------------------
      listen
      onkey
      onclick | onscreenclick
      ontimer
      
IV. SETTINGS AND SPECIAL METHODS
--------------------------------
      mode
      colormode
      getcanvas
      getshapes
      register_shape | addshape
      turtles
      window_height
      window_width
      
V. METHODS SPECIFIC TO Screen
=============================
      bye()
      exitonclick()
      setup()
      title()
      
---------------end of OVERVIEW ---------------------------



2. METHODS OF RawTurtle/Turtle AND CORRESPONDING FUNCTIONS
==========================================================                   
      
(I) TURTLE MOTION:        
------------------

(a) --- MOVE (AND DRAW)


    .. method:: forward(distance)
    .. method:: fd(distance)
        distance -- a number (integer or float)

        Move the turtle forward by the specified distance, in the direction
        the turtle is headed.

        Example (for a Turtle instance named turtle)::
          >>> turtle.position()
          (0.00, 0.00)
          >>> turtle.forward(25)
          >>> turtle.position()
          (25.00,0.00)
          >>> turtle.forward(-75)
          >>> turtle.position()
          (-50.00,0.00)    


    .. method:: back(distance)
    .. method:: bk(distance)
    .. method:: backward(distance)
        distance -- a number
        
        call: back(distance)
        --or: bk(distance)
        --or: backward(distance)

        Move the turtle backward by distance ,opposite to the direction the
        turtle is headed. Do not change the turtle's heading.

        Example (for a Turtle instance named turtle)::

          >>> turtle.position()
          (0.00, 0.00)
          >>> turtle.backward(30)
          >>> turtle.position()
          (-30.00, 0.00)
    

    .. method:: right(angle)
    .. method:: rt(angle)
        angle -- a number (integer or float)

        Turn turtle right by angle units. (Units are by default degrees,
        but can be set via the degrees() and radians() functions.)
        Angle orientation depends on mode. (See this.)
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.heading()
          22.0
          >>> turtle.right(45)
          >>> turtle.heading()
          337.0   


    .. method:: left(angle)
    .. method:: lt(angle)
        angle -- a number (integer or float)

        Turn turtle left by angle units. (Units are by default degrees,
        but can be set via the degrees() and radians() functions.)
        Angle orientation depends on mode. (See this.)
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.heading()
          22.0
          >>> turtle.left(45)
          >>> turtle.heading()
          67.0

    .. method:: goto(x, y=None)
    .. method:: setpos(x, y=None)
    .. method:: setposition(x, y=None)
        x -- a number      or     a pair/vector of numbers
        y -- a number             None

        call: goto(x, y)         # two coordinates
        --or: goto((x, y))       # a pair (tuple) of coordinates
        --or: goto(vec)          # e.g. as returned by pos()

        Move turtle to an absolute position. If the pen is down,
        draw line. Do not change the turtle's orientation.

        Example (for a Turtle instance named turtle)::
          >>> tp = turtle.pos()
          >>> tp
          (0.00, 0.00)
          >>> turtle.setpos(60,30)
          >>> turtle.pos()
          (60.00,30.00)
          >>> turtle.setpos((20,80))
          >>> turtle.pos()
          (20.00,80.00)
          >>> turtle.setpos(tp)
          >>> turtle.pos()
          (0.00,0.00)


    .. method:: setx(x)
        x -- a number (integer or float)

        Set the turtle's first coordinate to x, leave second coordinate
        unchanged.

        Example (for a Turtle instance named turtle)::
          >>> turtle.position()
          (0.00, 240.00)
          >>> turtle.setx(10)
          >>> turtle.position()
          (10.00, 240.00)
    
    
    .. method:: sety(y)
        y -- a number (integer or float)

        Set the turtle's first coordinate to x, leave second coordinate
        unchanged.

        Example (for a Turtle instance named turtle)::
          >>> turtle.position()
          (0.00, 40.00)
          >>> turtle.sety(-10)
          >>> turtle.position()
          (0.00, -10.00)

    
    .. method:: setheading(to_angle)
    .. method:: seth(to_angle)
        to_angle -- a number (integer or float)
        
        Set the orientation of the turtle to to_angle.
        Here are some common directions in degrees:
        
        =================== ====================
         standard - mode           logo-mode
        =================== ====================
           0 - east                0 - north
          90 - north              90 - east
         180 - west              180 - south
         270 - south             270 - west
        =================== ====================
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.setheading(90)
          >>> turtle.heading()
          90
    
    
    .. method:: home():
        Move turtle to the origin - coordinates (0,0) and set it's
        heading to it's start-orientation (which depends on mode).
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.home()

    
    .. method:: circle(radius, extent=None, steps=None)
        radius -- a number
        extent (optional) -- a number
        steps (optional) -- an integer
        
        Draw a circle with given radius. The center is radius units left
        of the turtle; extent - an angle - determines which part of the
        circle is drawn. If extent is not given, draw the entire circle.
        If extent is not a full circle, one endpoint of the arc is the
        current pen position. Draw the arc in counterclockwise direction
        if radius is positive, otherwise in clockwise direction. Finally
        the direction of the turtle is changed by the amount of extent.
        
        As the circle is approximated by an inscribed regular polygon,
        steps determines the number of steps to use. If not given,
        it will be calculated automatically. Maybe used to draw regular
        polygons.
        
        call: circle(radius)                  # full circle
        --or: circle(radius, extent)          # arc
        --or: circle(radius, extent, steps)
        --or: circle(radius, steps=6)         # 6-sided polygon

        Example (for a Turtle instance named turtle)::
          >>> turtle.circle(50)
          >>> turtle.circle(120, 180)  # semicircle
    

    .. method:: dot(size=None, *color)
        size -- an integer >= 1 (if given)
        color -- a colorstring or a numeric color tuple

        Draw a circular dot with diameter size, using color. If size
        is not given, the maximum of pensize+4 and 2*pensize is used.

        Example (for a Turtle instance named turtle)::
          >>> turtle.dot()
          >>> turtle.fd(50); turtle.dot(20, "blue"); turtle.fd(50)
    
    
    .. method:: stamp():
        Stamp a copy of the turtle shape onto the canvas at the current
        turtle position. Return a stamp_id for that stamp, which can be 
        used to delete it by calling clearstamp(stamp_id).

        Example (for a Turtle instance named turtle)::
          >>> turtle.color("blue")
          >>> turtle.stamp()
          13
          >>> turtle.fd(50)                

    
    .. method:: clearstamp(stampid):
        stampid - an integer, must be return value of previous stamp() call.
           
        Delete stamp with given stampid

        Example (for a Turtle instance named turtle)::
          >>> turtle.color("blue")
          >>> astamp = turtle.stamp()
          >>> turtle.fd(50)
          >>> turtle.clearstamp(astamp)

    
    .. method:: clearstamps(n=None):
        n -- an integer

        Delete all or first/last n of turtle's stamps.
        If n is None, delete all of pen's stamps,
        else if n > 0 delete first n stamps
        else if n < 0 delete last n stamps.

        Example (for a Turtle instance named turtle)::
          >>> for i in range(8):
          ...     turtle.stamp(); turtle.fd(30)
          >>> turtle.clearstamps(2)
          >>> turtle.clearstamps(-2)
          >>> turtle.clearstamps()

    
    .. method:: undo():
        undo (repeatedly) the last turtle action(s). Number of available 
        undo actions is determined by the size of the undobuffer.

        Example (for a Turtle instance named turtle)::
          >>> for i in range(4):
                  turtle.fd(50); turtle.lt(80)
                
          >>> for i in range(8):
                  turtle.undo()

    
    .. method:: speed(speed=None):
        speed -- an integer in the range 0..10 or a speedstring (see below)
        
        Set the turtle's speed to an integer value in the range 0 .. 10.
        If no argument is given: return current speed.

        If input is a number greater than 10 or smaller than 0.5,
        speed is set to 0.
        Speedstrings are mapped to speedvalues as follows:

           * 'fastest' :  0
           * 'fast'    :  10
           * 'normal'  :  6 
           * 'slow'    :  3
           * 'slowest' :  1

        speeds from 1 to 10 enforce increasingly faster animation of
        line drawing and turtle turning.

        Attention:
        speed = 0 : *no* animation takes place. forward/back makes turtle jump
        and likewise left/right make the turtle turn instantly.

        Example (for a Turtle instance named turtle)::
          >>> turtle.speed(3)
    
    
TELL TURTLE'S STATE
-------------------


    .. method:: position()
    .. method:: pos()
        Return the turtle's current location (x,y) (as a Vec2D-vector)
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.pos()
          (0.00, 240.00)
    

    .. method:: towards(x, y=None)
        x -- a number  or   a pair/vector of numbers   or   a turtle instance
        y -- a number       None                            None 

        call: distance(x, y)         # two coordinates
        --or: distance((x, y))       # a pair (tuple) of coordinates
        --or: distance(vec)          # e.g. as returned by pos()
        --or: distance(mypen)        # where mypen is another turtle

        Return the angle, between the line from turtle-position to position
        specified by x, y and the turtle's start orientation. (Depends on
        modes - "standard"/"world" or "logo")
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.pos()
          (10.00, 10.00)
          >>> turtle.towards(0,0)
          225.0
    
    
    .. method:: xcor()
        Return the turtle's x coordinate
        
        Example (for a Turtle instance named turtle)::
          >>> reset()
          >>> turtle.left(60)
          >>> turtle.forward(100)
          >>> print turtle.xcor()
          50.0
    
    
    .. method:: ycor()
        Return the turtle's y coordinate
        
        Example (for a Turtle instance named turtle)::
          >>> reset()
          >>> turtle.left(60)
          >>> turtle.forward(100)
          >>> print turtle.ycor()
          86.6025403784
    
    
    .. method:: heading()
        Return the turtle's current heading (value depends on mode).
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.left(67)
          >>> turtle.heading()
          67.0
    
    
    .. method:: distance(x, y=None)
        x -- a number   or  a pair/vector of numbers   or   a turtle instance
        y -- a number       None                            None 

        call: distance(x, y)         # two coordinates
        --or: distance((x, y))       # a pair (tuple) of coordinates
        --or: distance(vec)          # e.g. as returned by pos()
        --or: distance(mypen)        # where mypen is another turtle
        
        Return the distance from the turtle to (x,y) in turtle step units.

        Example (for a Turtle instance named turtle)::
          >>> turtle.pos()
          (0.00, 0.00)
          >>> turtle.distance(30,40)
          50.0
          >>> joe = Turtle()
          >>> joe.forward(77)
          >>> turtle.distance(joe)
          77.0
    
    
SETTINGS FOR MEASUREMENT


    .. method:: degrees(fullcircle=360.0)
        fullcircle -  a number 

        Set angle measurement units, i. e. set number
        of 'degrees' for a full circle. Dafault value is
        360 degrees. 
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.left(90)
          >>> turtle.heading()
          90
          >>> turtle.degrees(400.0)  # angle measurement in gon
          >>> turtle.heading()
          100
    
    
    .. method:: radians()
        Set the angle measurement units to radians.
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.heading()   
          90
          >>> turtle.radians()
          >>> turtle.heading()
          1.5707963267948966
    

(II) PEN CONTROL:
-----------------

DRAWING STATE


    .. method:: pendown()
    .. method:: pd()
    .. method:: down()
        Pull the pen down -- drawing when moving.
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.pendown()


    .. method:: penup()
    .. method:: pu()
    .. method:: up()
        Pull the pen up -- no drawing when moving.
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.penup()


    .. method:: pensize(width=None)
    .. method:: width(width=None)
        width -- positive number

        Set the line thickness to width or return it. If resizemode is set
        to "auto" and turtleshape is a polygon, that polygon is drawn with
        the same line thickness. If no argument is given, the current pensize
        is returned.

        Example (for a Turtle instance named turtle)::
          >>> turtle.pensize()
          1
          turtle.pensize(10)   # from here on lines of width 10 are drawn
    
    
    .. method:: pen(pen=None, **pendict)
            pen -- a dictionary with some or all of the below listed keys.
            **pendict -- one or more keyword-arguments with the below
                         listed keys as keywords.

        Return or set the pen's attributes in a 'pen-dictionary'
        with the following key/value pairs:

          * "shown"      :   True/False
          * "pendown"    :   True/False
          * "pencolor"   :   color-string or color-tuple
          * "fillcolor"  :   color-string or color-tuple
          * "pensize"    :   positive number
          * "speed"      :   number in range 0..10
          * "resizemode" :   "auto" or "user" or "noresize"
          * "stretchfactor": (positive number, positive number)
          * "outline"    :   positive number
          * "tilt"       :   number

        This dicionary can be used as argument for a subsequent
        pen()-call to restore the former pen-state. Moreover one
        or more of these attributes can be provided as keyword-arguments.
        This can be used to set several pen attributes in one statement.
                 
        Examples (for a Turtle instance named turtle)::
          >>> turtle.pen(fillcolor="black", pencolor="red", pensize=10)
          >>> turtle.pen()
          {'pensize': 10, 'shown': True, 'resizemode': 'auto', 'outline': 1,
          'pencolor': 'red', 'pendown': True, 'fillcolor': 'black',
          'stretchfactor': (1,1), 'speed': 3}
          >>> penstate=turtle.pen()
          >>> turtle.color("yellow","")
          >>> turtle.penup()
          >>> turtle.pen()
          {'pensize': 10, 'shown': True, 'resizemode': 'auto', 'outline': 1,
          'pencolor': 'yellow', 'pendown': False, 'fillcolor': '',
          'stretchfactor': (1,1), 'speed': 3}
          >>> p.pen(penstate, fillcolor="green")
          >>> p.pen()
          {'pensize': 10, 'shown': True, 'resizemode': 'auto', 'outline': 1,
          'pencolor': 'red', 'pendown': True, 'fillcolor': 'green',
          'stretchfactor': (1,1), 'speed': 3}


    .. method:: isdown(self):
        Return True if pen is down, False if it's up.
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.penup()
          >>> turtle.isdown()
          False
          >>> turtle.pendown()
          >>> turtle.isdown()
          True        
        

COLOR CONTROL


    .. method:: color(*args)
        Return or set pencolor and fillcolor.

        Several input formats are allowed. They use 0, 1, 2, or 3 arguments
        as follows:

        - color()
            Return the current pencolor and the current fillcolor
            as a pair of color specification strings as are returned
            by pencolor and fillcolor.
        - color(colorstring), color((r,g,b)), color(r,g,b)
            inputs as in pencolor, set both, fillcolor and pencolor,
            to the given value.
        - color(colorstring1, colorstring2),
        - color((r1,g1,b1), (r2,g2,b2))
            equivalent to pencolor(colorstring1) and fillcolor(colorstring2)
            and analogously, if the other input format is used.

        If turtleshape is a polygon, outline and interior of that polygon
        is drawn with the newly set colors.
        For more info see: pencolor, fillcolor

        Example (for a Turtle instance named turtle)::
          >>> turtle.color('red', 'green')
          >>> turtle.color()
          ('red', 'green')
          >>> colormode(255)
          >>> color((40, 80, 120), (160, 200, 240))
          >>> color()
          ('#285078', '#a0c8f0')
      

    .. method:: pencolor(*args)
        Return or set the pencolor.

        Four input formats are allowed:
        
        - pencolor()
          Return the current pencolor as color specification string,
          possibly in hex-number format (see example).
          May be used as input to another color/pencolor/fillcolor call.            
        - pencolor(colorstring)
          s is a Tk color specification string, such as "red" or "yellow"
        - pencolor((r, g, b))
          *a tuple* of r, g, and b, which represent, an RGB color,
          and each of r, g, and b are in the range 0..colormode,
          where colormode is either 1.0 or 255
        - pencolor(r, g, b)
          r, g, and b represent an RGB color, and each of r, g, and b
          are in the range 0..colormode

        If turtleshape is a polygon, the outline of that polygon is drawn
        with the newly set pencolor.
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.pencolor('brown')
          >>> tup = (0.2, 0.8, 0.55)
          >>> turtle.pencolor(tup)
          >>> turtle.pencolor()
          '#33cc8c'


    .. method:: fillcolor(*args)
        """ Return or set the fillcolor.

        Four input formats are allowed:
        
        - fillcolor()
          Return the current fillcolor as color specification string,
          possibly in hex-number format (see example).
          May be used as input to another color/pencolor/fillcolor call.            
        - fillcolor(colorstring)
          s is a Tk color specification string, such as "red" or "yellow"
        - fillcolor((r, g, b))
          *a tuple* of r, g, and b, which represent, an RGB color,
          and each of r, g, and b are in the range 0..colormode,
          where colormode is either 1.0 or 255
        - fillcolor(r, g, b)
          r, g, and b represent an RGB color, and each of r, g, and b
          are in the range 0..colormode

        If turtleshape is a polygon, the interior of that polygon is drawn
        with the newly set fillcolor.
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.fillcolor('violet')
          >>> col = turtle.pencolor()
          >>> turtle.fillcolor(col)
          >>> turtle.fillcolor(0, .5, 0)


        See also: Screen method colormode()


FILLING


    .. method:: fill(flag)
        flag -- True/False (or 1/0 respectively)

        Call fill(True) before drawing the shape you want to fill,
        and  fill(False) when done. When used without argument: return 
        fillstate (True if filling, False else).

        Example (for a Turtle instance named turtle)::
          >>> turtle.fill(True)
          >>> for _ in range(3):
          ...    turtle.forward(100)
          ...    turtle.left(120)
          ...
          >>> turtle.fill(False)


    .. method:: begin_fill()
        Called just before drawing a shape to be filled.

        Example (for a Turtle instance named turtle)::
          >>> turtle.color("black", "red")
          >>> turtle.begin_fill()
          >>> turtle.circle(60)
          >>> turtle.end_fill()


    .. method:: end_fill()
        Fill the shape drawn after the call begin_fill().
        
        Example: See begin_fill()


MORE DRAWING CONTROL


    .. method:: reset()
        Delete the turtle's drawings from the screen, re-center the turtle
        and set variables to the default values.      

        Example (for a Turtle instance named turtle)::
          >>> turtle.position()
          (0.00,-22.00)
          >>> turtle.heading()
          100.0
          >>> turtle.reset()
          >>> turtle.position()
          (0.00,0.00)
          >>> turtle.heading()
          0.0


    .. method:: clear()
        Delete the turtle's drawings from the screen. Do not move turtle.
        State and position of the turtle as well as drawings of other
        turtles are not affected.

        Examples (for a Turtle instance named turtle):
          >>> turtle.clear()
    
    
    .. method:: write(arg, move=False, align='left', font=('Arial', 8, 'normal'))
        arg -- info, which is to be written to the TurtleScreen
        move (optional) -- True/False
        align (optional) -- one of the strings "left", "center" or right"
        font (optional) -- a triple (fontname, fontsize, fonttype)

        Write text - the string representation of arg - at the current
        turtle position according to align ("left", "center" or right")
        and with the given font.
        If move is True, the pen is moved to the bottom-right corner
        of the text. By default, move is False.

        Example (for a Turtle instance named turtle)::
          >>> turtle.write('Home = ', True, align="center")
          >>> turtle.write((0,0), True)
    

TURTLE STATE:
-------------

VISIBILITY


    .. method:: showturtle()
    .. method:: st()
        Makes the turtle visible.
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.hideturtle()
          >>> turtle.showturtle()
    
    
    .. method:: hideturtle()
    .. method:: ht()
        Makes the turtle invisible.
        It's a good idea to do this while you're in the middle of  
        doing some complex drawing, because hiding the turtle speeds 
        up the drawing observably.
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.hideturtle()


    .. method:: isvisible(self):
        Return True if the Turtle is shown, False if it's hidden.
        
        Example (for a Turtle instance named turtle)::
          >>> turtle.hideturtle()
          >>> print turtle.isvisible():
          False


APPEARANCE


    .. method:: shape(name=None)
        name -- a string, which is a valid shapename

        Set turtle shape to shape with given name or, if name is not given,
        return name of current shape.
        Shape with name must exist in the TurtleScreen's shape dictionary.
        Initially there are the following polygon shapes:
        'arrow', 'turtle', 'circle', 'square', 'triangle', 'classic'.
        To learn about how to deal with shapes see Screen-method register_shape.

        Example (for a Turtle instance named turtle)::
          >>> turtle.shape()
          'arrow'
          >>> turtle.shape("turtle")
          >>> turtle.shape()
          'turtle'


    .. method:: resizemode(rmode=None)
        rmode -- one of the strings "auto", "user", "noresize"
        
        Set resizemode to one of the values: "auto", "user", "noresize".
        If rmode is not given, return current resizemode.
        Different resizemodes have the following effects:

          - "auto" adapts the appearance of the turtle
                   corresponding to the value of pensize.
          - "user" adapts the appearance of the turtle according to the
                   values of stretchfactor and outlinewidth (outline),
                   which are set by shapesize()
          - "noresize" no adaption of the turtle's appearance takes place.
        
        resizemode("user") is called by a shapesize when used with arguments.

        Examples (for a Turtle instance named turtle)::
          >>> turtle.resizemode("noresize")
          >>> turtle.resizemode()
          'noresize'


    .. method:: shapesize(stretch_wid=None, stretch_len=None, outline=None):    
        stretch_wid -- positive number
        stretch_len -- positive number
        outline -- positive number
        
        Return or set the pen's attributes x/y-stretchfactors and/or outline.
        Set resizemode to "user".
        If and only if resizemode is set to "user", the turtle will be 
        displayed stretched according to its stretchfactors:
        stretch_wid is stretchfactor perpendicular to it's orientation,
        stretch_len is stretchfactor in direction of it's orientation,
        outline determines the width of the shapes's outline.

        Examples (for a Turtle instance named turtle)::
          >>> turtle.resizemode("user")
          >>> turtle.shapesize(5, 5, 12)
          >>> turtle.shapesize(outline=8)
    
    
    .. method:: tilt(angle)
        angle - a number

        Rotate the turtleshape by angle from its current tilt-angle,
        but do NOT change the turtle's heading (direction of movement).
              
        Examples (for a Turtle instance named turtle)::
          >>> turtle.shape("circle")
          >>> turtle.shapesize(5,2)
          >>> turtle.tilt(30)
          >>> turtle.fd(50)
          >>> turtle.tilt(30)
          >>> turtle.fd(50)
      
        
    .. method:: settiltangle(angle)
        angle -- number

        Rotate the turtleshape to point in the direction specified by angle,
        regardless of its current tilt-angle. DO NOT change the turtle's
        heading (direction of movement).
              
        Examples (for a Turtle instance named turtle)::
          >>> turtle.shape("circle")
          >>> turtle.shapesize(5,2)
          >>> turtle.settiltangle(45)
          >>> stamp()
          >>> turtle.fd(50)
          >>> turtle.settiltangle(-45)
          >>> stamp()
          >>> turtle.fd(50)


    .. method:: tiltangle()
        Return the current tilt-angle, i. e. the angle between the
        orientation of the turtleshape and the heading of the turtle
        (it's direction of movement).
                      
        Examples (for a Turtle instance named turtle)::
          >>> turtle.shape("circle")
          >>> turtle.shapesize(5,2)
          >>> turtle.tilt(45)
          >>> turtle.tiltangle()
          45 


IV. USING EVENTS
----------------


    .. method:: onclick(fun, btn=1, add=None)  
        fun --  a function with two arguments, to which will be assigned
                the coordinates of the clicked point on the canvas.
        num --  number of the mouse-button defaults to 1 (left mouse button).
        add --  True or False. If True, new binding will be added, otherwise
                it will replace a former binding.

        Bind fun to mouse-click event on this turtle on canvas.
        If fun is None, existing bindings are removed.
        Example for the anonymous turtle, i. e. the procedural way::

          >>> def turn(x, y):
                  left(360)
                
          >>> onclick(turn) # Now clicking into the turtle will turn it.
          >>> onclick(None)  # event-binding will be removed
    
    
    .. method:: onrelease(fun, btn=1, add=None):
        """
        Arguments:
        fun -- a function with two arguments, to which will be assigned
                the coordinates of the clicked point on the canvas.
        num --  number of the mouse-button defaults to 1 (left mouse button).
        add --  True or False. If True, new binding will be added, otherwise
                it will replace a former binding.

        Bind fun to mouse-button-release event on this turtle on canvas.
        If fun is None, existing bindings are removed.

        Example (for a MyTurtle instance named turtle):
          >>> class MyTurtle(Turtle):
          ...     def glow(self,x,y):
          ...         self.fillcolor("red")
          ...     def unglow(self,x,y):
          ...         self.fillcolor("")
          ...             
          >>> turtle = MyTurtle()
          >>> turtle.onclick(turtle.glow)
          >>> turtle.onrelease(turtle.unglow)
          ### clicking on turtle turns fillcolor red,
          ### unclicking turns it to transparent.
    
    
    .. method:: ondrag(fun, btn=1, add=None):
        fun -- a function with two arguments, to which will be assigned
               the coordinates of the clicked point on the canvas.
        num -- number of the mouse-button defaults to 1 (left mouse button).
        add --  True or False. If True, new binding will be added, otherwise
                it will replace a former binding.

        Bind fun to mouse-move event on this turtle on canvas.
        If fun is None, existing bindings are removed.

        Remark: Every sequence of mouse-move-events on a turtle is preceded 
        by a mouse-click event on that turtle.
        If fun is None, existing bindings are removed.

        Example (for a Turtle instance named turtle):
          >>> turtle.ondrag(turtle.goto)
          ### Subsequently clicking and dragging a Turtle will move it across 
          ### the screen thereby producing handdrawings (if pen is down).
    
    
V. SPECIAL TURTLE METHODS
--------------------------


    .. method:: begin_poly():
        Start recording the vertices of a polygon. Current turtle position
        is first vertex of polygon.

        Example (for a Turtle instance named turtle):
          >>> turtle.begin_poly()


    .. method:: end_poly():
        Stop recording the vertices of a polygon. Current turtle position is
        last vertex of polygon. This will be connected with the first vertex.

        Example (for a Turtle instance named turtle):
          >>> turtle.end_poly()


    .. method:: get_poly():
        Return the lastly recorded polygon.

        Example (for a Turtle instance named turtle):
          >>> p = turtle.get_poly()
          >>> turtle.register_shape("myFavouriteShape", p)


    .. method:: clone():
        Create and return a clone of the turtle with same position, heading
        and turtle properties.

        Example (for a Turtle instance named mick):
        mick = Turtle()
        joe = mick.clone()


    .. method:: getturtle():
        Return the Turtleobject itself.
        Only reasonable use: as a function to return the 'anonymous turtle':
        
        Example:
          >>> pet = getturtle()
          >>> pet.fd(50)
          >>> pet
          <turtle.Turtle object at 0x01417350>
          >>> turtles()
          [<turtle.Turtle object at 0x01417350>]


    .. method:: getscreen():
        Return the TurtleScreen object, the turtle is drawing  on.
        So TurtleScreen-methods can be called for that object.

        Example (for a Turtle instance named turtle):
          >>> ts = turtle.getscreen()
          >>> ts
          <turtle.Screen object at 0x01417710>
          >>> ts.bgcolor("pink")


    .. method:: def setundobuffer(size):
        size -- an integer or None

        Set or disable undobuffer.
        If size is an integer an empty undobuffer of given size is installed.
        Size gives the maximum number of turtle-actions that can be undone
        by the undo() method/function.
        If size is None, no undobuffer is present. 

        Example (for a Turtle instance named turtle):
          >>> turtle.setundobuffer(42)        


    .. method:: undobufferentries():
        """Return count of entries in the undobuffer.

        Example (for a Turtle instance named turtle):
          >>> while undobufferentries():
          ...     undo()


    .. method:: tracer(flag=None, delay=None)
        A replica of the corresponding TurtleScreen-method
        *Deprecated since Python 2.6*  (as RawTurtle method)


    .. method:: window_width()
    .. method:: window_height()
        Both are replicas of the corresponding TurtleScreen-methods
        *Deprecated since Python 2.6*  (as RawTurtle methods)
      

EXCURSUS ABOUT THE USE OF COMPOUND SHAPES
-----------------------------------------

To use compound turtle shapes, which consist of several polygons
of different color, you must use the helper class Shape 
explicitely as described below:

    1. Create an empty Shape object of type compound
    2. Add as many components to this object as desired, 
       using the addcomponent() method:
       
    .. method:: addcomponent(self, poly, fill, outline=None)
        poly -- a polygon
        fill -- a color, the poly will be filled with
        outline -- a color for the poly's outline (if given)
    
So it goes like this::

  >>> s = Shape("compound")
  >>> poly1 = ((0,0),(10,-5),(0,10),(-10,-5))
  >>> s.addcomponent(poly1, "red", "blue")
  >>> poly2 = ((0,0),(10,-5),(-10,-5))
  >>> s.addcomponent(poly2, "blue", "red")

Now add Shape s to the Screen's shapelist ...
.. and use it::

  >>> register_shape("myshape", s)
  >>> shape("myshape")
       

NOTE 1: addcomponent() is a method of class Shape (not of
Turtle nor Screen) and thus there is NO FUNCTION of the same name. 

NOTE 2: class Shape is used internally by the register_shape method
in different ways. 

The application programmer has to deal with the Shape class 
ONLY when using compound shapes like shown above!

NOTE 3: A short description of the class Shape is in section 4.

    
     
3. METHODS OF TurtleScreen/Screen AND CORRESPONDING FUNCTIONS
=============================================================


WINDOW CONTROL
--------------


    .. method:: bgcolor(*args)
        args -- a color string or three numbers in the range 0..colormode 
                or a 3-tuple of such numbers.

        Set or return backgroundcolor of the TurtleScreen.
        
        Example (for a TurtleScreen instance named screen):
          >>> screen.bgcolor("orange")
          >>> screen.bgcolor()
          'orange'
          >>> screen.bgcolor(0.5,0,0.5)
          >>> screen.bgcolor()
          '#800080'        


    .. method:: bgpic(picname=None)
        picname -- a string, name of a gif-file or "nopic".
        
        Set background image or return name of current backgroundimage.
        If picname is a filename, set the corresponing image as background.
        If picname is "nopic", delete backgroundimage, if present. 
        If picname is None, return the filename of the current backgroundimage.
        
        Example (for a TurtleScreen instance named screen):
          >>> screen.bgpic()
          'nopic'
          >>> screen.bgpic("landscape.gif")
          >>> screen.bgpic()
          'landscape.gif'


    .. method:: clear()
    .. method:: clearscreen()
        Delete all drawings and all turtles from the TurtleScreen.
        Reset empty TurtleScreen to it's initial state: white background,
        no backgroundimage, no eventbindings and tracing on.

        Example (for a TurtleScreen instance named screen):
        screen.clear()

        *Note*: this method is only available as the function named
        clearscreen(). (The function clear() is another one derived from
        the Turtle-method clear()!).


    .. method:: reset()
    .. method:: resetscreen()
        Reset all Turtles on the Screen to their initial state.

        Example (for a TurtleScreen instance named screen):
          >>> screen.reset()

        *Note*: this method is pnly available as the function named
        resetscreen(). (The function reset() is another one derived from
        the Turtle-method reset()!).


    .. method:: screensize(canvwidth=None, canvheight=None, bg=None):
        canvwidth -- positive integer, new width of canvas in pixels
        canvheight -- positive integer, new height of canvas in pixels
        bg -- colorstring or color-tupel, new backgroundcolor
        
        If no arguments are given, return current (canvaswidth, canvasheight)
        Resize the canvas, the turtles are drawing on.
        Do not alter the drawing window. To observe hidden parts of
        the canvas use the scrollbars. (So one can make visible those 
        parts of a drawing, which were outside the canvas before!)

        Example (for a Turtle instance named turtle):
          >>> turtle.screensize(2000,1500)
          ### e. g. to search for an erroneously escaped turtle ;-)
        
        
    .. method:: setworldcoordinates(llx, lly, urx, ury):
        llx -- a number, x-coordinate of lower left corner of canvas
        lly -- a number, y-coordinate of lower left corner of canvas
        urx -- a number, x-coordinate of upper right corner of canvas
        ury -- a number, y-coordinate of upper right corner of canvas

        Set up user coodinate-system and switch to mode 'world' if necessary.
        This performs a screen.reset. If mode 'world' is already active,
        all drawings are redrawn according to the new coordinates.

        But *ATTENTION*: in user-defined coordinatesystems angles may appear
        distorted. (see Screen.mode())

        Example (for a TurtleScreen instance named screen):
          >>> screen.reset()
          >>> screen.setworldcoordinates(-50,-7.5,50,7.5)
          >>> for _ in range(72):
          ...     left(10)
          ...
          >>> for _ in range(8):
          ...     left(45); fd(2)   # a regular octogon
           
           
ANIMATION CONTROL
-----------------


    .. method:: delay(delay=None):
        delay -- positive integer
        
        Set or return the drawing delay in milliseconds. (This is sort of
        time interval between two consecutived canvas updates.) The longer 
        the drawing delay, the slower the animation.

        Optional argument:
        Example (for a TurtleScreen instance named screen)::

          >>> screen.delay(15)
          >>> screen.delay()
          15


    .. method:: tracer(n=None, delay=None):
        n -- nonnegative  integer
        delay -- nonnegative  integer
        
        Turn turtle animation on/off and set delay for update drawings.
        If n is given, only each n-th regular screen update is really performed.
        (Can be used to accelerate the drawing of complex graphics.)
        Second argument sets delay value (see delay())

        Example (for a TurtleScreen instance named screen):
          >>> screen.tracer(8, 25)
          >>> dist = 2
          >>> for i in range(200):
          ...     fd(dist)
          ...     rt(90)
          ...     dist += 2
                
                
    .. method:: update():
        Perform a TurtleScreen update. To be used, when tracer is turned
        off. 
        
    See also RawTurtle/Turtle - method speed()
           

USING SCREEN EVENTS
-------------------


    .. method:: listen(xdummy=None, ydummy=None):
        """Set focus on TurtleScreen (in order to collect key-events)
        Dummy arguments are provided in order to be able to pass listen 
        to the onclick method.

        Example (for a TurtleScreen instance named screen):
          >>> screen.listen()        


    .. method:: onkey(fun, key):
        fun -- a function with no arguments or None
        key -- a string: key (e.g. "a") or key-symbol (e.g. "space")

        Bind fun to key-release event of key. If fun is None, event-bindings
        are removed.
        Remark: in order to be able to register key-events, TurtleScreen
        must have focus. (See method listen.)

        Example (for a TurtleScreen instance named screen
        and a Turtle instance named turtle)::

          >>> def f():
          ...     fd(50)
          ...     lt(60)
          ...
          >>> screen.onkey(f, "Up")
          >>> screen.listen()
        
        
    .. method:: onclick(fun, btn=1, add=None):
    .. method:: onscreenclick(fun, btn=1, add=None):
        fun --  a function with two arguments, to which will be assigned
                the coordinates of the clicked point on the canvas - or None.
        num --  number of the mouse-button defaults to 1 (left mouse button).
        add --  True or False. If True, new binding will be added, otherwise
                it will replace a former binding.

        Example (for a TurtleScreen instance named screen and a Turtle instance
        named turtle)::

          >>> screen.onclick(turtle.goto)
          ### Subsequently clicking into the TurtleScreen will
          ### make the turtle move to the clicked point.
          >>> screen.onclick(None)
        
          ### event-binding will be removed

        *Note*: this method is only available as the function named
        onscreenclick(). (The function onclick() is a different one derived 
        from the Turtle-method onclick()!).


    .. method:: ontimer(fun, t=0):
        fun -- a function with no arguments.
        t -- a number >= 0

        Install a timer, which calls fun after t milliseconds.

        Example (for a TurtleScreen instance named screen):

          >>> running = True
          >>> def f():
                  if running:
                      fd(50)
                      lt(60)
                      screen.ontimer(f, 250)
          >>> f()   ### makes the turtle marching around
          >>> running = False           


SETTINGS AND SPECIAL METHODS


    .. method:: mode(mode=None):
        mode -- on of the strings 'standard', 'logo' or 'world'
        
        Set turtle-mode ('standard', 'logo' or 'world') and perform reset.
        If mode is not given, current mode is returned.

        Mode 'standard' is compatible with old turtle.py.
        Mode 'logo' is compatible with most Logo-Turtle-Graphics.
        Mode 'world' uses userdefined 'worldcoordinates'. *Attention*: in
        this mode angles appear distorted if x/y unit-ratio doesn't equal 1.

         ============ ========================= ===================
             Mode      Initial turtle heading     positive angles
         ============ ========================= ===================
          'standard'    to the right (east)       counterclockwise
            'logo'        upward    (north)         clockwise
         ============ ========================= ===================

        Examples::
          >>> mode('logo')   # resets turtle heading to north
          >>> mode()
          'logo'


    .. method:: colormode(cmode=None):
        cmode -- one of the values 1.0 or 255

        """Return the colormode or set it to 1.0 or 255.
        Subsequently r, g, b values of colortriples have to be in 
        range 0..cmode.

        Example (for a TurtleScreen instance named screen):
          >>> screen.colormode()
          1.0
          >>> screen.colormode(255)
          >>> turtle.pencolor(240,160,80)


    .. method:: getcanvas():
        Return the Canvas of this TurtleScreen. Useful for insiders, who
        know what to do with a Tkinter-Canvas ;-)

        Example (for a Screen instance named screen):
          >>> cv = screen.getcanvas()
          >>> cv
          <turtle.ScrolledCanvas instance at 0x010742D8>


    .. method:: getshapes():
        """Return a list of names of all currently available turtle shapes.
        
        Example (for a TurtleScreen instance named screen):
          >>> screen.getshapes()
          ['arrow', 'blank', 'circle', ... , 'turtle']


    .. method:: register_shape(name, shape=None)
    .. method:: addshape(name, shape=None)
        Arguments:
        (1) name is the name of a gif-file and shape is None.
            Installs the corresponding image shape.
            !! Image-shapes DO NOT rotate when turning the turtle,
            !! so they do not display the heading of the turtle!   
        (2) name is an arbitrary string and shape is a tuple
            of pairs of coordinates. Installs the corresponding
            polygon shape
        (3) name is an arbitrary string and shape is a
            (compound) Shape object. Installs the corresponding
            compound shape. (See class Shape.)
        
        Adds a turtle shape to TurtleScreen's shapelist. Only thusly
        registered shapes can be used by issueing the command shape(shapename).

        call: register_shape("turtle.gif")
        --or: register_shape("tri", ((0,0), (10,10), (-10,10)))
        
        Example (for a TurtleScreen instance named screen):
          >>> screen.register_shape("triangle", ((5,-3),(0,5),(-5,-3)))


    .. method:: turtles():
        Return the list of turtles on the screen.

        Example (for a TurtleScreen instance named screen):
          >>> for turtle in screen.turtles()
          ...     turtle.color("red")


    .. method:: window_height():
        Return the height of the turtle window.

        Example (for a TurtleScreen instance named screen):
          >>> screen.window_height()
          480


    .. method:: window_width():
        Return the width of the turtle window.

        Example (for a TurtleScreen instance named screen):
          >>> screen.window_width()
          640
        

METHODS SPECIFIC TO Screen, not inherited from TurtleScreen
-----------------------------------------------------------


    .. method:: bye():
        """Shut the turtlegraphics window.

        This is a method of the Screen-class and not available for
        TurtleScreen instances.

        Example (for a TurtleScreen instance named screen):
          >>> screen.bye()


    .. method:: exitonclick():
        Bind bye() method to mouseclick on TurtleScreen.
        If "using_IDLE" - value in configuration dictionary is False
        (default value), enter mainloop.
        Remark: If IDLE with -n switch (no subprocess) is used, this value 
        should be set to True in turtle.cfg. In this case IDLE's own mainloop
        is active also for the client script.

        This is a method of the Screen-class and not available for
        TurtleScreen instances.

        Example (for a Screen instance named screen):
          >>> screen.exitonclick()


    .. method:: setup(width=_CFG["width"], height=_CFG["height"],
          startx=_CFG["leftright"], starty=_CFG["topbottom"]):
        Set the size and position of the main window. 
        Default values of arguments are stored in the configuration dicionary
        and can be changed via a turtle.cfg file.
        
        width -- as integer a size in pixels, as float a fraction of the screen.
          Default is 50% of screen.
        height -- as integer the height in pixels, as float a fraction of the
          screen. Default is 75% of screen.
        startx -- if positive, starting position in pixels from the left
          edge of the screen, if negative from the right edge
          Default, startx=None is to center window horizontally.
        starty -- if positive, starting position in pixels from the top
          edge of the screen, if negative from the bottom edge
          Default, starty=None is to center window vertically.

        Examples (for a Screen instance named screen)::
        >>> screen.setup (width=200, height=200, startx=0, starty=0)
        # sets window to 200x200 pixels, in upper left of screen

        >>> screen.setup(width=.75, height=0.5, startx=None, starty=None)
        # sets window to 75% of screen by 50% of screen and centers

      
    .. method:: title(titlestring):
        titlestring -- a string, to appear in the titlebar of the
                       turtle graphics window.

        Set title of turtle-window to titlestring

        This is a method of the Screen-class and not available for
        TurtleScreen instances.

        Example (for a Screen instance named screen):
          >>> screen.title("Welcome to the turtle-zoo!")



4. THE PUBLIC CLASSES of the module turtle.py 
=============================================


class RawTurtle(canvas):
    canvas -- a Tkinter-Canvas, a ScrolledCanvas or a TurtleScreen
    
    Alias: RawPen
    
    Define a turtle.
    A description of the methods follows below. All methods are also
    available as functions (to control some anonymous turtle) thus
    providing a procedural interface to turtlegraphics
    
class Turtle()
    Subclass of RawTurtle, has the same interface with the additional
    property, that Turtle instances draw on a default Screen object,
    which is created automatically, when needed for the first time.
    
class TurtleScreen(cv)
    cv -- a Tkinter-Canvas
    Provides screen oriented methods like setbg etc.
    A description of the methods follows below.
    
class Screen()
    Subclass of TurtleScreen, with four methods added.
    All methods are also available as functions to conrtol a unique 
    Screen instance thus belonging to the procedural interface 
    to turtlegraphics. This Screen instance is automatically created
    when needed for the first time.

class ScrolledCavas(master)
    master -- some Tkinter widget to contain the ScrolledCanvas, i.e.
    a Tkinter-canvas with scrollbars added.
    Used by class Screen, which thus provides automatically a 
    ScrolledCanvas as playground for the turtles.

class Shape(type\_, data)
    type --- one of the strings "polygon", "image", "compound"

    Data structure modeling shapes.
    The pair type\_, data must be as follows:
    
         type\_                  data

       "polygon"     a polygon-tuple, i. e. 
                     a tuple of pairs of coordinates
       
       "image"       an image  (in this form only used internally!)
       
       "compound"    None
                     A compund shape has to be constructed using
                     the addcomponent method
                     
    addcomponent(self, poly, fill, outline=None)
        poly -- polygon, i. e. a tuple of pairs of numbers.
        fill -- the fillcolor of the component,
        outline -- the outline color of the component.

        Example:
          >>> poly = ((0,0),(10,-5),(0,10),(-10,-5))
          >>> s = Shape("compound")
          >>> s.addcomponent(poly, "red", "blue")
          ### .. add more components and then use register_shape()
           
     See EXCURSUS ABOUT THE USE OF COMPOUND SHAPES
                         

class Vec2D(x, y):
    A two-dimensional vector class, used as a helper class
    for implementing turtle graphics.
    May be useful for turtle graphics programs also.
    Derived from tuple, so a vector is a tuple!

    Provides (for a, b vectors, k number):

     * a+b vector addition
     * a-b vector subtraction
     * a*b inner product
     * k*a and a*k multiplication with scalar
     * \|a\| absolute value of a
     * a.rotate(angle) rotation      


    
V. HELP AND CONFIGURATION
=========================

This section contains subsections on:

- how to use help
- how to prepare  and use translations of the online-help 
  into other languages
- how to configure the appearance of the graphics window and
  the turtles at startup


HOW TO USE HELP:
----------------

The public methods of the Screen and Turtle classes are documented
extensively via docstrings. So these can be used as online-help
via the Python help facilities:

- When using IDLE, tooltips show the signatures and first lines of
  the docstrings of typed in function-/method calls.

- calling help on methods or functions display the docstrings. 
  Examples::
  
    >>> help(Screen.bgcolor)
    Help on method bgcolor in module turtle:
    
    bgcolor(self, *args) unbound turtle.Screen method
        Set or return backgroundcolor of the TurtleScreen.
        
        Arguments (if given): a color string or three numbers
        in the range 0..colormode or a 3-tuple of such numbers.
        
        Example (for a TurtleScreen instance named screen)::
    
          >>> screen.bgcolor("orange")
          >>> screen.bgcolor()
          'orange'
          >>> screen.bgcolor(0.5,0,0.5)
          >>> screen.bgcolor()
          '#800080'
    
    >>> help(Turtle.penup)
    Help on method penup in module turtle:
    
    penup(self) unbound turtle.Turtle method
        Pull the pen up -- no drawing when moving.
        
        Aliases: penup | pu | up
        
        No argument
        
        Example (for a Turtle instance named turtle):
        >>> turtle.penup()

The docstrings of the functions which are derived from methods have
a modified form::

    >>> help(bgcolor)
    Help on function bgcolor in module turtle:
    
    bgcolor(*args)
        Set or return backgroundcolor of the TurtleScreen.
        
        Arguments (if given): a color string or three numbers
        in the range 0..colormode or a 3-tuple of such numbers.
        
        Example::
    
          >>> bgcolor("orange")
          >>> bgcolor()
          'orange'
          >>> bgcolor(0.5,0,0.5)
          >>> bgcolor()
          '#800080'
    
    >>> help(penup)
    Help on function penup in module turtle:
    
    penup()
        Pull the pen up -- no drawing when moving.
        
        Aliases: penup | pu | up
        
        No argument
        
        Example:
        >>> penup()

These modified docstrings are created automatically together with the
function definitions that are derived from the methods at import time.


TRANSLATION OF DOCSTRINGS INTO DIFFERENT LANGUAGES
--------------------------------------------------

There is a utility to create a dictionary the keys of which are the 
method names and the values of which are the docstrings of the public 
methods of the classes Screen and Turtle.

write_docstringdict(filename="turtle_docstringdict"):
    filename -- a string, used as filename

    Create and write docstring-dictionary to a Python script
    with the given filename.
    This function has to be called explicitely, (it is not used by the 
    turtle-graphics classes). The docstring dictionary will be written 
    to the Python script <filname>.py  It is intended to serve as a 
    template for translation of the docstrings into different languages.

If you (or your students) want to use turtle.py with online help in 
your native language. You have to translate the docstrings and save
the resulting file as e.g. turtle_docstringdict_german.py

If you have an appropriate entry in your turtle.cfg file this dictionary
will be read in at import time and will replace the original English
docstrings.

At the time of this writing there exist docstring_dicts in German
and in Italian. (Requests please to glingl@aon.at)

 
 
HOW TO CONFIGURE SCREEN AND TURTLES
-----------------------------------

The built-in default configuration mimics the appearance and 
behaviour of the old turtle module in order to retain best possible
compatibility with it. 

If you want to use a different configuration which reflects 
better the features of this module or which fits better to
your needs, e. g. for use in a classroom, you can prepare
a configuration file turtle.cfg which will be read at import
time and modify the configuration according to it's settings.

The built in configuration would correspond to the following
turtle.cfg:

width = 0.5
height = 0.75
leftright = None
topbottom = None
canvwidth = 400
canvheight = 300
mode = standard
colormode = 1.0
delay = 10                 
undobuffersize = 1000
shape = classic
pencolor = black
fillcolor = black
resizemode = noresize
visible = True
language = english
exampleturtle = turtle
examplescreen = screen
title = Python Turtle Graphics
using_IDLE = False

Short explanation of selected entries:

- The first four lines correspond to the arguments of the 
  Screen.setup method
- Line 5 and 6 correspond to the arguments of the Method
  Screen.screensize
- shape can be any of the built-in shapes, e.g: arrow, turtle,
  etc. For more info try help(shape)
- if you want to use no fillcolor (i. e. turtle transparent),
  you have to write:
  fillcolor = ""
  (All not empty strings must not have quotes in the cfg-file!)
- if you want to reflect the turtle its state, you have to use
  resizemode = auto
- if you set, e. g.:  language = italian
  the docstringdict turtle_docstringdict_italian.py will be
  loaded at import time (if present on the import path, e.g. in
  the same directory as turtle.py
- the entries exampleturtle  and examplescreen define the names
  of these objects as they occur in the docstrings. The 
  transformation of method-docstrings to function-docstrings 
  will delete these names from the docstrings. (See examples in 
  section on HELP)
- using_IDLE  Set this to True if you regularly work with IDLE
  and it's -n - switch. ("No subprocess") This will prevent 
  exitonclick to enter the mainloop.
  
There can be a turtle.cfg file in the directory where turtle.py
is stored and an additional one in the currentworkingdirectory.
The latter will override the settings of the first one.

The turtledemo directory contains a turtle.cfg file. If you 
study it as an example and see its effects when running the
demos (preferably not from within the demo-viewer). 

      
VI. Demo scripts      
================      
      
There is a set of demo scripts in the turtledemo directory
located  here ... 

           #####  please complete  info about path  ########################
           
It contains:

- a set of 15 demo scripts demonstrating differet features
  of the new module turtle.py
- a Demo-Viewer turtleDemo.py which can be used to view
  the sourcecode of the scripts and run them at the same time
  14 of the examples can be accessed via the Examples Menu.
  All of them can also be run standalone.
- The example  turtledemo_two_canvases.py demonstrates the
  simultaneous use of two canvases with the turtle module.
  Therefor it only can be run standalone.
- There is a turtle.cfg file in this directory, which also
  serves as an example for how to write and use such files.
  
The demoscripts are:

+----------------+------------------------------+-----------------------+
|Name            |           description        |       features        |
+----------------+------------------------------+-----------------------+
|bytedesign      |   complex classical          |     tracer, delay     |
|                |   turtlegraphics pattern     |     update            |
+----------------+------------------------------+-----------------------+
|chaos           |  graphs verhust dynamics,    |    worldcoordinates   |
|                |  proofs that you must not    |                       |
|                |  trust computers computations|                       |
+----------------+------------------------------+-----------------------+
|clock           |   analog clock showing time  |    turtles as clock's |
|                |   of your computer           |    hands, ontimer     |
+----------------+------------------------------+-----------------------+
|colormixer      |     experiment with r, g, b  |         ondrag        |
+----------------+------------------------------+-----------------------+
|fractalcurves   |          Hilbert & Koch      |       recursion       |
+----------------+------------------------------+-----------------------+
|lindenmayer     |        ethnomathematics      |      L-System         |
|                |        (indian kolams)       |                       |
+----------------+------------------------------+-----------------------+
|minimal_hanoi   |      Towers of Hanoi         |   Rectangular Turtles |
|                |                              |   as Hanoi-Discs      |
|                |                              |   (shape, shapesize)  |
+----------------+------------------------------+-----------------------+
|paint           |      super minimalistic      |      onclick          |
|                |      drawing program         |                       |
+----------------+------------------------------+-----------------------+
|peace           |          elementary          |   turtle: appearance  |
|                |                              |   and animation       |      
+----------------+------------------------------+-----------------------+
|penrose         |     aperiodic tiling with    |        stamp          |
|                |         kites and darts      |                       |
+----------------+------------------------------+-----------------------+
|planet_and_moon |     simulation of            |      compound shape   |
|                |     gravitational system     |      Vec2D            |
+----------------+------------------------------+-----------------------+
|tree            | a (graphical) breadth        |          clone        |
|                | first tree (using generators)|                       |
+----------------+------------------------------+-----------------------+
|wikipedia       | a pattern from the wikipedia |       clone, undo     |
|                | article on turtle-graphics   |                       |
+----------------+------------------------------+-----------------------+
|yingyang        |  another elementary example  |         circle        |    
+----------------+------------------------------+-----------------------+

turtledemo_two-canvases:  two distinct Tkinter-Canvases
are populated with turtles. Uses class RawTurtle.


Have fun!