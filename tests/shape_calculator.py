import math
def error():
  print("Input is not valid, please try again")
def move_on():
  input("Press ENTER to reveal next step.")

def volume_or_suface_area():
  print("What do you want to calculate?")
  print("press 1- volume")
  print("press 2- surface area")
  print("press 3- go back")
  answer = input("> ")
  if answer == "1":
    do_3D_shapes_v()
  elif answer == "2":
    do_3D_shapes_s()
  elif answer == "3":
    dimensions()
  else:
    error()
    volume_or_suface_area()
def area_or_perimeter():
  print("What do you want to calculate?")
  print("press 1- area")
  print("press 2- perimeter")
  # print("press 3- go back")
  answer = input("> ")
  if answer == "1":
    do_2D_shapes_a()
  elif answer == "2":
    do_2D_shapes_p()
  # elif answer == "3":
  #   dimensions()
  else:
    error()
    area_or_perimeter()
def rectangle():
  print("This is what a rectangle looks like:")
  print("""
+--------------+
|              | 
|              | 
|              | 
|              | 
+--------------+
""")
  print("The formula of a rectangle is area = lw")
  print("l = length, w = width")
  x = float(input("What is your rectangle's length? "))
  y = float(input("What is your rectangle's width? "))
  a = x*y
  print("if you did it correctly, you should have this:",x,"*",y)
  move_on()
  print("you should have this as your final answer:",a)
def rhombus():
  print("area = pq/2")
  print("you see, rhombus is basically a diamond. you can draw a line vertically and horizonatally. p is the vertical one. Q is the horizonatal one.")
  x = float(input("What the the value of p?"))
  y = float(input("What is the value of q?"))
  a = x*y/2
  print("you should have this:",x,"*",y,"/2")
  move_on()
  print("multiply p and q. You should have this:",x*y,"/2")
  move_on()
  print("your final answer should be",a)
def Quadrilateral():
  print("press 1- trapezoid")
  print("press 2- rectangle")
  print("press 3- square")
  print("press 4- rhombus")
  print("press 5- kite")
  print("press 6- go back")
  answer = input("> ")
  if answer == "1":
    trapezoid()
  elif answer == "2":
    rectangle()
  elif answer == "3":
    square()
  elif answer == "4":
    rhombus()
  elif answer == "5":
    kite()
  elif answer == "6":
    do_2D_shapes_a()
  else:
    error()
    Quadrilateral()
def Quadrilateral_p():
  print("press 1- trapezoid")
  print("press 2- rectangle")
  print("press 3- square")
  print("press 4- rhombus")
  print("press 5- kite")
  print("press 6- go back")
  answer = input("> ")
  if answer == "1":
    trapezoid_p()
  elif answer == "2":
    rectangle_p()
  elif answer == "3":
    square_p()
  elif answer == "4":
    rhombus_p()
  elif answer == "5":
    kite_p()
  elif answer == "6":
    do_2D_shapes_p()
  else:
    error()
    Quadrilateral_p()
def rectangle_p():
  print("formula is this: perimeter = 2(a+b)")
  print("a and b are sides")
  a = float(input("What is a?"))
  b = float(input("What is b?"))
  print("add a and b:",a+b,"*2")
  move_on()
  print("multiply by 2:",(a+b)/2)
def square_p():
  print("formula is perimeter = 4a")
  print("a is the side")
  a = float(input("What is a?"))
  print("multiply 4 and a and your final answer is",4*a)
def rhommbus_p():
  print("formula is perimeter = 4a")
  print("a is the side")
  a = float(input("What is a?"))
  print("multiply 4 and a and your final answer is",4*a)
def trapezoid_p():
  print("formula is perimeter = a + b + c + d")
  print("a,b,c,d are all sides")
  a = float(input("What is a?"))
  b = float(input("What is b?"))
  c = float(input("What is c?"))
  d = float(input("What is d?"))
  print("add a,b,c,d and you get",a+b+c+d)
def kite_p():
  print("formula is perimeter = 2(a+b)")
  print("a and b are the different sides")
  x = float(input("What is a?"))
  y = float(input("What is b"))
  print("add a and b:",a+b,"*2")
  move_on()
  print("multiply by two and your final answer should be",(a+b)/2)
def kite():
  print("Like the rhombus, The kite has p and q.")
  print("p is the horizonatal line across the kite. q is the vertical line.")
  x = float(input("What is p?"))
  y = float(input("What is q?"))
  a = x*y/2
  print("if you plugged the numbers in correctly, you should have this:",x,"*",y,"/2")
  move_on()
  print("multiply x and y:",x*y,"/2")
  move_on()
  print("divide  by 2 and your final answer is",a)
