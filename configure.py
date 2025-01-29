#!/usr/bin/env python3

###
# Generates build files for the project.
# This file also includes the project configuration,
# such as compiler flags and the object matching status.
#
# Usage:
#   python3 configure.py
#   ninja
#
# Append --help to see available options.
###

import argparse
import sys
from pathlib import Path
from typing import Any, Dict, List

from tools.project import (
    Object,
    ProjectConfig,
    calculate_progress,
    generate_build,
    is_windows,
)

# Game versions
DEFAULT_VERSION = 0
VERSIONS = [
    "GP6E01",  # 0
]

parser = argparse.ArgumentParser()
parser.add_argument(
    "mode",
    choices=["configure", "progress"],
    default="configure",
    help="script mode (default: configure)",
    nargs="?",
)
parser.add_argument(
    "-v",
    "--version",
    choices=VERSIONS,
    type=str.upper,
    default=VERSIONS[DEFAULT_VERSION],
    help="version to build",
)
parser.add_argument(
    "--build-dir",
    metavar="DIR",
    type=Path,
    default=Path("build"),
    help="base build directory (default: build)",
)
parser.add_argument(
    "--binutils",
    metavar="BINARY",
    type=Path,
    help="path to binutils (optional)",
)
parser.add_argument(
    "--compilers",
    metavar="DIR",
    type=Path,
    help="path to compilers (optional)",
)
parser.add_argument(
    "--map",
    action="store_true",
    help="generate map file(s)",
)
parser.add_argument(
    "--no-asm",
    action="store_true",
    help="don't incorporate .s files from asm directory",
)
parser.add_argument(
    "--debug",
    action="store_true",
    help="build with debug info (non-matching)",
)
if not is_windows():
    parser.add_argument(
        "--wrapper",
        metavar="BINARY",
        type=Path,
        help="path to wibo or wine (optional)",
    )
parser.add_argument(
    "--dtk",
    metavar="BINARY | DIR",
    type=Path,
    help="path to decomp-toolkit binary or source (optional)",
)
parser.add_argument(
    "--sjiswrap",
    metavar="EXE",
    type=Path,
    help="path to sjiswrap.exe (optional)",
)
parser.add_argument(
    "--verbose",
    action="store_true",
    help="print verbose output",
)
parser.add_argument(
    "--non-matching",
    dest="non_matching",
    action="store_true",
    help="builds equivalent (but non-matching) or modded objects",
)
args = parser.parse_args()

config = ProjectConfig()
config.version = str(args.version)
version_num = VERSIONS.index(config.version)

# Apply arguments
config.build_dir = args.build_dir
config.dtk_path = args.dtk
config.binutils_path = args.binutils
config.compilers_path = args.compilers
config.debug = args.debug
config.generate_map = args.map
config.non_matching = args.non_matching
config.sjiswrap_path = args.sjiswrap
if not is_windows():
    config.wrapper = args.wrapper
if args.no_asm:
    config.asm_dir = None

# Tool versions
config.binutils_tag = "2.42-1"
config.compilers_tag = "20231018"
config.dtk_tag = "v0.9.2"
config.sjiswrap_tag = "v1.1.1"
config.wibo_tag = "0.6.11"

# Project
config.config_path = Path("config") / config.version / "config.yml"
config.check_sha_path = Path("config") / config.version / "build.sha1"
config.asflags = [
    "-mgekko",
    "--strip-local-absolute",
    "-I include",
    f"-I build/{config.version}/include",
    f"--defsym version={version_num}",
]
config.ldflags = [
    "-fp hardware",
    "-nodefaults",
    # "-listclosure", # Uncomment for Wii linkers
]
# Use for any additional files that should cause a re-configure when modified
config.reconfig_deps = []

# Base flags, common to most GC/Wii games.
# Generally leave untouched, with overrides added below.
cflags_base = [
    "-nodefaults",
    "-proc gekko",
    "-align powerpc",
    "-enum int",
    "-fp hardware",
    "-Cpp_exceptions off",
    # "-W all",
    "-O4,p",
    "-inline auto",
    '-pragma "cats off"',
    '-pragma "warn_notinlined off"',
    "-maxerrors 1",
    "-nosyspath",
    "-RTTI off",
    "-fp_contract on",
    "-str reuse",
    "-multibyte",  # For Wii compilers, replace with `-enc SJIS`
    "-i include",
    f"-i build/{config.version}/include",
    f"-DVERSION={version_num}",
]

