# Keeps the two copies of the program the same.
#
#   python3 sync_files.py from-py        after editing cascade_robot_auton.py
#   python3 sync_files.py from-vexcode   after editing and saving in VEXcode
#   python3 sync_files.py check          see if they match
#
# The .v5python file is what VEXcode opens. It's JSON with the whole
# program stored inside it as one string ("textContent"), so GitHub can't
# show it nicely - that's why there's also a plain .py copy.
#
# This runs on your computer, not on the robot.

import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PY_FILE = os.path.join(HERE, "cascade_robot_auton.py")
VEXCODE_FILE = os.path.join(HERE, "Cascade Robot Auton.v5python")


def read_py():
    with open(PY_FILE, encoding="utf-8", newline="") as f:
        return f.read()


def read_project():
    with open(VEXCODE_FILE, encoding="utf-8") as f:
        return json.load(f)


def from_py():
    project = read_project()
    project["textContent"] = read_py()
    with open(VEXCODE_FILE, "w", encoding="utf-8") as f:
        json.dump(project, f, separators=(",", ":"))
    print("Updated " + os.path.basename(VEXCODE_FILE))


def from_vexcode():
    code = read_project()["textContent"]
    with open(PY_FILE, "w", encoding="utf-8", newline="") as f:
        f.write(code)
    print("Updated " + os.path.basename(PY_FILE))


def check():
    if read_project()["textContent"] == read_py():
        print("The .py and .v5python files match.")
        return 0
    print("The .py and .v5python files are different.")
    print("Run 'python3 sync_files.py from-py' or 'python3 sync_files.py from-vexcode'.")
    return 1


if __name__ == "__main__":
    command = sys.argv[1] if len(sys.argv) > 1 else ""
    if command == "from-py":
        from_py()
    elif command == "from-vexcode":
        from_vexcode()
    elif command == "check":
        sys.exit(check())
    else:
        print("Usage: python3 sync_files.py from-py | from-vexcode | check")
        sys.exit(1)
