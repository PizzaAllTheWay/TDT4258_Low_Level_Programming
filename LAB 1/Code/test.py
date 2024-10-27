def is_palindrome(s):
    # Remove spaces and convert to lowercase for case insensitivity
    clean_str = ''.join(c.lower() for c in s if c.isalnum())  # Keep only alphanumeric characters

    # Compare the string with its reverse
    return clean_str == clean_str[::-1]

# Test cases
test_strings = [
    "11211",
    "22ee22",
    "22eE22",
    "22eE2 2",
    "22eE2   2",
    "00?00",
    "00 ?00",
    "a0 ? 0 a",
    "123",   # Not a palindrome
    "2RR",   # Not a palindrome
    "22 e 2" # Not a Palindrome (ignoring spaces)
]

for test in test_strings:
    print(f"'{test}' is palindrome: {is_palindrome(test)}")