# Debug flags
if config.debug:
    cflags_base.extend(["-sym on", "-DDEBUG=1"])
else:
    cflags_base.append("-DNDEBUG=1")

# Metrowerks library flags
cflags_runtime = [
    *cflags_base,
    "-use_lmw_stmw on",
    "-str reuse,pool,readonly",
    "-gccinc",
    "-common off",
    "-inline auto",
]

# REL flags
cflags_rel = [
    *cflags_base,
    "-O0,p",
    "-sdata 0",
    "-sdata2 0",
]

# Game flags
cflags_game = [
    *cflags_base,
    "-O0,p",
    "-char unsigned",
    "-fp_contract off",
]

# Zlib flags
cflags_zlib = [
    *cflags_base,
    "-O0,p",
    "-fp_contract off",
]


# Game flags
cflags_libhu = [
    *cflags_base,
    "-O0,p",
    "-char unsigned",
    "-fp_contract off",
]

# Game flags
cflags_msm = [
    *cflags_base,
]


config.linker_version = "GC/2.6"
config.rel_strip_partial = False
config.rel_empty_file = "REL/empty.c"

# Helper function for Dolphin libraries
def DolphinLib(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_base,
        "host": False,
        "objects": objects,
    }


# Helper function for REL script objects
def Rel(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.3.2",
        "cflags": cflags_rel,
        "host": True,
        "objects": objects,
    }


Matching = True                   # Object matches and should be linked
NonMatching = False               # Object does not match and should not be linked
Equivalent = config.non_matching  # Object should be linked when configured with --non-matching

