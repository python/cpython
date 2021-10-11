def selection_sort(List):

  for i in range(len(List)-1):

    m = min(List[i:])
    k = List.index(m)
    
    List[i], List[k] = List[k], List[i]
    

List = [9,3,1,6,10,2,5,20,4]
selection_sort(List)
print(List)
