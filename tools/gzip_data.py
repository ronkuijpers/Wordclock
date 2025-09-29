Import("env")

import os
import gzip
import shutil


TEXT_EXTS = {
    ".html", ".htm", ".css", ".js", ".json", ".svg", ".txt", ".map"
}


def should_compress(path):
    if os.environ.get("WORDCLOCK_DISABLE_GZIP"):
        return False
    if path.endswith('.gz'):
        return False
    _, ext = os.path.splitext(path)
    return ext.lower() in TEXT_EXTS


def compress_if_needed(src):
    dst = src + ".gz"
    try:
        if os.path.exists(dst) and os.path.getmtime(dst) >= os.path.getmtime(src):
            return False
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        with open(src, 'rb') as fin, open(dst, 'wb') as raw_out:
            with gzip.GzipFile(filename=os.path.basename(src), mode='wb',
                               fileobj=raw_out, compresslevel=9, mtime=0) as fout:
                shutil.copyfileobj(fin, fout)
        print(f"[gzip_data] Compressed: {src} -> {dst}")
        return True
    except Exception as e:
        print(f"[gzip_data] Failed to compress {src}: {e}")
        return False


def run_gzip_on_data(*args, **kwargs):
    data_dir = env.subst("$PROJECT_DATA_DIR")
    if not data_dir or data_dir == "$PROJECT_DATA_DIR":
        data_dir = os.path.join(env["PROJECT_DIR"], "data")
    if not os.path.isdir(data_dir):
        print(f"[gzip_data] No data dir: {data_dir}")
        return
    changed = 0
    for root, _, files in os.walk(data_dir):
        for name in files:
            src = os.path.join(root, name)
            if should_compress(src):
                if compress_if_needed(src):
                    changed += 1
    print(f"[gzip_data] Done. {changed} file(s) compressed.")


# Run before building or uploading FS so .gz files are included
env.AddPreAction("buildfs", run_gzip_on_data)
env.AddPreAction("uploadfs", run_gzip_on_data)
