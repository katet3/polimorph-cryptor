import os
import random
import re

# Пути к каталогам
ANTI_AMULATE_DIR = 'anti-amulate'
CHIPHER_FUNC_DIR = 'chipher/algo'
CHIPHER_CALL_DIR = 'chipher/call'
TRASH_CODE_DIR   = 'trash_code'
TEMPLATE_FILE    = 'template/unpacker_template.cpp'
OUTPUT_FILE      = 'unpacker.cpp'
CHIPHER_FILE_NAME = ''
MORPH_PARAMETERS_FILE = '../../Win32/src/simple_packer/morph_parametrs.h'

def get_file_content(_file_path):
    with open(_file_path, 'r', encoding='utf-8') as file:
        return file.read()


def get_random_file_content(_directory):
    files = [f for f in os.listdir(_directory) if os.path.isfile(os.path.join(_directory, f))]
    if not files:
        return ''

    random_file = random.choice(files)
    if _directory == CHIPHER_FUNC_DIR:
        global CHIPHER_FILE_NAME
        CHIPHER_FILE_NAME = random_file

    with open(os.path.join(_directory, random_file), 'r', encoding='utf-8') as file:
        return file.read()


def create_morph_parameters(enc_algo):
    content = f'''#pragma once

/*
    1 - XOR
    2 - AES
    3 - RC4
*/
static const unsigned int enc_algo = 0x{enc_algo:02X};
'''
    with open(MORPH_PARAMETERS_FILE, 'w', encoding='utf-8') as file:
        file.write(content)


def add_chipher(_template_content):
    chipher_func_code = get_random_file_content(CHIPHER_FUNC_DIR)
    chipher_call_code = get_file_content(CHIPHER_CALL_DIR + '/' + CHIPHER_FILE_NAME)

    # Определите алгоритм шифрования на основе имени файла или другого критерия
    if 'xor' in CHIPHER_FILE_NAME.lower():
        enc_algo = 1
        print('Chipher xor')
    elif 'aes' in CHIPHER_FILE_NAME.lower():
        enc_algo = 2
        print('Chipher aes')
    elif 'rc4' in CHIPHER_FILE_NAME.lower():
        enc_algo = 3
        print('Chipher rc4')
    else:
        enc_algo = 0  # Неизвестный алгоритм

    create_morph_parameters(enc_algo)

    final_code = _template_content.replace('/* CHIPHER_FUNC_PLACEHOLDER */', chipher_func_code)
    final_code = final_code.replace('/* CHIPHER_CALL_PLACEHOLDER */', chipher_call_code)

    return final_code


def add_trash_code_main(final_code, num_insertions):
    unpacker_main_pattern = re.compile(r'(extern "C" void __declspec\(naked\) unpacker_main\(\)\s*{[\s\S]*?})', re.MULTILINE)
    match = unpacker_main_pattern.search(final_code)
    if not match:
        raise ValueError("Function unpacker_main not found in the template.")
    
    unpacker_main_code = match.group(1)

    # Разделим функцию на строки
    lines = unpacker_main_code.split('\n')

    # Найдем все возможные позиции для вставки, исключая строки с __asm и первую строку функции
    possible_positions = []
    for i in range(1, len(lines) - 1):
        if not lines[i].strip().startswith('__asm') and not lines[i+1].strip().startswith('__asm'):
            possible_positions.append(i)

    if not possible_positions:
        raise ValueError("No valid positions found to insert trash code.")

    # Вставим мусорный код num_insertions раз в случайные позиции
    for _ in range(num_insertions):
        if not possible_positions:
            break
        insert_position = random.choice(possible_positions)
        trash_code = get_random_file_content(TRASH_CODE_DIR)
        lines.insert(insert_position + 1, f'\n{trash_code}\n')
        possible_positions.remove(insert_position)

    updated_unpacker_main_code = '\n'.join(lines)

    # Замена старого кода unpacker_main на обновленный с вставками мусорного кода
    final_code = final_code.replace(unpacker_main_code, updated_unpacker_main_code)

    return final_code


def add_anti_amulaten_code_main(final_code, num_insertions):
    unpacker_main_pattern = re.compile(r'(extern "C" void __declspec\(naked\) unpacker_main\(\)\s*{[\s\S]*?})', re.MULTILINE)
    match = unpacker_main_pattern.search(final_code)
    if not match:
        raise ValueError("Function unpacker_main not found in the template.")
    
    unpacker_main_code = match.group(1)

    # Разделим функцию на строки
    lines = unpacker_main_code.split('\n')

    # Найдем все возможные позиции для вставки, исключая строки с __asm и первую строку функции
    possible_positions = []
    for i in range(1, len(lines) - 1):
        if not lines[i].strip().startswith('__asm') and not lines[i+1].strip().startswith('__asm'):
            possible_positions.append(i)

    if not possible_positions:
        raise ValueError("No valid positions found to insert anti-amulate code.")

    # Вставим анти-обфускационный код num_insertions раз в случайные позиции
    for _ in range(num_insertions):
        if not possible_positions:
            break
        insert_position = random.choice(possible_positions)
        anti_amulaten_code = get_random_file_content(ANTI_AMULATE_DIR)
        lines.insert(insert_position + 1, f'\n{anti_amulaten_code}\n')
        possible_positions.remove(insert_position)

    updated_unpacker_main_code = '\n'.join(lines)

    # Замена старого кода unpacker_main на обновленный с вставками анти-обфускационного кода
    final_code = final_code.replace(unpacker_main_code, updated_unpacker_main_code)

    return final_code


def add_prologue_epilogue(final_code):
    # Получаем случайный файл из assembler/prologue
    prologue_files = [f for f in os.listdir('assembler/prologue') if os.path.isfile(os.path.join('assembler/prologue', f))]
    if not prologue_files:
        raise ValueError("No files found in assembler/prologue directory.")
    
    prologue_file = random.choice(prologue_files)
    prologue_content = get_file_content(os.path.join('assembler/prologue', prologue_file))

    # Получаем файл из assembler/epilogue с таким же именем
    epilogue_file = prologue_file
    epilogue_content = get_file_content(os.path.join('assembler/epilogue', epilogue_file))

    # Заменяем плейсхолдеры в финальном коде
    final_code = final_code.replace('/* ASM_PROLOG_PLACEHOLDER */', prologue_content)
    final_code = final_code.replace('/* ASM_EPILOGUE_PLACEHOLDER */', epilogue_content)

    return final_code



def main():
    # Чтение шаблона
    with open(TEMPLATE_FILE, 'r', encoding='utf-8') as template_file:
        template_content = template_file.read()

    final_code = add_chipher(template_content)

    final_code = add_trash_code_main(final_code, 10)

    final_code = add_prologue_epilogue(final_code)

    # Сохранение итогового файла
    with open(OUTPUT_FILE, 'w', encoding='utf-8') as output_file:
        output_file.write(final_code)

    print(f"Unpacker generated: {OUTPUT_FILE}")

if __name__ == "__main__":
    main()
