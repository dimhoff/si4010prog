#!/usr/bin/env python
# tune_cap_bank.py - Si4010 Get tuned PA cap bank setting
#
# This runs the Si4010 antenna tuning algorithm. This algorithm changes the cap
# bank capacitance till the antenna is tuned. The tuned cap bank value can be
# used to configure the PA in the future by passing it to vPa_Setup() as
# wNominalCap.
#
# If the returned value is 0 or 511 the antenna could not be properly tuned.
# See 'AN547 - Si4010 Calculator Spreadsheet Usage' chapter 2.8. 'Usage of the
# Power Amplifier Setup' for more information.
#
#
# Copyright (c) 2018, David Imhoff <dimhoff.devel@gmail.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the author nor the names of its contributors may
#       be used to endorse or promote products derived from this software
#       without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
import argparse
import struct
import subprocess
import tune_cap_bank_map as fw_map

VERSION_MAJOR = "0"
VERSION_MINOR = "0"

# Parse arguments
parser = argparse.ArgumentParser(
    description='Get Si4010 tuned antenna cap bank setting')
parser.add_argument(
    'freq', type=float,
    metavar='FREQ',
    help='Frequency in MHz at which to tune the antenna')
parser.add_argument(
    '--alpha',
    dest='fAlpha', type=float, default=0.0,
    metavar='FLOAT',
    help="Pa fAlpha value, default: 0.0")
parser.add_argument(
    '--beta',
    dest='fBeta', type=float, default=0.0,
    metavar='FLOAT',
    help="Pa fBeta value, default: 0.0")
parser.add_argument(
    '--level',
    dest='bLevel', type=int, default=76,
    metavar='INT',
    help="Pa bLevel value, default: 76")
parser.add_argument(
    '--max-drv',
    dest='bMaxDrv', type=int, default=0,
    metavar='0-1',
    help="Pa bMaxDrv value, default: 0")
parser.add_argument(
    '--nominal-cap',
    dest='wNominalCap', type=int, default=256,
    metavar='INT',
    help="Pa initial wNominalCap value, default: 256")
parser.add_argument(
    '--si4010prog-exec',
    dest="si4010prog_exec", default='si4010prog',
    metavar='PATH',
    help='Path of si4010 executable, default: si4010')
parser.add_argument(
    '--programmer',
    dest="programmer",
    metavar='URI',
    help='Si4010 Programmer URI to pass to si4010prog')
parser.add_argument(
    '--reset-workaround',
    dest='reset_workaround', action='store_true',
    help="Perform workaround for devices that automatically run program in NVM on boot")
parser.add_argument(
    '--version',
    action='version',
    version='%(prog)s {}.{}'.format(VERSION_MAJOR, VERSION_MINOR))

args = parser.parse_args()

if args.freq < 27 or args.freq >= 960:
    print("Error: Frequency out of range (27 <= freq < 960)")
    exit(-1)

if args.bLevel < 0 or args.bLevel >= 0x80:
    print("Error: bLevel out of range (0 <= bLevel < 128)")
    exit(-1)

if args.bMaxDrv != 0 and args.bMaxDrv != 1:
    print("Error: bMaxDrv out of range (bMaxDrv=0 or 1)")
    exit(-1)

if args.wNominalCap < 0 or args.wNominalCap >= 0x200:
    print("Error: wNominalCap out of range (0 <= wNominalCap < 512)")
    exit(-1)

print("> Tuning antenna to: {:.2f} MHz".format(args.freq))

freq_config = struct.pack(">f", args.freq * 1e6).encode('hex')
pa_config = struct.pack(">ffBBH", args.fAlpha, args.fBeta, args.bLevel, args.bMaxDrv, args.wNominalCap).encode('hex')

init_commands = [ "reset", "run", "halt", "setpc:0"]

si4010_cmdline = [ args.si4010prog_exec, "-y"]

if args.programmer:
    si4010_cmdline.extend([ "-d", args.programmer ])

si4010_cmdline.append( "reset")
if args.reset_workaround:
    si4010_cmdline.extend([ "run", "halt", "setpc:0"])

si4010_cmdline.extend([
    "prg:tune_cap_bank.ihx",
    "wram:0x{:x},{}".format(fw_map.ADDR_FREQ, freq_config),
    "wxram:0x{:x},{}".format(fw_map.ADDR_PA_CONFIG, pa_config),
    "break:0x{:x}".format(fw_map.BP_ADDR),
    "run",
    "delay:1000",
    "dxram:0x400c,2"])

print("> Running: " + " ".join(si4010_cmdline))
subprocess.call(si4010_cmdline)

