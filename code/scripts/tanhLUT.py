import numpy as np

# written by chatgpt 4o

# Number of entries in the LUT
LUT_SIZE = 65536

# Define the input range for the tanh function (-pi to pi)
INPUT_MIN = -np.pi
INPUT_MAX = np.pi

# Define the fixed-point range (-1 to 1)
FIXED_POINT_MIN = -1.0
FIXED_POINT_MAX = 1.0

# Compute the step size for the input range
input_step = (INPUT_MAX - INPUT_MIN) / (LUT_SIZE - 1)

# Initialize the LUT array
lut = np.zeros(LUT_SIZE, dtype=np.int16)

# Fill the LUT with tanh values
for i in range(LUT_SIZE):
    # Compute the input value
    input_value = INPUT_MIN + i * input_step
    # Compute the tanh value
    tanh_value = np.tanh(input_value)
    # Map the tanh value to the fixed-point range
    fixed_point_value = int((tanh_value - FIXED_POINT_MIN) / (FIXED_POINT_MAX - FIXED_POINT_MIN) * 65535 - 32768)
    # Store the fixed-point value in the LUT
    lut[i] = np.clip(fixed_point_value, -32768, 32767)

# Save the LUT to a header file
header_filename = "lookup_tanh.h"

with open(header_filename, 'w') as header_file:
    header_file.write("#ifndef TANH_LUT_H\n")
    header_file.write("#define TANH_LUT_H\n\n")
    header_file.write("#include <stdint.h>\n\n")
    header_file.write("#define LUT_SIZE {}\n\n".format(LUT_SIZE))
    header_file.write("const int16_t tanh_lut[LUT_SIZE] = {\n")
    
    # Write the LUT values
    for i in range(LUT_SIZE):
        header_file.write("    {}".format(lut[i]))
        if i < LUT_SIZE - 1:
            header_file.write(",")
        if (i + 1) % 8 == 0:
            header_file.write("\n")
        else:
            header_file.write(" ")

    header_file.write("\n};\n\n")
    header_file.write("#endif // TANH_LUT_H\n")

print(f"LUT has been saved to {header_filename}")

# Function to use the LUT in Python (for testing)
def tanh_lut(input_value):
    # Clip the input value to the allowed range
    input_value = np.clip(input_value, INPUT_MIN, INPUT_MAX)
    # Map the input value to the LUT index
    index = int((input_value - INPUT_MIN) / (INPUT_MAX - INPUT_MIN) * (LUT_SIZE - 1))
    # Return the corresponding value from the LUT
    return lut[index]

# Example usage
input_value = 0.5  # Input value in the range [-pi, pi]
output_value = tanh_lut(input_value)
print(f"Tanh LUT output for input {input_value}: {output_value}")

