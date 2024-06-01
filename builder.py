import subprocess
import os
import argparse

PATH_UNPACKER         = 'Win32\\src\\unpacker\\Release\\unpacker.exe'
PATH_CONVERTER        = 'Win32\\src\\unpacker_converter\\x64\\Debug\\unpacker_converter.exe'
PATH_OUTPUT_CONVERTER = 'Win32\\src\\simple_packer\\unpacker.h'

PATH_PACKER = 'Win32\\src\\simple_packer\\x64\\Debug'

def find_msbuild():
    vswhere_path = r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if not os.path.exists(vswhere_path):
        raise FileNotFoundError(f"vswhere.exe не найден по пути: {vswhere_path}")

    result = subprocess.run(
        [vswhere_path, "-latest", "-requires", "Microsoft.Component.MSBuild", "-find", "MSBuild\\**\\Bin\\MSBuild.exe"],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )

    if result.returncode != 0:
        raise RuntimeError(f"Не удалось найти MSBuild: {result.stderr.strip()}")

    msbuild_path = result.stdout.strip()
    if not msbuild_path:
        raise RuntimeError("MSBuild не найден.")
    
    return msbuild_path

def build_project(solution_path, configuration, platform):
    try:
        msbuild_path = find_msbuild()
        print(f"MSBuild найден по пути: {msbuild_path}")
    except (FileNotFoundError, RuntimeError) as e:
        print(f"Ошибка: {e}")
        return -1

    build_command = f'"{msbuild_path}" "{solution_path}" /p:Configuration={configuration} /p:Platform={platform}'
    result = subprocess.run(build_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    if result.returncode == 0:
        print("Сборка прошла успешно.")
        print(result.stdout)
        return 0
    else:
        print("Ошибка сборки.")
        print(result.stderr)
        return -1

def build_unpacker():
    solution_path = ".\\Win32\\src\\unpacker\\unpacker.vcxproj"
    configuration = "Release"
    platform = "Win32"
    return build_project(solution_path, configuration, platform)

def build_unpacker_converter():
    solution_path = "Win32\\src\\unpacker_converter\\unpacker_converter.vcxproj"
    configuration = "Debug"
    platform = "x64"
    return build_project(solution_path, configuration, platform)

def build_packer():
    solution_path = "Win32\\src\\simple_packer\\simple_packer.vcxproj"
    configuration = "Debug"
    platform = "x64"
    return build_project(solution_path, configuration, platform)

def main():
    parser = argparse.ArgumentParser(description="Сборка и упаковка проекта.")
    parser.add_argument("file_path", type=str, help="Путь к файлу, который нужно упаковать.")
    args = parser.parse_args()
    input_file_path = args.file_path  

    if (build_unpacker()):
        print('Error build unpacker')
        return -1

    if (build_unpacker_converter()):
        print('Error build unpacker_converter')
        return -1

    build_command = PATH_CONVERTER + ' ' + PATH_UNPACKER + ' ' + PATH_OUTPUT_CONVERTER
    result = subprocess.run(build_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if result.returncode == 0:
        print("Сборка прошла успешно.")
        print(result.stdout)
    else:
        print("Ошибка сборки.")
        print(result.stderr)
        return -1

    if (build_packer()):
        print('Error build packer')
        return -1

    packer_exe = '.\\Win32\\src\\simple_packer\\x64\\Debug\\simple_packer.exe'
    build_command = packer_exe + ' ' + input_file_path
    print(build_command)
    result = subprocess.run(build_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if result.returncode == 0:
        print("Сборка прошла успешно.")
        print(result.stdout)
    else:
        print("Ошибка сборки.")
        print(result.stderr)
        return -1

if __name__ == '__main__':
    main()
