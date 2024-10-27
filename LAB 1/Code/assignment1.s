//; Define start of the Assembly instuctions
.global _start

_start:
    //; Initialization stage --------------------------------------------------
    push {lr} //; Save return address

    //; Start every time with blank slate for consistency
    bl clear_registers //; Branch and link to clear_registers function

    //; Print "Hello World!" in JTAG UART to confirm it works properly
    ldr r0, =test_str //; Load r0 with test_str because this will be passed to the fuction
    bl jtag_uart_send_data //; Branch and link to jtag_uart_send_data function

    //; Start every time with all LEDs in off state
    mov r0, #0b00000 //; Pass r0 to function as: Left 5 most LEDs states (All off)
    mov r1, #0b00000 //; Pass r1 to function as: Right 5 most LEDs states (All off)
    bl led_status_set
    
    pop {lr} //; Restore return address



    //; Start Palidrom Logic --------------------------------------------------
    push {lr} //; Save return address

    //; Check if input is palidrom
    ldr r0, =input //; Input to funstion to check if the input variable is a palidrom
    bl check_palindrom //; Returns r0 as TRUE(1)/FALSE(0)
    mov r4, r0 //; Since r0 should be kep for function calling, we move answer in r0 to r4 
    
    //; Check Results. 
    //; Logic to select correct outcome
    cmp r4, #1 //; Compare the result in r0 with 1 (TRUE)
    bne .+8 //; If r0 == 1, branch to is_palindrom, else skip next line
	bl is_palindrom

    cmp r4, #0 //; Compare the result in r0 with 0 (FALSE)
    bne .+8 //; If r0 == 0, branch to is_no_palindrom, else skip next line
    bl is_no_palindrom

    //; End the progam
    pop {lr} //; Restore return address
	b _exit



//; Register clear function --------------------------------------------------
clear_registers:
    //; Make every register 0x00000000
    mov r0 , #0
    mov r1 , #0
    mov r2 , #0
    mov r3 , #0
    mov r4 , #0
    mov r5 , #0
    mov r6 , #0
    mov r7 , #0
    mov r8 , #0
    mov r9 , #0
    mov r10, #0
    mov r11, #0
    mov r12, #0

    //; Return
    bx lr //; Branch back to caller



//; UART functions --------------------------------------------------
check_string_length:
    mov r1, r0 //; Copy the string pointer from r0 to r1
    mov r0, #0 //; Initialize the length counter r0 to 0

    push {lr} //; Save return address
    bl check_string_length_loop
    pop {lr} //; Restore return address

    //; Return
    bx lr //; Branch back to caller

check_string_length_loop:
    ldrb r2, [r1], #1 //; Load byte at r1 to r2 for inspection, and increment r1 
    cmp r2, #0 //; Compare the byte with null terminator
    bne .+8 //; If: Null terminator, exit the loop | If: NOT equal branch OVER next instruction
	bx lr //; Branch back to caller
    add r0, r0, #1 //; Increment the string length counter
    b check_string_length_loop //; Continue the loop

jtag_uart_send_data:
    push {r4}  //; Save r4 onto the stack just in case we mess with these inside the function
    
    //; Get the lenght of the array
    push {lr} //; Save return address
    push {r0-r3}  //; Save r0-r3 onto the stack just in case we mess with these inside the function
    //; further pass r0 to the function as it contains the message we wnat to send through UART 
    bl check_string_length //; Branch and link to check_string_length function
    mov r4, r0 //; Move string lenght r0 to r4 as r0 will be erraced and replaced with new values from the stack in the line bellow
    pop {r0-r3}  //; load r0-r3 from the stack just in case we messed with these inside the function
    pop {lr} //; Restore return address

    //; Send data to JTAG UART one byte at the time
    push {lr} //; Save return address
    push {r0-r3}  //; Save r0-r3 onto the stack just in case we mess with these inside the function
    //; r0 we loaded from the stack so we perserved the data string to send
    mov r1, r4 //; Move data string lenght to r1 for function input
    ldr r4, =0xFF201000 //; Load UART data register address
    bl jtag_uart_send_data_loop //; Branch and link to jtag_uart_send_data_loop function
    pop {r0-r3}  //; load r0-r3 from the stack just in case we messed with these inside the function
    pop {lr} //; Restore return address
    
    //; Return
    pop {r4}  //; load r4 from the stack just in case we messed with these inside the function
    bx lr //; Branch back to caller