config.warn_missing_config = True
config.warn_missing_source = False
config.libs = [
    {
        "lib": "Runtime.PPCEABI.H",
        "mw_version": config.linker_version,
        "cflags": cflags_runtime,
        "host": False,
        "objects": [
            Object(NonMatching, "Runtime.PPCEABI.H/global_destructor_chain.c"),
            Object(NonMatching, "Runtime.PPCEABI.H/__init_cpp_exceptions.cpp"),
        ],
    },
    {
        "lib": "Game",
        "mw_version": config.linker_version,
        "cflags": cflags_game,
        "host": False,
        "objects": [
            Object(NonMatching, "game/main.c"),
            Object(NonMatching, "game/pad.c"),
            Object(NonMatching, "game/dvd.c"),
            Object(NonMatching, "game/data.c"),
            Object(NonMatching, "game/decode.c"),
            Object(NonMatching, "game/font.c"),
            Object(NonMatching, "game/init.c"),
            Object(NonMatching, "game/jmp.c"),
            Object(NonMatching, "game/malloc.c"),
            Object(NonMatching, "game/memory.c"),
            Object(NonMatching, "game/printfunc.c"),
            Object(NonMatching, "game/process.c"),
            Object(NonMatching, "game/sprman.c"),
            Object(NonMatching, "game/sprput.c"),
            Object(NonMatching, "game/hsfload.c"),
            Object(NonMatching, "game/hsfdraw.c"),
            Object(NonMatching, "game/hsfman.c"),
            Object(NonMatching, "game/hsfmotion.c"),
            Object(NonMatching, "game/hsfanim.c"),
            Object(NonMatching, "game/hsfex.c"),
            Object(NonMatching, "game/perf.c"),
            Object(NonMatching, "game/objmain.c"),
            Object(NonMatching, "game/fault.c"),
            Object(NonMatching, "game/gamework.c"),
            Object(NonMatching, "game/objsysobj.c"),
            Object(NonMatching, "game/objdll.c"),
            Object(NonMatching, "game/frand.c"),
            Object(NonMatching, "game/audio.c"),
            Object(NonMatching, "game/EnvelopeExec.c"),
            Object(NonMatching, "game/gamemes.c"),
            Object(NonMatching, "game/esprite.c"),
            Object(NonMatching, "game/ovllist.c"),
            Object(NonMatching, "game/ClusterExec.c"),
            Object(NonMatching, "game/ShapeExec.c"),
            Object(NonMatching, "game/wipe.c"),
            Object(NonMatching, "game/window.c"),
            Object(NonMatching, "game/card.c"),
            Object(NonMatching, "game/armem.c"),
            Object(NonMatching, "game/charman.c"),
            Object(NonMatching, "game/mapspace.c"),
            Object(NonMatching, "game/THPSimple.c"),
            Object(NonMatching, "game/THPDraw.c"),
            Object(NonMatching, "game/thpmain.c"),
            Object(NonMatching, "game/mgdata.c"),
            Object(NonMatching, "game/objsub.c"),
            Object(NonMatching, "game/flag.c"),
            Object(NonMatching, "game/saveload.c"),
            Object(NonMatching, "game/sreset.c"),
            Object(NonMatching, "game/mgtimer.c"),
            Object(NonMatching, "game/mgscore.c"),
            Object(NonMatching, "game/seqman.c"),
            Object(NonMatching, "game/colman.c"),
            Object(NonMatching, "game/actman.c"),
            Object(NonMatching, "game/mggamemes.c"),
            Object(NonMatching, "game/mic.c"),
            Object(NonMatching, "game/code_80146BA0.c"),
            Object(NonMatching, "game/kerent.c"),
        ],
    },
    {
        "lib": "libhu",
        "mw_version": config.linker_version,
        "cflags": cflags_libhu,
        "host": False,
        "objects": [
            Object(NonMatching, "libhu/setvf.c"),
            Object(NonMatching, "libhu/subvf.c"),
        ],
    },
    {
        "lib": "msm",
        "mw_version": "GC/1.2.5",
        "cflags": cflags_msm,
        "host": False,
        "objects": [
            Object(NonMatching, "msm/msmsys.c"),
            Object(NonMatching, "msm/msmmem.c"),
            Object(NonMatching, "msm/msmfio.c"),
            Object(NonMatching, "msm/msmmus.c"),
            Object(NonMatching, "msm/msmse.c"),
            Object(NonMatching, "msm/msmstream.c"),
        ],
    },
    {
        "lib": "zlib",
        "mw_version": config.linker_version,
        "cflags": cflags_zlib,
        "host": False,
        "objects": [
            Object(NonMatching, "zlib/adler32.c"),
            Object(NonMatching, "zlib/inflate.c"),
            Object(NonMatching, "zlib/infblock.c"),
            Object(NonMatching, "zlib/infcodes.c"),
            Object(NonMatching, "zlib/infutil.c"),
            Object(NonMatching, "zlib/inftrees.c"),
            Object(NonMatching, "zlib/inffast.c"),
            Object(NonMatching, "zlib/zutil.c"),
        ],
    },
    {
        "lib": "board",
        "mw_version": config.linker_version,
        "cflags": cflags_game,
        "host": False,
        "objects": [
            Object(NonMatching, "board/pausewatch.c"),
            Object(NonMatching, "board/main.c"),
            Object(NonMatching, "board/math.c"),
            Object(NonMatching, "board/camera.c"),
            Object(NonMatching, "board/player.c"),
            Object(NonMatching, "board/snpc.c"),
            Object(NonMatching, "board/model.c"),
            Object(NonMatching, "board/window.c"),
            Object(NonMatching, "board/audio.c"),
            Object(NonMatching, "board/comchoice.c"),
            Object(NonMatching, "board/scroll.c"),
            Object(NonMatching, "board/masu.c"),
            Object(NonMatching, "board/coin.c"),
            Object(NonMatching, "board/star.c"),
            Object(NonMatching, "board/padall.c"),
            Object(NonMatching, "board/sai.c"),
            Object(NonMatching, "board/status.c"),
            Object(NonMatching, "board/opening.c"),
            Object(NonMatching, "board/pause.c"),
            Object(NonMatching, "board/tutorial.c"),
            Object(NonMatching, "board/roulette.c"),
            Object(NonMatching, "board/capsulesel.c"),
            Object(NonMatching, "board/capsule0.c"),
            Object(NonMatching, "board/capsule1.c"),
            Object(NonMatching, "board/capsule2.c"),
            Object(NonMatching, "board/capsule3.c"),
            Object(NonMatching, "board/capsule.c"),
            Object(NonMatching, "board/capsulemain.c"),
        ],
    },
    {
        "lib": "REL",
        "mw_version": config.linker_version,
        "cflags": cflags_rel,
        "host": False,
        "objects": [
            Object(Matching, "REL/empty.c"),  # Must be marked as matching
        ],
    },
]

if args.mode == "configure":
    # Write build.ninja and objdiff.json
    generate_build(config)
elif args.mode == "progress":
    # Print progress and write progress.json
    config.progress_each_module = args.verbose
    calculate_progress(config)
else:
    sys.exit("Unknown mode: " + args.mode)
