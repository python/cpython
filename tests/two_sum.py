# Definition for singly-linked list.
class ListNode:
    def __init__(self, val=0, next=None):
        self.val = val
        self.next = next

class Solution:
    def addTwoNumbers(self, l1: ListNode, l2: ListNode) -> ListNode:
        return addTwoNumbers(l1, l2, 0, None, None)
        
def addTwoNumbers(l1: ListNode, l2: ListNode, carry: int, result: ListNode, tail: ListNode) -> ListNode:
    if l1 is None and l2 is None:
        if carry > 0:
            new_tail = ListNode(carry, None)
            tail.next = new_tail
        return result
    digit1 = l1 and l1.val or 0
    digit2 = l2 and l2.val or 0
    resultDigit = digit1 + digit2 + carry
    carry = 0
    if resultDigit >= 10:
        carry = 1
        resultDigit -= 10
    new_tail = ListNode(resultDigit, None)
    if tail is not None:
        tail.next = new_tail
    if result is None:
        result = new_tail
    return addTwoNumbers(
        l1 and l1.next, 
        l2 and l2.next, 
        carry, 
        result, 
        new_tail
    )

def displayNumber(node):
    if not node:
        return ""
    return displayNumber(node.next) + str(node.val)
  
num1 = ListNode(3, ListNode(2, ListNode(1)))
num2 = ListNode(2, ListNode(2))

print('num1', displayNumber(num1))
print('num2', displayNumber(num2))

s = Solution()
result = s.addTwoNumbers(num1, num2)
print('result', displayNumber(result))