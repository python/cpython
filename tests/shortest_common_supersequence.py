class Solution:
    def shortestCommonSupersequence(self, str1: str, str2: str) -> str:
        table = []
        for i in range(len(str2) + 1):
            table.append([None] * (len(str1) + 1))
        
        table[len(str2)][len(str1)] = ("", None, 0)
        
        for i, char1 in enumerate(str1):
            table[len(str2)][i] = (char1, (len(str2), i + 1), len(str1) - i)
        
        for j, char2 in enumerate(str2):
            table[j][len(str1)] = (char2, (j + 1, len(str1)), len(str2) - j)
        
        #print(table)
        for i in range(len(str1) - 1, -1, -1):
            char1 = str1[i]
            #print("%d[%s]" % (i, char1), end=": ")
            for j in range(len(str2) - 1, -1, -1):
                char2 = str2[j]
                #print("%d[%s]" % (j, char2), end=", ")
                if char1 == char2:
                    child_entry = table[j + 1][i + 1]
                    table[j][i] = (char1, (j + 1, i + 1), child_entry[2] + 1)
                else:
                    child_entry_1 = table[j + 1][i]
                    child_entry_2 = table[j][i + 1]
                    if child_entry_1[2] < child_entry_2[2]:
                        table[j][i] = (char2, (j + 1, i), child_entry_1[2] + 1)
                    else:
                        table[j][i] = (char1, (j, i + 1), child_entry_2[2] + 1)
                    
        output = ""
        
        entry = table[0][0]
        while entry[1]:
            output += entry[0]
            entry = table[entry[1][0]][entry[1][1]]

        #print(table)     
        return output

s = Solution()
result = s.shortestCommonSupersequence('abac', 'cab')
print('result', result)