import sys

filename = ""

args = sys.argv
if len(args) >= 2:
    filename = args[1]

COMMENT_BEGIN_MARKER = "PUBLIC_EXCLUDE_BEGIN"
COMMENT_END_MARKER = "PUBLIC_EXCLUDE_END"

lines = []
comment_depth = 0
with open(filename, "r") as file:
    for line in file:
        if COMMENT_BEGIN_MARKER in line:
            comment_depth = comment_depth + 1
        if comment_depth == 0:
            lines.append(line)
        if COMMENT_END_MARKER in line:
            comment_depth = comment_depth - 1

if len(lines) > 0:
    with open(filename, "w") as file:
        for line in lines:
            file.write(line)
