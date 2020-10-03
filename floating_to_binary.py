# Python program to convert float decimal to binary number 
# Function returns octal representation 
def float_bin(number, places = 3): 
  
    # split() seperates whole number and decimal part and stores it in two seperate variables  
    whole, dec = str(number).split(".") 
 
    whole = int(whole) 
    dec = int (dec) 
  
    # Convert the whole number part to it's respective binary form and remove the "0b" from it.  
    res = bin(whole).lstrip("0b") + "."
  
    # Iterate the number of times, we want the number of decimal places to be 
    for x in range(places): 
        whole, dec = str((decimal_converter(dec)) * 2).split(".") 
        dec = int(dec) 
        res += whole   
    return res 
  
def decimal_converter(num):  
    while num > 1: 
        num /= 10
    return num 
  
# Driver Code 
n = input("Enter your floating point value : \n") 
  
p = int(input("Enter the number of decimal places of the result : \n")) 
  
print(float_bin(n, places = p)) 
