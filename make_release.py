# -*- coding: utf-8 -*-
"""
PRA 发版打包脚本（纯 Python，UTF-8）。

职责：
  1. 从 D:/PRA/src/update.h 读取 PRA_VERSION（版本号的唯一真相来源）
  2. 计算 D:/PRA/PRA.exe 的 md5 + 字节数
  3. 写出本目录的 version.json（{"version","md5","size"}）
  4. 把 PRA.exe 复制成 PRA.dat 和 PRA.exe（PRA.dat 用于 jsDelivr，.exe 留给旧客户端/直接下载）

用法：在发布目录里运行  python make_release.py
前置：先改 update.h 的 PRA_VERSION、用 build.bat 重新编译出 D:/PRA/PRA.exe。
"""
import hashlib, json, os, re, shutil, sys

SRC_EXE = "D:/PRA/PRA.exe"
SRC_H   = "D:/PRA/src/update.h"
PUB     = os.path.dirname(os.path.abspath(__file__))


def extract_version(hpath):
    txt = open(hpath, encoding="utf-8").read()
    m = re.search(r'#define\s+PRA_VERSION\s+"([^"]+)"', txt)
    if not m:
        raise SystemExit("[错误] 在 update.h 找不到 PRA_VERSION 宏")
    return m.group(1)


def main():
    if not os.path.exists(SRC_EXE):
        raise SystemExit("[错误] 找不到 " + SRC_EXE + "，请先用 build.bat 编译 PRA.exe")
    ver = extract_version(SRC_H)
    data = open(SRC_EXE, "rb").read()
    md5 = hashlib.md5(data).hexdigest()
    size = len(data)
    info = {"version": ver, "md5": md5, "size": size}
    with open(os.path.join(PUB, "version.json"), "w", encoding="utf-8") as f:
        f.write(json.dumps(info, ensure_ascii=False) + "\n")
    shutil.copy(SRC_EXE, os.path.join(PUB, "PRA.dat"))
    shutil.copy(SRC_EXE, os.path.join(PUB, "PRA.exe"))
    print("[OK] version =", ver)
    print("[OK] md5     =", md5)
    print("[OK] size    =", size)
    print("[OK] 已写出:", os.path.join(PUB, "version.json"), "PRA.dat", "PRA.exe")


if __name__ == "__main__":
    main()
