# Kastle 2 Coding Style Guide
This document outlines the coding style conventions to be followed for the Kastle 2 project. Adhering to these guidelines will ensure consistency and readability across the project's codebase.

## 1. C++ Header Files
Use `#pragma once` at the beginning instead of old-style include guards.
Header files should have `*.hpp` extension.

```cpp
MyClassName.hpp

#pragma once
```

## 2. Class Names
Class names should be written in **PascalCase**. This means that the first letter of each word in the class name should be capitalized, with no spaces, underscores, or other separators between the words.

**Example:**

```cpp
class MyClassName {
    // Class implementation
};
```

## 3. Method Names
Method names should be written in **PascalCase**. This means that the first letter of each word in the method name should be capitalized, with no spaces, underscores, or other separators between the words.

**Example:**

```cpp
bool MyMethodName(bool parameter) {
    // Method implementation
}
```

## 4. Local variables
Local variables should be written in lowercase, with words separated by underscores (_).

**Example:**

```cpp
int32_t a_very_nice_number;
```

## 5. Private Member Variables
Private member variables should be written in **lowercase**, with words separated by underscores (_), and should include a trailing underscore.

**Example:**

```cpp
private:
    float a_very_nice_number_;
    q31_t amplitude_;
    q31_t phase_, phase_inc_;
```

## 6. Constants
Constants should be named starting a lowercase **k**, then **PascalCase**.

**Example:**

```cpp
static constexpr int32_t kImportantNumber = 69420;
static const auto kMapRingMod = MapDef<int32_t, 4>{
    {pot(0.0f), pot(0.4f), pot(0.7f), pot(1.0f)},
    {q15(0.0f), q15(1.0f),q15(1.0f), q15(4.0f)}};

```

## 7. Enums
Enumerations should be used to define a set of named integer constants. The enum name should be singular if we presume only one entry is valid at once. Enumerator names should be written in uppercase, with the last item in the enumeration representing the total count of enumerators. Use `enum class` instead of plain enums. With this structure you can then use utilities like EnumArray or EnumIncrement.

**Example:**

```cpp
enum class MyBerry
{
    RASPBERRY,        
    BLUEBERRY, 
    STRAWBERRY,
    COUNT 
};

EnumArray<MyBerry, int32_t> berries_count;

MyBerry current_berry = MyBerry::RASPBERRY;
current_berry = EnumIncrement(current_berry);
```

## 8. Code Formatting
Use 4 spaces for indentation; do not use tabs. Always include braces for `if`, `for`, `while`, and other control statements.
Don't put control statements and their code blocks on the same line.

**Example:**

```
if (condition)
{
    OneLineFunction();
}
```

## 9. Miscellaneous
Use `constexpr` and `const` where applicable, especially for parameters and values that should not be modified.
Prefer meaningful variable names over single-letter names, except in loop counters where i, j, etc., are acceptable.

By adhering to these guidelines, the Kastle 2 project will maintain a high standard of code quality and readability. These conventions will make it easier for developers to collaborate and maintain the codebase over time.

Thank you and happy coding!