def trapezoid():
  print("the formula of a trapezoid is this:")
  print("(b₁+b₂)h/2")
  print("b₁ = base 1(one of the two parellel lines), b₂ is the other parallel line, and h = height")
  x = float(input("What is base 1?" ))
  y = float(input("What is base 2?" ))
  h = float(input("What is the height?" ))
  move_on()
  print("if you did it correctly, you should have this:""(",x,"+",y,")""*",h,"/2")
  move_on()
  print("add the bases and you get this:",x+y,"*",h,"/2")
  move_on()
  print("multiply the added bases and the height so you should have",(x+y)*h,"/2")
  move_on()
  print("Finally, you divide by 2 which is ",(x+y)*h/2)
def square():
  print("This is what a square looks like:")
  print("""
+--------+
|        |
|        |
|        |
+--------+
""")
  print("the formula of a square is this:")
  print("area = x²")
  print("x = length or width. They have the same length.")
  x = float(input("What the length of your square? "))
  print("So it is this:",x,"²")
  a = pow(x,2)
  print("Which is",a)
def Triangle():
  print("press 1- right triangle")
  print("press 2- equalateral triangle")
  print("press 3- isosceles triangle")
  print("press 4- scalene triangle")
  print("press 5- go back")
  answer = input("> ")
  if answer == "1":
    right_triangle()
def right_triangle():
  print("formula is area = a*b/2")
  print("a and b are the opposite and adjacent")
  a = float(input("What is a?"))
  b = float(input("What is b?"))
  print("multiply a and b:",a*b,"/2")
  move_on()
  print("divide by 2:",a*b/2)
def polygon():
  print("press 1- pentagon")
  print("press 2- hexagon")
  # print("press 3- heptagon")
  print("press 3- octagon")
  print("press 4- go back")
  # print("press 5- nonagon")
  # print("press 6- decagon")
  # print("press 7- dodecagon")
  # print("press 8- hendecagon")
  # print("press 9- hexadecagon")
  # print("press 10- icosagon")
  answer = input("> ")
  if answer == "1":
    pentagon()
  elif answer == "2":
    hexagon()
  elif answer == "3":
    octagon()
  elif answer == "4":
    do_2D_shapes_a()
def polygon_p():
  print("press 1- pentagon")
  print("press 2- hexagon")
  # print("press 3- heptagon")
  print("press 3- octagon")
  print("press 4- go back")
  # print("press 5- nonagon")
  # print("press 6- decagon")
  # print("press 7- dodecagon")
  # print("press 8- hendecagon")
  # print("press 9- hexadecagon")
  # print("press 10- icosagon")
  answer = input("> ")
  if answer == "1":
    pentagon_p()
  elif answer == "2":
    hexagon_p()
  elif answer == "3":
    octagon_p()
  elif answer == "4":
    do_2D_shapes_p()
def pentagon_p():
  print("formula is perimeter = 5a")
  print("a is the side")
  a = float(input("What is a?"))
  print("multiply 5 and a and your final answer should be",5*a)
def hexagon_p():
  print("formula is perimeter = 6a")
  print("a is the side")
  a = float(input("What is a?"))
  print("multiply 6 and a and your final answer should be",6*a)
def octagon_p():
  print("formula is perimeter = 8a")
  print("a is the side")
  a = float(input("What is a?"))
  print("multiply 8 and a and your final answer should be",8*a)
def octagon():
  print("the formula for an octagon is area = 2(1+√2)a²")
  print("a is the length of a side")
  x = float(input("What is the length of an your octagon's side?"))
  a = 2*(1 + pow(2,1/2))*pow(x,2)
  print("if you plugged the numbers correctly, you should have 2(1+√2)",x,"²")
  move_on()
  print("square your number: 2(1+√2)",pow(x,2))
  move_on()
  print("distribute 1 and √2: 2+2√2*",pow(x,2))
  move_on()
  print("multiply",pow(x,2),"and 2: 2+",2*pow(x,2),"√2")
  move_on()
  print("if you want a rounded answer, multiply",2*pow(x,2),"and √2: 2+",2*pow(x,2)*pow(2,1/2))
  move_on()
  print("add 2 and your final answer should be about",a)
def pentagon():
  print("The formula of a pentagon is area = 1/2 * perimeter * apothem")
  print("""
        /\\
      /    \\
    /      / \\
  /       /    \\
  \\            /
   \\          /
    \+------+/
  """)
  print("the line that goes from the center to the center of a side is called the apothem")
  print("to find the perimeter, you multiply one side by 5(since they are all equal)")
  x = float(input("What is the length of one line?(if you are given the perimeter, just divide the perimeter by 5)"))
  y = float(input("What is the length of the apothem?"))
  a = x*5*y/2
  print("if you pluged the numbers in correctly, you should have",x,"* 5 *",y,"/2")
  move_on()
  print("multiply x and 5 to get the perimeter",x*5,"*",y,"/2")
  move_on()
  print("multiply by y and you get this:",x*y*5,"/2")
  move_on()
  print("divide by 2 and your final answer is",a)
def hexagon():
  print("area = ((3√3)a²)/2")
  print("a is the side")
  x = float(input("what is a?"))
  a = 3 * pow(1/2,3)
  print("plug the number in: ((3√3)",a,"²)/2")
