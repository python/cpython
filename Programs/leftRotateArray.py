#Initialize array     
arr = [1, 2, 3, 4, 5];     
#n determine the number of times an array should be rotated    
n = 3;    
     
#Displays original array    
print("Original array: ");    
for i in range(0, len(arr)):    
    print(arr[i]),     
     
#Rotate the given array by n times toward left    
for i in range(0, n):    
    #Stores the first element of the array    
    first = arr[0];    
        
    for j in range(0, len(arr)-1):    
        #Shift element of array by one    
        arr[j] = arr[j+1];    
            
    #First element of array will be added to the end    
    arr[len(arr)-1] = first;    
     
print();    
     
#Displays resulting array after rotation    
print("Array after left rotation: ");    
for i in range(0, len(arr)):    
    print(arr[i]),    
