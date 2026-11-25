#!/usr/bin/env python3
"""Consistency checker for docs/rebirth. Exit code 1 on failure."""
import re, sys, glob, os
ROOT = os.path.join(os.path.dirname(__file__), "..", "docs", "rebirth")
files = glob.glob(ROOT + "/**/*.md", recursive=True)
errs = []
txt = {f: open(f, encoding="utf-8").read() for f in files}
# 1 internal links
for f, t in txt.items():
    for m in re.finditer(r"\]\(([^)#]+\.md)(#[^)]*)?\)", t):
        tgt = os.path.normpath(os.path.join(os.path.dirname(f), m.group(1)))
        if not os.path.exists(tgt):
            errs.append(f"broken link {os.path.relpath(f,ROOT)} -> {m.group(1)}")
# 2 required header fields and no placeholders
for f, t in txt.items():
    for k in ("| Purpose |", "| Status |", "| Revision date |"):
        if k not in t: errs.append(f"missing header field {k} in {os.path.relpath(f,ROOT)}")
    if re.search(r"\b(TBD|TODO|FIXME|lorem)\b", t): errs.append(f"placeholder text in {os.path.relpath(f,ROOT)}")
# 3 shared constants
sc = txt[os.path.join(ROOT, "00_SHARED_CONSTANTS.md")]
clips = int(re.search(r"Animation clips per fighter \| (\d+)", sc).group(1))
anim = txt[os.path.join(ROOT, "10_ANIMATION_AND_VFX.md")]
if "(%d)" % clips not in anim: errs.append("clip count mismatch between SC-7 and 10_ANIMATION_AND_VFX")
for f in glob.glob(ROOT + "/fighters/*.md"):
    t = txt[f]
    m = re.search(r"Per .*the (\d+)-clip list", t)
    if not m or int(m.group(1)) != clips: errs.append("fighter clip count mismatch " + os.path.basename(f))
    lst = re.search(r"\n(Idle, .*SurgeActivate)\.", t)
    if not lst or len(lst.group(1).split(", ")) != clips: errs.append("fighter anim list length " + os.path.basename(f))
# 4 phase ids in roadmap vs work packages
road = txt[os.path.join(ROOT, "24_PRODUCTION_ROADMAP.md")]
ids = re.findall(r"^\| (P\d+) \|", road, re.M)
if ids != ["P%d" % i for i in range(15)]: errs.append("roadmap phase ids not P0..P14: %s" % ids)
for i in ids:
    if not glob.glob(ROOT + "/work-packages/%s_*.md" % i): errs.append("missing work package " + i)
# 5 dependency order: each dep must have lower number
for m in re.finditer(r"^\| (P\d+) \|[^|]*\|[^|]*\|[^|]*\| ([^|]*) \|$", road, re.M):
    n = int(m.group(1)[1:])
    for d in re.findall(r"P\d+", m.group(2)):
        if int(d[1:]) >= n: errs.append(f"{m.group(1)} depends on later/equal phase {d}")
# 6 estimate arithmetic
rows = re.findall(r"^\| P\d+ \|[^|]*\| (\d+)-(\d+) \| ([^|]*) \|", road, re.M)
lo = sum(int(a) for a, b, c in rows); hi = sum(int(b) for a, b, c in rows)
m = re.search(r"^\| Sum \| \| (\d+)-(\d+) \| (\d+)-(\d+) \|", road, re.M)
if not m or (int(m.group(1)), int(m.group(2))) != (lo, hi): errs.append("roadmap sum mismatch")
fl = sum(int(c.split('-')[0]) for a, b, c in rows if re.match(r"\d+-\d+", c.strip()))
fh = sum(int(c.split('-')[1]) for a, b, c in rows if re.match(r"\d+-\d+", c.strip()))
if m and (int(m.group(3)), int(m.group(4))) != (fl, fh): errs.append("first-release sum mismatch")
# 7 stale numbers
for f, t in txt.items():
    if "12-18 months" in t and "not credible" not in t and "preliminary" not in t.lower():
        errs.append("unqualified 12-18 months in " + os.path.relpath(f, ROOT))
print("files checked:", len(files))
if errs:
    print("FAIL"); [print(" -", e) for e in errs]; sys.exit(1)
print("PASS: links, headers, constants, phases, dependencies, estimates")
