# File: Programs/samplebinarysearch.py

import sys
import os

# Add Lib/ folder to path dynamically (so import works)
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'Lib'))
import flexsearch

def main():
	while True:
		# User input
		numbers = input("\nEnter numbers separated by space or comma (or '-1' to quit): ").strip()
		if numbers.lower() == '-1':
			break
		if not numbers:
			print("No numbers entered. Try again.")
			continue

		mode = input("Enter sort mode (a/d/min/max): ").strip().lower()
		if mode not in ('a', 'd', 'min', 'max'):
			print("Invalid sort mode. Try again.")
			continue

		target_str = input("Enter target number for search: ").strip()
		try:
			target = int(target_str)
		except ValueError:
			print("Invalid target number. Try again.")
			continue

		# Parse numbers safely
		try:
			arr = flexsearch.parse_numbers(numbers)
			if not arr:
				print("No valid numbers found. Try again.")
				continue
		except ValueError:
			print("Invalid number format. Try again.")
			continue

		# Sort and search
		sorted_arr = flexsearch.sort_numbers(arr, mode)
		idx = flexsearch.binary_search(sorted_arr, target)

		# Output results
		print("\nInput string:", numbers)
		print("Sorted array:", sorted_arr)
		if idx == -1:
			print(f"Target {target} not found in the array.")
		else:
			print(f"Target {target} found at index:", idx)

if __name__ == "__main__":
	main()
