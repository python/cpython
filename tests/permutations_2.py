class Solution:
    def permuteUnique(self, nums):
        return permute_unique([], sorted(nums))
        

def permute_unique(so_far, rest):
    if len(rest) == 0:
        return [so_far]
    permutations = []
    last_num = None
    for i, num in enumerate(rest):
        if last_num != num:
            new_so_far = so_far + [num]
            new_rest = rest.copy()
            del new_rest[i]
            result = permute_unique(new_so_far, new_rest)
            permutations.extend(result)
        last_num = num
    return permutations
      
s = Solution()
result = s.permuteUnique([1,2,3])
print(result)
result2 = s.permuteUnique([1,2,2,3])
print(result2)