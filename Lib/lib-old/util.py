# Module 'util' -- some useful functions that dont fit elsewhere

# Remove an item from a list at most once
#
def remove(item, list):
	for i in range(len(list)):
		if list[i] = item:
			del list[i]
			break
