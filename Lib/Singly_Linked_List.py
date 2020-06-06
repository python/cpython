#!/usr/bin/env python3
class Node(object):
	"""docstring for Node"""
	def __init__(self, data):
		self.data = data
		self.next = None

class LinkedList(object):
	"""docstring for LinkedList"""
	def __init__(self):
		self.head = None
		self.length = 0

	def insert(self,node):# insert at end
		if self.head == None:
			self.head = node
			self.length += 1
		else:
			cursor = self.head
			while cursor.next!=None:
				cursor = cursor.next
			cursor.next = node
			self.length += 1

	def delete(self):
		cursor = self.head
		while cursor.next != None:
			second_last_node = cursor # it will have the second last node at the end of the loop
			cursor = cursor.next
		second_last_node.next = None
		del cursor # avoiding memory leaks
		self.length -= 1

	def insert_at_head(self,node):
		temp = self.head
		self.head = node
		self.head.next = temp
		self.length += 1
		del temp # to avoid any memory leaks

	def delete_at_head(self):
		temp = self.head
		self.head = temp.next
		del temp
		self.length -= 1


	def insert_between(self,firstnode,secondnode,node):
		cursor = self.head
		while cursor.data != firstnode.data:
			cursor = cursor.next
		node.next = cursor.next
		cursor.next = node
		self.length += 1

	def insert_between_by_data(self,data1,data2,node):
		cursor = self.head
		while cursor.data != data1:
			cursor = cursor.next
		node.next = cursor.next
		cursor.next = node
		self.length += 1

	def delete_between(self,firstnode,secondnode):
		cursor = self.head
		while cursor.data != firstnode.data:
			cursor = cursor.next
		temp = cursor.next
		cursor.next = temp.next
		temp.next = None
		del temp
		self.length -= 1

	def delete_node_by_data(self,value):
		cursor = self.head
		while cursor.data != value:
			previous_node = cursor
			cursor = cursor.next
		previous_node.next = cursor.next
		cursor.next = None
		del cursor
		self.length -= 1


	def insert_at_index(self,index,node):
		cursor = self.head
		for i in range(index-1):
			cursor = cursor.next
		node.next = cursor.next
		cursor.next = node
		self.length += 1

	def delete_at_index(self,index):
		if index == 0:
			self.delete_at_head()
			return
		if index == self.length - 1:
			self.delete()
			return
		cursor = self.head
		for i in range(index):
			previous_node = cursor
			cursor = cursor.next
		previous_node.next = cursor.next
		cursor.next = None
		del cursor
		self.length -= 1
		

	def show(self):
		cursor = self.head
		if cursor == None:
			print("Empty List")
			return
		while cursor.next != None:
			print(cursor.data,"-->",sep="",end="")
			cursor = cursor.next
		print(cursor.data)
		print("Length of List is %d" %self.length)

	def __len__(self):
		return self.length

	def __iter__(self):
		cursor = self.head
		while cursor != None:
			yield cursor
			cursor = cursor.next

	def __getitem__(self,index):
		# the node will be returned
		if index == 0:
			return self.head

		elif index > 0:
			if index > self.length - 1:
				raise IndexError("Index Out of Bounds")
			cursor = self.head
			for i in range(index):
				cursor = cursor.next
			
			return cursor

		else:
			raise IndexError("Index can't be negative")

	def __setitem__(self,index,data):
		# the node will be returned
		if index == 0:
			self.head.data = data
			return 

		elif index > 0:
			if index > self.length - 1:
				raise IndexError("Index Out of Bounds")
			cursor = self.head
			for i in range(index):
				cursor = cursor.next
			cursor.data = data
			return 

		else:
			raise IndexError("Index can't be negative")
		
		
def main():
	first_node = Node(10)
	l_list = LinkedList()
	l_list.insert(first_node)
	second_node = Node(11)
	l_list.insert(second_node)
	third_node = Node(12)
	l_list.insert(third_node)
	fourth_node = Node(9)
	l_list.insert_at_head(fourth_node)
	#l_list.show()
	#len(l_list)
	fifth_node = Node(10.5)
	l_list.insert_between(first_node,second_node,fifth_node)
	#l_list.show()
	sixth_node = Node(10.75)
	l_list.insert_at_index(3,sixth_node)
	#l_list.show()
	#print(len(l_list))
	#l_list.delete()
	#l_list.show()
	#l_list.delete_at_head()
	#l_list.show()
	#l_list.delete_between(fifth_node,second_node)
	#l_list.show()
	#l_list.delete_node_by_data(11)
	#l_list.show()
	#l_list.delete_at_index(2)
	#l_list.show()
	#l_list.delete_at_index(0)
	#l_list.show()
	#l_list.delete_at_index(5)
	#l_list.show()
	seventh_node = Node(11.5)
	l_list.insert_between_by_data(11,12,seventh_node)
	l_list.show()
	'''
	for x in l_list:
		print(x.data)
		#print(x.next)
		#print(x.previous)
	'''
	#print(l_list[0].data)
	#print(l_list[6].data)
	#print(l_list[7].data) # index out of bounds
	#print(l_list[5].data)
	#print(l_list[-1].data) # index out of bounds
	#l_list[0] = 0
	#l_list[4] = 4
	#l_list[6] = 6
	#l_list[7] = 7 # index error
	#l_list.show()

if __name__ == '__main__':
	main()