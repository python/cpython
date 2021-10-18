n = int(input("Enter a number: "))
for i in range(1, n+1):
    count = 0
    for j in range(1, n+1):
        if i%j==0:
            count= count+1
    if count==2:
        print(i)
