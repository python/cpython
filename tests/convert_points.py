def c3():
  global x1
  global y1
  global x2
  global y2
  global x3
  global y3
  global x4
  global y4
  x1=float(input("What are your x1?"))
  y1=float(input("What are your y1?"))
  x2=float(input("What are your x2?"))
  y2=float(input("What are your y2?"))
  x3=float(input("What are your x3?"))
  y3=float(input("What are your y3?"))
  x4=0
  y4=0
def c4():
  global x1
  global y1
  global x2
  global y2
  global x3
  global y3
  global x4
  global y4
  x1=float(input("What are your x1?"))
  y1=float(input("What are your y1?"))
  x2=float(input("What are your x2?"))
  y2=float(input("What are your y2?"))
  x3=float(input("What are your x3?"))
  y3=float(input("What are your y3?"))
  x4=float(input("What are your x4?"))
  y4=float(input("What are your y4?"))
print("How many coordinates?")
print("3")
print("4")
answer = input("> ")
if answer == "3":
  c3()
elif answer == "4":
  c4()

  
print("Step 1:")
print("1-reflections")
print("2-rotations")
print("3-translations")
answer = input("> ")
if answer == "1":
  i = input("across what?")
  if i == "y":
    x1*-1
    x2*-1
    x3*-1
    x4*-1
  elif i == "x":
    y1*-1
    y2*-1
    y3*-1
    y4*-1
if answer == "2":
  i = input("which way?")
  if answer == "90 cw":
    x1, y1 = y1, x1
    x1*-1
    x2, y2 = y2, x2
    x2*-1
    x3, y3 = y3, x3
    x3*-1
    x4, y4 = y4, x4
    x4*-1
  if answer == "90 ccw":
    x1, y1 = y1, x1
    y1*-1
    x2, y2 = y2, x2
    y2*-1
    x3, y3 = y3, x3
    y3*-1
    x4, y4 = y4, x4
    y4*-1
if answer == "3":
  i = float(input("how many units right?"))
  i1 = float(input("how many units up?"))
  x1+i
  x2+i
  x3+i
  x4+i
  y1+i1
  y2+i1
  y3+i1
  y4+i1
print("Step 2:")
print("1-reflections")
print("2-rotations")
print("3-translations")
answer = input("> ")
if answer == "1":
  i = input("across what?")
  if i == "y":
    x1*-1
    x2*-1
    x3*-1
    x4*-1
  elif i == "x":
    y1*-1
    y2*-1
    y3*-1
    y4*-1
if answer == "2":
  i = input("which way?")
  if answer == "90 cw":
    x1, y1 = y1, x1
    x1*-1
    x2, y2 = y2, x2
    x2*-1
    x3, y3 = y3, x3
    x3*-1
    x4, y4 = y4, x4
    x4*-1
  if answer == "90 ccw":
    x1, y1 = y1, x1
    y1*-1
    x2, y2 = y2, x2
    y2*-1
    x3, y3 = y3, x3
    y3*-1
    x4, y4 = y4, x4
    y4*-1
if answer == "3":
  i = float(input("how many units right?"))
  i1 = float(input("how many units up?"))
  x1+i
  x2+i
  x3+i
  x4+i
  y1+i1
  y2+i1
  y3+i1
  y4+i1
print("(",x1,",",y1,")")
print("(",x2,",",y2,")")
print("(",x3,",",y3,")")
print("(",x4,",",y4,")")