jtag_uart_send_data_loop:
    //ldrb r2, [r1], #1 //; Load byte at r1 to r2 for inspection, and increment r1 
    cmp r1, #0 //; Compare the byte with null terminator
    bne .+8 //; If: Null terminator, exit the loop | If: NOT equal branch OVER next instruction
	bx lr //; Branch back to caller

    ldrb r2, [r0], #1 //; Load byte at r0 (string pointer), then increment r0
    strb r2, [r4] //; Write the byte to the UART data register

    sub r1, r1, #1 //; Decrease the string length counter
    b jtag_uart_send_data_loop //; Continue the loop



//; LED functions --------------------------------------------------
led_status_set:
    push {r2-r12}  //; Save r4-r12 onto the stack just in case we mess with these inside the function
    
    ldr r4, =0xFF200000 //; Load LED base address into r4
    and r0, r0, #0x1F //; Mask r0 to only keep the lowest 5 bits for the right 5 LEDs
    and r1, r1, #0x1F //; Mask r1 to only keep the lowest 5 bits for the left 5 LEDs
    lsl r0, r0, #5 //; Shift r0 left by 5 bits to align it with the left 5 LEDs
    orr r0, r1, r0 //; Combine the values from r0 and r1 into a single 10-bit value
    str r0, [r4] //; Store the combined value in the LED data register, making LEDs turn on or off
    
    //; Return
    pop {r2-r12}  //; load r2-r12 from the stack just in case we messed with these inside the function
    bx lr //; Branch back to caller



//; Palindrom functions --------------------------------------------------
remove_spaces:
    push {lr} //; Save return address
    push {r2-r12}  //; Save r4-r12 onto the stack just in case we mess with these inside the function

    //; Removing space loop
    mov r1, r0 //; Copy the pointer of the string r0 to r1 to make space for r0 as output later, r1 used for comparison
    bl remove_spaces_loop //; Start removing spaces
    mov r1, #0 //; Place the null terminator used down bellow to terminate the string
    strb r1, [r0] //; Place the null terminator (r1) after the last character in r0

    //; Return
    pop {r2-r12}  //; load r4-r12 from the stack just in case we messed with these inside the function
    pop {lr} //; Restore return address
    bx lr //; Return to the caller

remove_spaces_loop:
    ldrb r2, [r1], #1 //; Load a byte from the string at r1 into r2, then increment r1
    cmp r2, #0 //; Check if it's the null terminator
    bne .+8 //; If: Null terminator, exit the loop | Else: NOT equal branch OVER next instruction
	bx lr //; Branch back to caller

    cmp r2, #' ' //; Check if the byte is a space (ASCII 0x20)
    beq remove_spaces_loop //; If space, skip to the next character

    strb r2, [r0], #1 //; Store the non-space character in r0, then increment r0
    b remove_spaces_loop //; Repeat the loop

remove_special_characters:
    push {lr} //; Save return address
    push {r2-r12}  //; Save r4-r12 onto the stack just in case we mess with these inside the function

    //; Removing special charecters loop
    mov r2, r0 //; Copy the pointer of the string r0 to r1 to make space for r0 as output later, r1 used for comparison
    //; r3 stays the same, a array lenth variable
    bl remove_special_characters_loop //; Start removing special charecters

    //; Return
    pop {r2-r12}  //; load r4-r12 from the stack just in case we messed with these inside the function
    pop {lr} //; Restore return address
    bx lr //; Return to the caller

