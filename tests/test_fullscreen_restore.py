#!/usr/bin/env python3
"""Compile the real transition functions against lightweight compositor mocks.

Run: python3 tests/test_fullscreen_restore.py (requires a C compiler).
This checks state/geometry logic, not live Wayland or XWayland protocol behavior.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def extract_function(source, name):
    match = re.search(r"^(?:static )?void " + name + r"\s*\(", source, re.M)
    if match is None:
        raise ValueError(f"Missing function: {name}")
    opening = source.index("{", match.start())
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


source = (ROOT / "src/manage/client.c").read_text()
functions = "\n\n".join(extract_function(source, name) for name in (
    "client_set_monitor", "client_set_floating", "client_apply_fullscreen", "client_set_fake_fullscreen",
    "maximize_screen_gaps", "client_set_maximize_screen", "clear_fullscreen_flag",
))
# Exercise the production state-transfer/forced-cleanup blocks without mocking
# an entire unmap or scroller scene graph.
swallow_start = source.index("\tif (c->swallowing) {\n\t\tclient_set_maximize_screen")
swallow_end = source.index("\n\t}\n", swallow_start) + len("\n\t}")
functions += "\nvoid sync_swallow(Client *c) {\n" + source[swallow_start:swallow_end] + "\n}\n"
scroll = (ROOT / "src/layout/scroll.c").read_text()
head_start = scroll.index("\tstack_head->fullscreen_was_maximized = false;")
head_end = scroll.index("\n\t/* Syncs to the Client fields. */", head_start)
functions += "\nvoid clear_stack_head(Client *stack_head) {\n" + scroll[head_start:head_end] + "\n}\n"
fixture = (ROOT / "tests/fullscreen_restore.c").read_text()
with tempfile.TemporaryDirectory(prefix="mango-fullscreen-test-") as directory:
    c_file = Path(directory) / "test.c"
    binary = Path(directory) / "test"
    c_file.write_text(fixture.replace("/* TRANSITION_FUNCTIONS */", functions))
    subprocess.run(shlex.split(os.environ.get("CC", "cc")) + [
        "-std=c11", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter",
        str(c_file), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
