#!/usr/bin/env python3

# Brute-force search for the required number 'x'
def find_number():
    min_value = 132000000
    max_value = 200000000
    divisor = 1000000
    multiplier = 44000
    result = False
    for x in range(1, max_value // multiplier + 1):
        product = x * multiplier
        if min_value <= product <= max_value and product % divisor == 0:
            print(f"Found number: {x}, freq: {product}\n")
            result = True
    return result

result = find_number()
if result:
    print("Numbers found.")
else:
    print("No suitable number found.")