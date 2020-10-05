import math
import matplotlib.pyplot as plt

y = 1
y_list = []
h = 0.05
x = 0
x_list = []

actual_y = []
error = []

for i in range(0,100):
    y = y + h * y
    x = x + h
    e = (math.exp(x) - y)

    error.append(e)
    y_list.append(y)
    x_list.append(x)
    actual_y.append(math.exp(x))

for i in range(len(y_list)):
    print(f' {i} :  {x_list[i]} \t\t\t\t\t {y_list[i]}\t\t\t\t \t\t  {actual_y[i]}t\t\t \t\t{error[i]} ')

plt.plot(x_list, y_list)
plt.plot(x_list, actual_y)
plt.title("h = " + str(h))


plt.show()

