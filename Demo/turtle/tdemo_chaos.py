# Datei: chaosplotter.py
# Autor: Gregor Lingl
# Datum: 31. 5. 2008

# Ein einfaches Programm zur Demonstration von "chaotischem Verhalten".

from turtle import *

def f(x):
    return 3.9*x*(1-x)

def g(x):
    return 3.9*(x-x**2)

def h(x):
    return 3.9*x-3.9*x*x

def coosys():
    penup()
    goto(-1,0)
    pendown()
    goto(n+1,0)
    penup()
    goto(0, -0.1)
    pendown()
    goto(-0.1, 1.1)

def plot(fun, start, farbe):
    x = start
    pencolor(farbe)
    penup()
    goto(0, x)
    pendown()
    dot(5)
    for i in range(n):
        x=fun(x)
        goto(i+1,x)
        dot(5)

def main():
    global n
    n = 80
    ox=-250.0
    oy=-150.0
    ex= -2.0*ox / n
    ey=300.0

    reset()
    setworldcoordinates(-1.0,-0.1, n+1, 1.1)
    speed(0)
    hideturtle()
    coosys()
    plot(f, 0.35, "blue")
    plot(g, 0.35, "green")
    plot(h, 0.35, "red")
    for s in range(100):
        setworldcoordinates(0.5*s,-0.1, n+1, 1.1)

    return "Done!"

if __name__ == "__main__":
    main()
    mainloop()
