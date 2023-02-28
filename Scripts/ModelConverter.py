import os
import shutil

SRC_PATH = "D:\\Code\\Cpp\\renderer\\assets"

DST_PATH = "C:\\Users\\Geek\\Desktop\\dst"


def tga2dds(src_path, dst_path, filename):

    src_file = os.path.join(src_path, filename)

    command = f"texconv  -o {dst_path} {src_file} -y"

    os.system(command)


def convertMetaFile(path, file, dst_path):
    src_file = os.path.join(path, file)
    dst_file = os.path.join(dst_path, file)

    with open(src_file, "r") as src:
        with open(dst_file, "w") as dst:
            for line in src.readlines():

                if ".tga" in line:

                    line = line.replace(".tga", ".dds")
                print(line, file=dst, end="")


def copyFile(src_path, dst_path, filename):

    src = os.path.join(src_path, filename)

    dst = os.path.join(dst_path, filename)

    shutil.copyfile(src, dst)


def createDir(path):
    if not os.path.exists(path):
        os.mkdir(path)


def convertModel(path, name):

    files = os.listdir(path)
    dst_path = os.path.join(DST_PATH, name)
    createDir(dst_path)
    for file in files:

        if file.endswith(".scn"):

            convertMetaFile(path, file, dst_path)

        elif file.endswith(".tga"):

            tga2dds(path, dst_path, file)
        else:

            copyFile(path, dst_path, file)


def convertModels(path):
    files = os.listdir(path)
    for file in files:

        abspath = os.path.join(path, file)

        if os.path.isdir(abspath):

            convertModel(abspath, file)


if __name__ == "__main__":

    convertModels(SRC_PATH)
