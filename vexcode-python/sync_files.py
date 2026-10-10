# Keeps the two copies of each program the same.
#
#   python3 sync_files.py from-py        after editing a .py file
#   python3 sync_files.py from-vexcode   after editing and saving in VEXcode
#   python3 sync_files.py check          see if they match
#
# The .v5python file is what VEXcode opens. It's JSON with the whole
# program stored inside it as one string ("textContent"), so GitHub can't
# show it nicely - that's why there's also a plain .py copy.
#
# Each command does all five programs (match, motor test, drive, drive v2 and
# the amps measuring tool).
# This runs on your computer, not on the robot.

import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PROGRAMS = [
    ("cascade_robot_auton.py", "Cascade Robot Auton.v5python"),
    ("cascade_robot_test.py", "Cascade Robot Test.v5python"),
    ("cascade_robot_drive.py", "Cascade Robot Drive.v5python"),
    ("cascade_robot_drive_v2.py", "Cascade Robot Drive V2.v5python"),
    ("cascade_robot_amps.py", "Cascade Robot Amps.v5python"),
    ("cascade_robot_drive_limits.py", "Cascade Robot Drive Limits.v5python"),
]


def read_py(py_file):
    with open(os.path.join(HERE, py_file), encoding="utf-8", newline="") as f:
        return f.read()


def read_project(vexcode_file):
    with open(os.path.join(HERE, vexcode_file), encoding="utf-8") as f:
        return json.load(f)


def from_py(py_file, vexcode_file):
    project = read_project(vexcode_file)
    project["textContent"] = read_py(py_file)
    with open(os.path.join(HERE, vexcode_file), "w", encoding="utf-8") as f:
        json.dump(project, f, separators=(",", ":"))
    print("Updated " + vexcode_file)


def from_vexcode(py_file, vexcode_file):
    code = read_project(vexcode_file)["textContent"]
    with open(os.path.join(HERE, py_file), "w", encoding="utf-8", newline="") as f:
        f.write(code)
    print("Updated " + py_file)


def check(py_file, vexcode_file):
    if read_project(vexcode_file)["textContent"] == read_py(py_file):
        print("Same:      " + py_file + "  and  " + vexcode_file)
        return True
    print("DIFFERENT: " + py_file + "  and  " + vexcode_file)
    return False


if __name__ == "__main__":
    command = sys.argv[1] if len(sys.argv) > 1 else ""
    if command == "from-py":
        for py_file, vexcode_file in PROGRAMS:
            from_py(py_file, vexcode_file)
    elif command == "from-vexcode":
        for py_file, vexcode_file in PROGRAMS:
            from_vexcode(py_file, vexcode_file)
    elif command == "check":
        results = [check(py_file, vexcode_file) for py_file, vexcode_file in PROGRAMS]
        if not all(results):
            print("Run 'python3 sync_files.py from-py' or 'python3 sync_files.py from-vexcode'.")
            sys.exit(1)
    else:
        print("Usage: python3 sync_files.py from-py | from-vexcode | check")
        sys.exit(1)
