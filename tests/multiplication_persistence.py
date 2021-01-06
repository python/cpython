# From https://www.youtube.com/watch?v=Wim9WJeDTHQ&t=549s

def per(n):
    if len(str(n)) == 1:
        print(n)
        return "DONE"
    
    digits = [int(i) for i in str(n)]

    result = 1
    for j in digits:
        result *= j
    print(result)
    per(result)

per(277777788888899)