def do_2D_shapes_a():
  # print("What 2D shapes do you want to calculate?")
  print("What kind of shapes do you want to calculate?")
  print("press 1- Quadrilateral")
  print("press 2- right triangle")
  print("press 3- polygon(not Quadrilateral)")
  # print("press 4- pentagram")
  # print("press 5- heptagram")
  # print("press 6- octagram")
  # print("press 7- decagram")
  print("press 5- circle")
  print("press 6- go back")
  answer = input("> ")
  if answer == "1":
    Quadrilateral()
  elif answer == "2":
    right_triangle()
  elif answer == "3":
    polygon()
  # elif answer == "4":
  #   pentagram()
  # elif answer == "5":
  #   heptagram()
  # elif answer == "6":
  #   octagram()
  # elif answer == "7":
  #   decagram()
  elif answer == "5":
    circle()
  elif answer == "6":
    area_or_perimeter()
  else:
    error()
    do_2D_shapes_a()
def circle():
  print("This is what a circle looks like:")
  print("""
        o  o
     o        o
    o          o
    o          o
     o        o
        o  o
""")
  print("The formula of a circle is this:")
  print("area = πr²")
  print("r is radius")
  x = float(input("What is your circle's radius?"))
  a = pow(x,2)*math.pi
  move_on()
  print("your final answer should be",a)
def do_2D_shapes_p():
  print("What 2D shapes do you want to calculate?")
  print("press 1- quadrilateral")
  print("press 2- right triangle")
  print("press 3- polygon(not Quadrilateral)")
  print("press 4- circle")
  # print("press 5- pentagram")
  # print("press 6- heptagram")
  # print("press 7- octagram")
  # print("press 8- decagram")
  print("press 5- go back")
  answer = input("> ")
  if answer == "1":
    Quadrilateral_p()
  elif answer == "2":
    right_triangle_p()
  elif answer == "3":
    polygon_p()
  # elif answer == "4":
  #   pentagram_p()
  # elif answer == "5":
  #   heptagram_p()
  # elif answer == "6":
  #   octagram_p()
  # elif answer == "7":
  #   decagram_p()
  elif answer == "4":
    circle_p()
  elif answer == "5":
    area_or_perimeter()
  else:
    error()
    do_2D_shapes_p()
def right_triangle_p():
  print("formula is perimeter = a + b + √a²+b²")
  print("√a²+b² is the hypotenuse because pythagorean theorem. a and b are the legs.")
  a = float(input("what is a?"))
  b = float(input("what is b?"))
  print("add a and b",a+b,"+ √a²+b²")
  move_on()
  print("square a and b",a+b,"+ √",pow(a,2),"+",pow(b,2))
  move_on()
  print("add the squares:",a+b,"+ √",pow(a,2)+pow(b,2))
  move_on()
  print("root of a² and b²:",a+b,"+",pow(pow(a,2)+pow(b,2),1/2))
  move_on()
  print("add the legs and the hypotenuse:",a+b+pow(pow(a,2)+pow(b,2),1/2))
def circle_p():
  print("formula is circumference = 2πr")
  x = float(input("What is your radius?"))
  print("multiply 2 and your radius:",2*x,"π")
  print("if you want to convert your circle's circumference into an estimated amount, multiply π and",2*x,"and your final answer is:",2*x*math.pi)
def do_3D_shapes_s():
  print("What 3D shapes do you want to calculate?")
  print("press 1- Cube")
  print("press 2- Rectanglar Prism")
def do_3D_shapes_v():
  print("What 3D shapes do you want to calculate?")
  print("press 1- Cube")
  print("press 2- Rectanglar Prism")
# def dimensions():
#   print("What dimension do you want to calculate?")
#   print("press 1- 2D")
#   print("press 2- 3D")
#   answer = input("> ")
#   if answer == "1":
#     area_or_perimeter()
#   elif answer == "2":
#     volume_or_suface_area()
print("""
  _      __        __                                   
 | | /| / / ___   / / ____ ___   __ _  ___              
 | |/ |/ / / -_) / / / __// _ \ /  ' \/ -_)             
 |__/|__/  \__/ /_/  \__/ \___//_/_/_/\__/              
                                                        
  __                                                    
 / /_ ___                                               
/ __// _ \                                              
\__/ \___/                                              
                                                        
   ____   __                                            
  / __/  / /  ___ _   ___  ___                          
 _\ \   / _ \/ _ `/  / _ \/ -_)                         
/___/  /_//_/\_,_/  / .__/\__/                          
                   /_/                                  
  _____         __              __        __            
 / ___/ ___ _  / / ____ __ __  / / ___ _ / /_ ___   ____
/ /__  / _ `/ / / / __// // / / / / _ `// __// _ \ / __/
\___/  \_,_/ /_/  \__/ \_,_/ /_/  \_,_/ \__/ \___//_/   
                                                                                    
""")
#dimensions()
area_or_perimeter()
while True:
  input("Type ENTER to restart.")
  # dimensions()
  area_or_perimeter()