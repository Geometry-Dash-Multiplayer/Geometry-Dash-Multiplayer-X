import argparse
from pathlib import Path
from subprocess import run

parser = argparse.ArgumentParser("Push mod to device")
parser.add_argument("arch", choices=("aarch64", "arm"))
parser.add_argument(
  '-r', "--release", 
  action="store_true", 
  help="Whether to use the release version of the mod"
)
parser.add_argument(
  '-d', "--destination", type=str,
  default="/storage/emulated/0/Android/media/com.geode.launcher/game/geode/mods/",
  help="The target directory to which the mod will be pushed"
)
args = parser.parse_args()

preset = f"android-{args.arch}-{'release' if args.release else 'debug'}"
root = Path(run(["git", "rev-parse", "--show-toplevel"], capture_output=True).stdout.decode().strip())
mod = root.resolve() / "out" / "build" / preset / "angel.gdmx.geode"
run(["adb", "push", str(mod), args.destination])
