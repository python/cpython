# File: Lib/flexsearch.py

def parse_numbers(s):
	"""
	Parse a string of numbers separated by spaces or commas.
	"""
	return [int(x) for x in s.replace(',', ' ').split()]

def sort_numbers(arr, mode):
	"""
	Sort array based on mode:
	a    -> ascending
	d    -> descending
	min  -> min to max
	max  -> max to min
	"""
	if mode in ('a', 'min'):
		arr.sort()
	elif mode in ('d', 'max'):
		arr.sort(reverse=True)
	else:
		raise ValueError(f"Unknown sort mode: {mode}")
	return arr

def binary_search(arr, target):
	"""
	Perform binary search on arr (auto-detect ascending/descending).
	Returns index of target, or -1 if not found.
	"""
	if not arr:
		return -1
	asc = arr[0] < arr[-1]
	l, r = 0, len(arr) - 1
	while l <= r:
		m = (l + r) // 2
		if arr[m] == target:
			return m
		if (asc and arr[m] < target) or (not asc and arr[m] > target):
			l = m + 1
		else:
			r = m - 1
	return -1

if __name__ == "__main__":
	# User input
	numbers = input("Enter numbers separated by space or comma: ")
	mode = input("Enter sort mode (a(ascending)/d(descending)/min/max): ").strip()
	target_str = input("Enter target number for search: ").strip()

	# Convert target to int
	try:
		target = int(target_str)
	except ValueError:
		print("Invalid target number. Exiting.")
		exit(1)

	arr = parse_numbers(numbers)
	sorted_arr = sort_numbers(arr, mode)
	idx = binary_search(sorted_arr, target)

	print("\nInput string:", numbers)
	print("Sorted array:", sorted_arr)
	print(f"Target {target} found at index:", idx)
