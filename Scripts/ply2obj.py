import os
from pathlib import Path
import aspose.threed as a3d

SRC_PATH = r"C:\Users\Geek\Desktop\Assets"
DST_PATH = r"D:\Code\Cpp\BeaconRenderer\Assets\stanford"

files = os.listdir(SRC_PATH)
for file in files:
    if file[-4:]=='.ply':
        path = str(Path(SRC_PATH)/file)
        scene = a3d.Scene.from_file(path)
        if not os.path.exists(DST_PATH):  #判断是否存在文件夹如果不存在则创建为文件夹
            os.makedirs(DST_PATH)
        dst = str(Path(DST_PATH)/file.replace('.ply','.obj'))
        scene.save(dst)

## ERROR 导入的模型显示异常，又重新使用Blender进行转化

