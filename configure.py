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
config.compilers_tag = "20240706"
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
    "-DMUSY_TARGET=MUSY_TARGET_DOLPHIN",
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
    "-char unsigned",
    "-fp_contract off",
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


# Mic SDK flags
cflags_dolphin = [
    *cflags_base,
]

cflags_thp = [
    *cflags_base,
]


# Mic SDK flags
cflags_sdk_mic = [
    *cflags_base,
]

# Game Speech SDK flags
cflags_gssdk = [
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
        "cflags": cflags_dolphin,
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
            Object(NonMatching, "Runtime.PPCEABI.H/runtime.c"),
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
            Object(Matching, "game/pad.c"),
            Object(Matching, "game/dvd.c"),
            Object(Matching, "game/data.c"),
            Object(Matching, "game/decode.c"),
            Object(Matching, "game/font.c"),
            Object(Matching, "game/init.c"),
            Object(Matching, "game/jmp.c"),
            Object(Matching, "game/malloc.c"),
            Object(Matching, "game/memory.c"),
            Object(Matching, "game/printfunc.c"),
            Object(Matching, "game/process.c"),
            Object(Matching, "game/sprman.c"),
            Object(Matching, "game/sprput.c"),
            Object(Matching, "game/hsfload.c"),
            Object(Equivalent, "game/hsfdraw.c"),
            Object(Matching, "game/hsfman.c"),
            Object(Matching, "game/hsfmotion.c"),
            Object(Matching, "game/hsfanim.c"),
            Object(Matching, "game/hsfex.c"),
            Object(Matching, "game/perf.c"),
            Object(NonMatching, "game/objmain.c"),
            Object(Matching, "game/fault.c"),
            Object(Matching, "game/gamework.c"),
            Object(Matching, "game/objsysobj.c"),
            Object(Matching, "game/objdll.c"),
            Object(Matching, "game/frand.c"),
            Object(Matching, "game/audio.c"),
            Object(Matching, "game/EnvelopeExec.c"),
            Object(NonMatching, "game/gamemes.c"),
            Object(Matching, "game/esprite.c"),
            Object(Matching, "game/ovllist.c"),
            Object(Matching, "game/ClusterExec.c"),
            Object(Matching, "game/ShapeExec.c"),
            Object(NonMatching, "game/wipe.c"),
            Object(Matching, "game/window.c"),
            Object(Matching, "game/card.c"),
            Object(Matching, "game/armem.c"),
            Object(NonMatching, "game/charman.c"),
            Object(Matching, "game/mapspace.c"),
            Object(Matching, "game/THPSimple.c"),
            Object(Matching, "game/THPDraw.c"),
            Object(Matching, "game/thpmain.c"),
            Object(NonMatching, "game/mgdata.c"),
            Object(NonMatching, "game/objsub.c"),
            Object(Matching, "game/flag.c"),
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
    DolphinLib(
        "base",
        [
            Object(NonMatching, "dolphin/base/PPCArch.c"),
        ],
    ),
    DolphinLib(
        "os",
        [
            Object(NonMatching, "dolphin/os/OS.c"),
            Object(NonMatching, "dolphin/os/OSAlarm.c"),
            Object(NonMatching, "dolphin/os/OSAlloc.c"),
            Object(NonMatching, "dolphin/os/OSArena.c"),
            Object(NonMatching, "dolphin/os/OSAudioSystem.c"),
            Object(NonMatching, "dolphin/os/OSCache.c"),
            Object(NonMatching, "dolphin/os/OSContext.c"),
            Object(NonMatching, "dolphin/os/OSError.c"),
            Object(NonMatching, "dolphin/os/OSExec.c"),
            Object(NonMatching, "dolphin/os/OSFont.c"),
            Object(NonMatching, "dolphin/os/OSInterrupt.c"),
            Object(NonMatching, "dolphin/os/OSLink.c"),
            Object(NonMatching, "dolphin/os/OSMessage.c"),
            Object(NonMatching, "dolphin/os/OSMemory.c"),
            Object(NonMatching, "dolphin/os/OSMutex.c"),
            Object(NonMatching, "dolphin/os/OSReboot.c"),
            Object(NonMatching, "dolphin/os/OSReset.c"),
            Object(NonMatching, "dolphin/os/OSResetSW.c"),
            Object(NonMatching, "dolphin/os/OSRtc.c"),
            Object(NonMatching, "dolphin/os/OSSemaphore.c"),
            Object(NonMatching, "dolphin/os/OSStopwatch.c"),
            Object(NonMatching, "dolphin/os/OSSync.c"),
            Object(NonMatching, "dolphin/os/OSThread.c"),
            Object(NonMatching, "dolphin/os/OSTime.c"),
            Object(NonMatching, "dolphin/os/__start.c"),
            Object(NonMatching, "dolphin/os/__ppc_eabi_init.c"),
        ],
    ),
    DolphinLib(
        "db",
        [
            Object(NonMatching, "dolphin/db/db.c"),
        ],
    ),
    DolphinLib(
        "mtx",
        [
            Object(NonMatching, "dolphin/mtx/mtx.c"),
            Object(NonMatching, "dolphin/mtx/mtxvec.c"),
            Object(NonMatching, "dolphin/mtx/mtx44.c"),
            Object(NonMatching, "dolphin/mtx/mtx44vec.c"),
            Object(NonMatching, "dolphin/mtx/vec.c"),
            Object(NonMatching, "dolphin/mtx/quat.c"),
            Object(NonMatching, "dolphin/mtx/psmtx.c"),
        ],
    ),
    DolphinLib(
        "dvd",
        [
            Object(NonMatching, "dolphin/dvd/dvdlow.c"),
            Object(NonMatching, "dolphin/dvd/dvdfs.c"),
            Object(NonMatching, "dolphin/dvd/dvd.c"),
            Object(NonMatching, "dolphin/dvd/dvdqueue.c"),
            Object(NonMatching, "dolphin/dvd/dvderror.c"),
            Object(NonMatching, "dolphin/dvd/dvdidutils.c"),
            Object(NonMatching, "dolphin/dvd/dvdFatal.c"),
            Object(NonMatching, "dolphin/dvd/fstload.c"),
        ],
    ),
    DolphinLib(
        "vi",
        [
            Object(NonMatching, "dolphin/vi/vi.c"),
        ],
    ),
    DolphinLib(
        "demo",
        [
            Object(NonMatching, "dolphin/demo/DEMOInit.c"),
            Object(NonMatching, "dolphin/demo/DEMOFont.c"),
            Object(NonMatching, "dolphin/demo/DEMOPuts.c"),
            Object(NonMatching, "dolphin/demo/DEMOStats.c"),
        ],
    ),
    DolphinLib(
        "pad",
        [
            Object(NonMatching, "dolphin/pad/Padclamp.c"),
            Object(NonMatching, "dolphin/pad/Pad.c"),
        ],
    ),
    DolphinLib(
        "ai",
        [
            Object(NonMatching, "dolphin/ai/ai.c"),
        ],
    ),
    DolphinLib(
        "ar",
        [
            Object(NonMatching, "dolphin/ar/ar.c"),
            Object(NonMatching, "dolphin/ar/arq.c"),
        ],
    ),
    DolphinLib(
        "dsp",
        [
            Object(NonMatching, "dolphin/dsp/dsp.c"),
            Object(NonMatching, "dolphin/dsp/dsp_debug.c"),
            Object(NonMatching, "dolphin/dsp/dsp_task.c"),
        ],
    ),
    DolphinLib(
        "gx",
        [
            Object(NonMatching, "dolphin/gx/GXInit.c"),
            Object(NonMatching, "dolphin/gx/GXFifo.c"),
            Object(NonMatching, "dolphin/gx/GXAttr.c"),
            Object(NonMatching, "dolphin/gx/GXMisc.c"),
            Object(NonMatching, "dolphin/gx/GXGeometry.c"),
            Object(NonMatching, "dolphin/gx/GXFrameBuf.c"),
            Object(NonMatching, "dolphin/gx/GXLight.c"),
            Object(NonMatching, "dolphin/gx/GXTexture.c"),
            Object(NonMatching, "dolphin/gx/GXBump.c"),
            Object(NonMatching, "dolphin/gx/GXTev.c"),
            Object(NonMatching, "dolphin/gx/GXPixel.c"),
            Object(NonMatching, "dolphin/gx/GXDraw.c"),
            Object(NonMatching, "dolphin/gx/GXDisplayList.c"),
            Object(NonMatching, "dolphin/gx/GXTransform.c"),
            Object(NonMatching, "dolphin/gx/GXPerf.c"),
        ],
    ),
    DolphinLib(
        "card",
        [
            Object(NonMatching, "dolphin/card/CARDBios.c"),
            Object(NonMatching, "dolphin/card/CARDUnlock.c"),
            Object(NonMatching, "dolphin/card/CARDRdwr.c"),
            Object(NonMatching, "dolphin/card/CARDBlock.c"),
            Object(NonMatching, "dolphin/card/CARDDir.c"),
            Object(NonMatching, "dolphin/card/CARDCheck.c"),
            Object(NonMatching, "dolphin/card/CARDMount.c"),
            Object(NonMatching, "dolphin/card/CARDFormat.c"),
            Object(NonMatching, "dolphin/card/CARDOpen.c"),
            Object(NonMatching, "dolphin/card/CARDCreate.c"),
            Object(NonMatching, "dolphin/card/CARDRead.c"),
            Object(NonMatching, "dolphin/card/CARDWrite.c"),
            Object(NonMatching, "dolphin/card/CARDDelete.c"),
            Object(NonMatching, "dolphin/card/CARDStat.c"),
            Object(NonMatching, "dolphin/card/CARDNet.c"),
        ],
    ),
    DolphinLib(
        "exi",
        [
            Object(NonMatching, "dolphin/exi/EXIBios.c"),
            Object(NonMatching, "dolphin/exi/EXIUart.c"),
        ],
    ),
    DolphinLib(
        "si",
        [
            Object(NonMatching, "dolphin/si/SIBios.c"),
            Object(NonMatching, "dolphin/si/SISamplingRate.c"),
        ],
    ),
    {
        "lib": "thp",
        "mw_version": "GC/1.2.5",
        "cflags": cflags_thp,
        "host": False,
        "objects": [
            Object(NonMatching, "dolphin/thp/THPDec.c"),
            Object(NonMatching, "dolphin/thp/THPAudio.c"),
        ],
    },
    {
        "lib": "sdk_mic",
        "mw_version": config.linker_version,
        "cflags": cflags_sdk_mic,
        "host": False,
        "objects": [
            Object(NonMatching, "dolphin/mic/mic.c"),
            Object(NonMatching, "dolphin/mic/m2s.c"),
        ],
    },
    {
        "lib": "gssdk",
        "mw_version": config.linker_version,
        "cflags": cflags_gssdk,
        "host": False,
        "objects": [
            Object(NonMatching, "gssdk_lib/gsapi/sid/sid.c"),
            Object(NonMatching, "gssdk_lib/gsapi/callbacks.c"),
            Object(NonMatching, "gssdk_lib/gsapi/ctxfuncs.c"),
            Object(NonMatching, "gssdk_lib/gsapi/extaudio.c"),
            Object(NonMatching, "gssdk_lib/gsapi/gsapi.c"),
            Object(NonMatching, "gssdk_lib/gsapi/mathusage.c"),
            Object(NonMatching, "gssdk_lib/gsapi/wrddata.c"),
            Object(NonMatching, "gssdk_lib/asrpho/asrspi.c"),
            Object(NonMatching, "gssdk_lib/asrpho/rec1600/convert.c"),
            Object(NonMatching, "gssdk_lib/asrpho/rec1600/creasp.c"),
            Object(NonMatching, "gssdk_lib/asrpho/rec1600/creaspch.c"),
            Object(NonMatching, "gssdk_lib/asrpho/rec1600/creaspt.c"),
            Object(NonMatching, "gssdk_lib/asrpho/rec1600/creatree.c"),
            Object(NonMatching, "gssdk_lib/asrpho/rec1600/crsptrch.c"),
            Object(NonMatching, "gssdk_lib/asrpho/rec1600/ctrl.c"),
            Object(NonMatching, "gssdk_lib/asrpho/rec1600/initial.c"),
            Object(NonMatching, "gssdk_lib/asrpho/rec1600/spi1600.c"),
            Object(NonMatching, "gssdk_lib/asrpho/rec1600/train.c"),
            Object(NonMatching, "gssdk_lib/asrpho/rec1600/userword.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/delaybl.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/dpgenuw.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/dpscruw.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/exev_dp.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/fft_maye.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/fftmod.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/isoword.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/nbestdp.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/pitchdp.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/pitchwin.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/stacker.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/undersam.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flblocks/acne.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flblocks/dctlift.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flblocks/gender.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flblocks/logexp.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flblocks/mel.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flblocks/mtx.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flblocks/mtxopt.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flblocks/smoother.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flblocks/spline.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flblocks/specsub.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flblocks/vad.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flblocks/vq1500.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flblocks/window.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flfxblks/combiner.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flfxblks/dist16.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flfxblks/genfilt.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flfxblks/lkahead.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flfxblks/median.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flfxblks/pitchco.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flfxblks/shs_vuv.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flfxblks/slidhist.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flfxblks/statio.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flfxblks/subsamp.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flfxblks/trigglr.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/blocks/flfxblks/voicing.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/ctxdata/ctxdata.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/ctxdata/langdata.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/tos/mqueue.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/tos/tinyos.c"),
            Object(NonMatching, "gssdk_lib/asrpho/common/fastallo/fastallo.c"),
            Object(NonMatching, "gssdk_lib/common/csspi/csspi.c"),
            Object(NonMatching, "gssdk_lib/common/safeh/safeh.c"),
            Object(NonMatching, "gssdk_lib/common/osspi/osspi.c"),
            Object(NonMatching, "gssdk_lib/common/rsrc/rsrc.c"),
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
            Object(Matching, "zlib/adler32.c"),
            Object(Matching, "zlib/inflate.c"),
            Object(Matching, "zlib/infblock.c"),
            Object(Matching, "zlib/infcodes.c"),
            Object(Matching, "zlib/infutil.c"),
            Object(Matching, "zlib/inftrees.c"),
            Object(Matching, "zlib/inffast.c"),
            Object(Matching, "zlib/zutil.c"),
        ],
    },
    {
        "lib": "board",
        "mw_version": config.linker_version,
        "cflags": cflags_game,
        "host": False,
        "objects": [
            Object(NonMatching, "board/exit.c"),
            Object(NonMatching, "board/board.c"),
            Object(NonMatching, "board/math.c"),
            Object(NonMatching, "board/camera.c"),
            Object(NonMatching, "board/player.c"),
            Object(NonMatching, "board/snpc.c"),
            Object(NonMatching, "board/object.c"),
            Object(NonMatching, "board/window.c"),
            Object(NonMatching, "board/audio.c"),
            Object(NonMatching, "board/comchoice.c"),
            Object(NonMatching, "board/scroll.c"),
            Object(NonMatching, "board/masu.c"),
            Object(NonMatching, "board/coin.c"),
            Object(NonMatching, "board/star.c"),
            Object(NonMatching, "board/padall.c"),
            Object(NonMatching, "board/dice.c"),
            Object(NonMatching, "board/status.c"),
            Object(NonMatching, "board/opening.c"),
            Object(NonMatching, "board/pause.c"),
            Object(NonMatching, "board/tutorial.c"),
            Object(NonMatching, "board/roulette.c"),
            Object(NonMatching, "board/capselect.c"),
            Object(NonMatching, "board/capmove.c"),
            Object(NonMatching, "board/capthrow.c"),
            Object(NonMatching, "board/captrap.c"),
            Object(NonMatching, "board/capspecial.c"),
            Object(NonMatching, "board/capsule.c"),
            Object(NonMatching, "board/capevent.c"),
            Object(NonMatching, "board/shopevent.c"),
            Object(NonMatching, "board/guide.c"),
            Object(NonMatching, "board/branch.c"),
            Object(NonMatching, "board/mgcall.c"),
            Object(NonMatching, "board/effect.c"),
            Object(NonMatching, "board/config.c"),
            Object(NonMatching, "board/gate.c"),
            Object(NonMatching, "board/last5.c"),
            Object(NonMatching, "board/telop.c"),
            Object(NonMatching, "board/wipe.c"),
            Object(NonMatching, "board/single.c"),
            Object(NonMatching, "board/malloc.c"),
        ],
    },
    {
        "lib": "REL",
        "mw_version": config.linker_version,
        "cflags": cflags_rel,
        "host": False,
        "objects": [
            Object(Matching, "REL/empty.c"),  # Must be marked as matching
            Object(
                NonMatching,
                "REL/runtime.c",
                source="Runtime.PPCEABI.H/runtime.c",
            ),
        ],
    },
    Rel(
        "bootDll",
        objects={
            Object(NonMatching, "REL/bootDll/boot.c"),
            Object(NonMatching, "REL/bootDll/data.c"),
            Object(NonMatching, "REL/bootDll/opening.c"),
        },
    ),

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
