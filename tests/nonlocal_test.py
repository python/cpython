def outer():
    name = "Bobby"
    age = 1
    gender = "female"

    def inner():
        nonlocal name
        age = 3
        name = "Jerry"
        print("gender", gender)
        def inner_inner():
            nonlocal name
            nonlocal age
            name = "Jennifer"
            age = 4
        
        inner_inner()
        print(name, age)

    inner()

    print(name)

    name = "James"

    print(name)

outer()