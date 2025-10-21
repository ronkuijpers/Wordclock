Import("env")

from datetime import datetime, timezone
import os
import subprocess


def get_git_sha():
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            stderr=subprocess.DEVNULL
        ).decode().strip()
    except Exception:
        return "unknown"


def write_build_info(*args, **kwargs):
    sha = get_git_sha()
    build_time = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    env_name = env.get("PIOENV", "unknown")

    content = f"""#pragma once

#define BUILD_GIT_SHA "{sha}"
#define BUILD_TIME_UTC "{build_time}"
#define BUILD_ENV_NAME "{env_name}"
"""
    include_dir = env.subst("$PROJECT_INCLUDE_DIR")
    os.makedirs(include_dir, exist_ok=True)
    path = os.path.join(include_dir, "build_info.h")

    with open(path, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"[buildinfo] SHA={sha} env={env_name} time={build_time}")


env.AddPreAction("buildprog", write_build_info)
env.AddPreAction("buildfs", write_build_info)
