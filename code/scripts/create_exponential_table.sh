import numpy as np

# Constants
TABLE_SIZE = 1024
Q31_MAX = 2147483647
DECAY_CONSTANT = 4.0  # Adjust this value to make the transition slower

# Function to convert a float to Q31
def float_to_q31(value):
    return int(value * Q31_MAX)

# Generate the exponential table
exp_table = []
for i in range(TABLE_SIZE):
    # Calculate exponential decay value with a slower decay constant
    value = np.exp(-DECAY_CONSTANT * i / (TABLE_SIZE - 1))
    # Convert to Q31
    q31_value = float_to_q31(value)
    exp_table.append(q31_value)

# Save the table to a file
with open("exponential_table_q31.h", "w") as f:
    f.write("#ifndef EXPONENTIAL_TABLE_Q31_H\n")
    f.write("#define EXPONENTIAL_TABLE_Q31_H\n\n")
    f.write("const int32_t exp_table_q31[{}] = {{\n".format(TABLE_SIZE))
    for i, value in enumerate(exp_table):
        if i % 8 == 0:
            f.write("\n    ")
        f.write("{:>11},".format(value))
    f.write("\n};\n\n")
    f.write("#endif // EXPONENTIAL_TABLE_Q31_H\n")

print("Exponential table saved to 'exponential_table_q31.h'")