# Definig a class food, 
# which contain name and price of the food item

class Food(object):
    def __init__(self, name, price):
        self.name = name
        self.price = price
    
    def getprice(self):
        return self.price
    
    def __str__(self):
        return self.name + ' : ' + str(self.getprice())
  
# Defining a function for building a Menu 
# which generates list of Food    
def buildmenu(names, costs):
    menu = []
    for i in range(len(names)):
        menu.append(Food(names[i], costs[i]))
    return menu

# items
names = ['Coffee', 'Tea', 'Pizza', 'Burger', 'Fries', 'Chicken', 'Donut', 'Cake']

# prices
costs = [250, 150, 180, 70, 65, 150, 120, 350]

# building food menu
Foods = buildmenu(names, costs)

n = 1
for el in Foods:
    print(n,'. ', el)
    n = n + 1