remove_special_characters_loop:
    ldrb r1, [r2], #1 //; Load a byte from the string at r2 into r1, and increment r2
    cmp r1, #0 //; Check if it is the null terminator
    bne .+8 //; If: Null terminator, exit the loop | Else: NOT equal branch OVER next instruction
	bx lr //; Branch back to caller

    cmp r1, #'?' //; Compare with question mark
    bne remove_special_characters_loop //; If: Null terminator, go remove the charecters | Else: NOT equal branch OVER next 3 instructions
	push {lr} //; Save return address
    bl remove_characters_pair //; Brances to the charecter removing pair function (r0 is array adress start, r1 is value of the adress we are looking at, r2 is next byte on the list, r3 is passed in as array lenght)
    pop {lr} //; Restore return address

    b remove_special_characters_loop //; Continue to next character

remove_characters_pair:
    push {r4-r6} //; Save address

    //; Calculate mirrored index (mirrored_index = string_len - current_index - 1)
    sub r4, r2, r0 //; r4 = next_index (next address - start of string adress)
    sub r4, r4, #1 //; r4 = current_index (next_index - 1)
    sub r4, r3, r4 //; r4 = string_length - current_index
    sub r4, r4, #1 //; r4 = mirrored_index

    // Remove mirrored character at r4
    add r5, r0, r4      // r5 = pointer to mirrored character
    mov r6, #' '        // r6 = space character (or any marker for removal)
    strb r6, [r2, #-1]  // Overwrite '?' with space (go back to the previous byte in r2)
    strb r6, [r5]       // Overwrite mirrored character with space

    pop {r4-r6} //; Restore address
    bx lr //; Branch back to caller

cast_to_lower_case:
    push {lr} //; Save return address

    //; Lopp through the whole array and make sure everything is cast to lower cases
    //; r0: Array Adress start
    //; r1: Array length
    bl cast_to_lower_case_loop

    //; Return
    pop {lr} //; Restore return address
    bx lr //; Return to the caller

cast_to_lower_case_loop:
    cmp r1, #0 //; Check if we've processed all characters
    bne .+8 //; If r1 == 0, exit the loop | Else: NOT equal branch OVER next instruction
	bx lr //; Branch back to caller

    sub r1, r1, #1 //; Decrement the length counter (r1)

    ldrb r2, [r0], #1 //; Load the current byte from the string r0 into r3, increment r0
    cmp r2, #'A' //; Compare the current character with 'A' (ASCII 65)
    blt cast_to_lower_case_loop //; If it is less than 'A', skip conversion (non-uppercase)

    cmp r2, #'Z' //; Compare the current character with 'Z' (ASCII 90)
    bgt cast_to_lower_case_loop //; If it is greater than 'Z', skip conversion

    add r2, r2, #32 //; Convert uppercase letter to lowercase by adding 32
    strb r2, [r0, #-1] //; Store the modified byte back into the string

    b cast_to_lower_case_loop //; Repeat for the next character

check_palindrom:
    push {r2-r12}  //; Save r4-r12 onto the stack just in case we mess with these inside the function

    //; Remove spaces from string
    push {lr} //; Save return address
    push {r0-r3}  //; Save r0-r3 onto the stack just in case we mess with these inside the function
    //; pass r0 to the function as it contains the message we wnat to remove spaces from
    bl remove_spaces //; Branch and link to remove_spaces function
    //; We now have sucessfully manipulated r0 to have no spaces
    mov r5, r0 //; The upper bound of adress memory
    pop {r0-r3}  //; load r0-r3 from the stack just in case we messed with these inside the function
    mov r4, r0 //; The lower bound of adress memory
    pop {lr} //; Restore return address

    //; Get the lenght of the new array
    push {lr} //; Save return address
    push {r0-r3}  //; Save r0-r3 onto the stack just in case we mess with these inside the function
    //; pass r0 to the function as it contains the new filtered message we wnat to check for palidrom 
    bl check_string_length //; Branch and link to check_string_length function
    mov r6, r0 //; Move string lenght r0 to r6 as r0 will be erraced and replaced with new values from the stack in the line bellow
    pop {r0-r3}  //; load r0-r3 from the stack just in case we messed with these inside the function
    pop {lr} //; Restore return address

    //; Convert question marks to spaces in string
    push {lr} //; Save return address
    push {r0-r3}  //; Save r0-r3 onto the stack just in case we mess with these inside the function
    mov r0, r4 //; pass r0 to the function as it contains the message we want to replace question marks with spaces, start of the mesage
    mov r3, r6 //; r3 will represent array lenght that will be used down the line
    bl remove_special_characters //; Branch and link to remove_special_characters function
    //; We now have sucessfully manipulated r0 to have no ? and their potential palindrom charecters
    pop {r0-r3}  //; load r0-r3 from the stack just in case we messed with these inside the function
    pop {lr} //; Restore return address

    //; Remove spaces from string
    push {lr} //; Save return address
    push {r0-r3}  //; Save r0-r3 onto the stack just in case we mess with these inside the function
    mov r0, r4 //; pass r0 to the function as it contains the message we want to remove spaces from, start of the mesage
    bl remove_spaces //; Branch and link to remove_spaces function
    //; We now have sucessfully manipulated r0 to have no spaces
    mov r5, r0 //; The upper bound of adress memory
    pop {r0-r3}  //; load r0-r3 from the stack just in case we messed with these inside the function
    mov r4, r0 //; The lower bound of adress memory
    pop {lr} //; Restore return address

    //; Get the lenght of the new array
    push {lr} //; Save return address
    push {r0-r3}  //; Save r0-r3 onto the stack just in case we mess with these inside the function
    //; pass r0 to the function as it contains the new filtered message we wnat to check for palidrom 
    bl check_string_length //; Branch and link to check_string_length function
    mov r6, r0 //; Move string lenght r0 to r6 as r0 will be erraced and replaced with new values from the stack in the line bellow
    pop {r0-r3}  //; load r0-r3 from the stack just in case we messed with these inside the function
    pop {lr} //; Restore return address

    //; Cast all remaining filtrated data array series to lower case
    push {lr} //; Save return address
    push {r0-r3}  //; Save r0-r3 onto the stack just in case we mess with these inside the function
    mov r0, r4 //; pass r0 to the function as it contains the new filtered message we wnat to check for palidrom 
    mov r1, r6 //; Move string lenght to r1
    bl cast_to_lower_case //; Branch and link to cast_to_lower_case function
    //; We now have sucessfully manipulated r0 to have all lower cases
    pop {r0-r3}  //; load r0-r3 from the stack just in case we messed with these inside the function
    pop {lr} //; Restore return address

    //; Check the string if it is palindrom
    push {lr} //; Save return address
    push {r0-r3}  //; Save r0-r3 onto the stack just in case we mess with these inside the function
    mov r0, r4 //; pass r0 to the function as it contains the lower bound of message adress 
    mov r1, r5 //; pass r1 to the function as it contains the upper bound of message adress
    ldrb r3, [r1], #-1 //; Decrement r1 (upper bound) an extra time at the start, because the last adress in data array is NULL TERMINATE, so we want to avoid that
    bl check_palindrom_loop //; Branch and link to check_palindrom_loop function
    mov r4, r0 //; Move string lenght r0 to r4 as r0 will be erraced and replaced with new values from the stack in the line bellow
    pop {r0-r3}  //; load r0-r3 from the stack just in case we messed with these inside the function
    pop {lr} //; Restore return address

	//; Return
    mov r0, r4 //; Here we return r0 as a TRUE(1)/FALSE(0) for determining if string was palindrom or not
    pop {r2-r12}  //; load r4-r12 from the stack just in case we messed with these inside the function
    bx lr //; Branch back to caller

check_palindrom_loop:
    ldrb r2, [r0], #1 //; Load byte from the start of the string and increment r0 (lower bound)
    ldrb r3, [r1], #-1 //; Decrement r1 (upper bound)

    cmp r1, r0 //; Compare pointers: If they meet or cross, we are done
    blt palindrome_true //; If lower bound is greater, it is a palindrome

    cmp r2, r3 //; Compare the two characters
    bne palindrome_false //; If they don not match, it is not a palindrome

    b check_palindrom_loop //; Continue to the next pair

palindrome_true:
    mov r0, #1 //; Return TRUE
    bx lr //; Branch back to caller

palindrome_false:
    mov r0, #0 //; Return FALSE
    bx lr //; Branch back to caller

is_palindrom:
    push {lr} //; Save return address
    push {r2-r12}  //; Save r4-r12 onto the stack just in case we mess with these inside the function
    
	//; Switch on only the 5 rightmost LEDs
    mov r0, #0b00000 //; Pass r0 to function as: Left 5 most LEDs states (All off)
    mov r1, #0b11111 //; Pass r1 to function as: Right 5 most LEDs states (All on)
    bl led_status_set

	//; Write 'Palindrom detected' to UART
    ldr r0, =input //; Load r0 with custom message
    bl jtag_uart_send_data //; Branch and link to jtag_uart_send_data function
    ldr r0, =newline_str //; Load r0 with custom message
    bl jtag_uart_send_data //; Branch and link to jtag_uart_send_data function
    ldr r0, =is_palindrom_str //; Load r0 with custom message
    bl jtag_uart_send_data //; Branch and link to jtag_uart_send_data function
    ldr r0, =newline_str //; Load r0 with custom message
    bl jtag_uart_send_data //; Branch and link to jtag_uart_send_data function
    ldr r0, =newline_str //; Load r0 with custom message
    bl jtag_uart_send_data //; Branch and link to jtag_uart_send_data function
    ldr r0, =newline_str //; Load r0 with custom message
    bl jtag_uart_send_data //; Branch and link to jtag_uart_send_data function

    //; Return
    pop {r2-r12}  //; load r4-r12 from the stack just in case we messed with these inside the function
	pop {lr} //; Restore return address
    bx lr //; Branch back to caller

is_no_palindrom:
    push {lr} //; Save return address
    push {r2-r12}  //; Save r4-r12 onto the stack just in case we mess with these inside the function
    
	//; Switch on only the 5 leftmost LEDs
    mov r0, #0b11111 //; Pass r0 to function as: Left 5 most LEDs states (All off)
    mov r1, #0b00000 //; Pass r1 to function as: Right 5 most LEDs states (All on)
    bl led_status_set

	//; Write 'Not a palindrom' to UART
    ldr r0, =input //; Load r0 with custom message
    bl jtag_uart_send_data //; Branch and link to jtag_uart_send_data function
    ldr r0, =newline_str //; Load r0 with custom message
    bl jtag_uart_send_data //; Branch and link to jtag_uart_send_data function
    ldr r0, =is_no_palindrom_str //; Load r0 with custom message
    bl jtag_uart_send_data //; Branch and link to jtag_uart_send_data function
    ldr r0, =newline_str //; Load r0 with custom message
    bl jtag_uart_send_data //; Branch and link to jtag_uart_send_data function
    ldr r0, =newline_str //; Load r0 with custom message
    bl jtag_uart_send_data //; Branch and link to jtag_uart_send_data function
    ldr r0, =newline_str //; Load r0 with custom message
    bl jtag_uart_send_data //; Branch and link to jtag_uart_send_data function
	
    //; Return
    pop {r2-r12}  //; load r4-r12 from the stack just in case we messed with these inside the function
	pop {lr} //; Restore return address
    bx lr //; Branch back to caller



//; Exit infinity loop function --------------------------------------------------
_exit:
	//; Infinite loop
    //; This way we dont interate into .data and destroy everything there O_O
    b .
	


//; Data and Variables --------------------------------------------------
.data
.align
    input: .asciz "Grav ned den varg"
    test_str: .asciz "Hello World! \n"
    newline_str: .asciz "\n"
    is_palindrom_str: .asciz "Palindrom detected! \n"
    is_no_palindrom_str: .asciz "Not a palindrom O_O \n"
.end