from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection
import re
import sys

def find_function_calls(obj_file_path, function_names, target_functions):
    with open(obj_file_path, 'rb') as f:
        elffile = ELFFile(f)

        # Find the symbol table
        symtab = elffile.get_section_by_name('.symtab')
        if not symtab or not isinstance(symtab, SymbolTableSection):
            print(f"No symbol table found in {obj_file_path}")
            return {}

        # Find the addresses of the target functions
        target_addrs = {}
        for symbol in symtab.iter_symbols():
            if symbol.name in target_functions:
                target_addrs[symbol.name] = symbol['st_value']

        if not target_addrs:
            print(f"None of the target functions found in {obj_file_path}")
            return {}

        # Find the .text section (code)
        text_section = elffile.get_section_by_name('.text')
        if not text_section:
            print(f"No .text section found in {obj_file_path}")
            return {}

        # Get the .text section data and address
        text_data = text_section.data()
        text_addr = text_section['sh_addr']

        # Dictionary to store return addresses for each function
        return_addresses = {func: {target: [] for target in target_functions} for func in function_names}

        # Iterate through the symbol table to find the functions
        for symbol in symtab.iter_symbols():
            if symbol.name in function_names:
                func_name = symbol.name
                func_addr = symbol['st_value']
                func_size = symbol['st_size']

                # Iterate through the function's code to find calls to target functions
                for offset in range(0, func_size - 4, 4):  # Assuming 4-byte instructions
                    instruction = text_data[func_addr - text_addr + offset:func_addr - text_addr + offset + 4]
                    for target_name, target_addr in target_addrs.items():
                        if is_bl_instruction(instruction, target_addr, func_addr + offset):
                            # Ret address is the address of the next instruction
                            return_address = func_addr + offset + 4
                            return_addresses[func_name][target_name].append(return_address)

        return return_addresses

def is_bl_instruction(instruction, target_addr, current_addr):
    # ARM 'bl' instruction opcode (adjust for your architecture)
    # bl instruction: opcode = 0xEB, followed by a 24-bit signed offset
    if len(instruction) != 4:
        return False

    # Check if the instruction is a 'bl' (branch with link)
    if (instruction[3] & 0xFF) == 0xEB:
        # Calculate the target address of the 'bl' instruction
        offset = int.from_bytes(instruction[:3], 'little', signed=True) << 2
        calculated_target_addr = current_addr + 8 + offset
        return calculated_target_addr == target_addr

    return False

def main():
    if len(sys.argv) < 4:
        print(f"Usage: {sys.argv[0]} <object_file> <function1> <function2> ... -- <target1> <target2> ...")
        print("Example: python get_yield_ret_addr.py firmware.o Log_Task Task1 -- vTaskDelay vTaskDelayUntil vTaskSuspend taskYIELD_FROM_TASK")
        sys.exit(1)

    obj_file_path = sys.argv[1]  # Path to the object file
    separator_index = sys.argv.index('--')  # Separator between function names and target functions
    function_names = sys.argv[2:separator_index]  # List of function names to analyze
    target_functions = sys.argv[separator_index + 1:]  # List of target functions to search for

    return_addresses = find_function_calls(obj_file_path, function_names, target_functions)

    print("Valid return addresses for target function calls:")
    for func, targets in return_addresses.items():
        print(f"Function: {func}")
        for target, addresses in targets.items():
            if addresses:
                print(f"  Calls to {target}:")
                for addr in addresses:
                    print(f"    0x{addr:08X} - {int(addr)}")

if __name__ == '__main__':
    main()