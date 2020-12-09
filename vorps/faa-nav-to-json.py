#!/usr/bin/env python3

import json
import sys

def parse_faa_nav_line(line):
    def l(a,b): return line[a:b].rstrip()

    return {
        "key": l(0,8),
        "type": l(8,28),
        "id": l(28,32),
        "date": l(32,42),
        "name": l(42,72),
        "city": l(72,112),
        "state": l(112,142),
        "unk1": l(142,179),
        "org1": l(179,229),
        "org2": l(229,279),
        "unk2": l(279,292),
        "unk3": l(292,303),
        "atc1": l(303,337),
        "atc2": l(337,371),
        "loc1": l(371,385),
        "loc2": l(385,436),
        "loc3": l(436,473),
        "unk4": l(473,481),
        "unk5": l(481,495),
        "unk6": l(495,498),
        "unk7": l(498,499),
        "fss": l(499,529),
        "chan": l(529,533),
        "freq": l(533,576),
        "unk8": l(576,612),
        "unk9": l(612,646),
        "unk10": l(646,746),
        "unk11": l(746,766),
        "status": l(766,796),
        "unk12": l(796,800),
    }

r = []
with open(sys.argv[1], "r") as f:
    for line in f.readlines():
        if not line.startswith("NAV1"):
            continue

        r.append(parse_faa_nav_line(line))

print(
    json.dumps(
        [x for x in r if not x["status"].startswith("DECOMMISSIONED")],
        indent=8,
    )
)
