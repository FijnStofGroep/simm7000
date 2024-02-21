Import("env")
import hashlib
import os
import shutil
from datetime import datetime

def _file_md5_hexdigest(fname):
    return hashlib.md5(open(fname, 'rb').read()).hexdigest()

#!after_build function()
def after_build(source, target, env):
    if not os.path.exists("builds"):
        os.mkdir("builds")

    date = datetime.now().strftime("%Y_%m_%d")

    lang = env.GetProjectOption('lang')
    target_name = lang.lower()

    print("\n**** After build \"airrohr-firmware => SEN5X sensor\" **** ")
 
    with open(f"builds/latest_{target_name}.bin.md5", "w") as md5:
        print(_file_md5_hexdigest(target[0].path), file = md5)

    with open(f"builds/latest_{target_name}_{date}.bin.md5", "w") as md5:
        print(_file_md5_hexdigest(target[0].path), file = md5)

    shutil.copy(target[0].path, f"builds/latest_{target_name}.bin")
    shutil.copy(target[0].path, f"builds/latest_{target_name}_{date}.bin")

    print( f"Copy: [\"{target[0].path}\"] To " + f"[\"./builds/latest_{target_name}.bin\"]")
    print( f"Copy: [\"{target[0].path}\"] To " + f"[\"./builds/latest_{target_name}_{date}.bin\"]")
     
env.AddPostAction("$BUILD_DIR/firmware.bin", after_build)
