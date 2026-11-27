#!/usr/bin/env python3
"""Migrate a server's cfg directory for issue #2689 (prayer and sacrifice as spells, piety skill).

lib/cfg is not under version control on a live server, so the changes the branch made to the
repository's lib/cfg have to be carried over by hand -- or by this script. The new blocks (spells,
affects, messages, the kReligion skill line) are copied from the repository's lib/cfg, which is the
source of truth; the changed lines are patched in place. Every step is idempotent: a file that is
already migrated is left alone, so the script can be run again safely.

Usage:
    tools/migrate_cfg_2689_religion.py --cfg /path/to/server/lib/cfg [--src lib/cfg] [--dry-run]

Each changed file is backed up next to itself as <name>.bak-2689 before it is written.
The files must be UTF-8 (tools/convert_cfg_to_utf8.py converts old KOI8-R ones).

What it does:
  skills.xml           self_train="1" -> flags="kSelfTrain"; the old disabled kReligion line is
                       replaced by the enabled piety skill; the header documents the flags.
  classes/pc_*.xml     every class lists kReligion (an obligatory skill is not saved otherwise).
  affects/affects.xml  eight undispellable favours (kYariloFavor ... kGeorgeGrace).
  spells.xml           the service spells kOffering and kPrayer.
  spells.scheme        the <action choice=> attribute for the Vedun editor.
  messages/ru/skill_msg.xml   kReligion is named "набожность".
  messages/ru/affect_msg.xml  the favours' short descriptions.
  messages/ru/spell_msg.xml   kUnknownChoice in kDefault, the kOffering and kPrayer sheaves.
  mechanics/piety.xml  new: what may be offered and which gift raises piety (copied if missing).
  mechanics/piety.scheme  new: its Vedun scheme (copied if missing).
"""

import argparse
import re
import shutil
import sys
from pathlib import Path

CLASS_FILES = ["pc_assasine", "pc_charmer", "pc_conjurer", "pc_guard", "pc_magus", "pc_merchant",
               "pc_necromancer", "pc_paladine", "pc_ranger", "pc_sorcerer", "pc_thief", "pc_vigilant",
               "pc_warrior", "pc_wizard"]
FAVOURS = ["kYariloFavor", "kMakoshFavor", "kPerunFavor", "kVelesFavor",
           "kTheotokosGrace", "kNicholasGrace", "kElijahGrace", "kGeorgeGrace"]


class MigrationError(Exception):
    pass


def read(path):
    try:
        return path.read_bytes().decode("utf-8")
    except UnicodeDecodeError:
        raise MigrationError(f"{path}: not UTF-8 -- convert it first (tools/convert_cfg_to_utf8.py)")


def slice_between(text, start, end, path, include_end=True):
    """The text from `start` up to (and including) `end`; both must be present."""
    a = text.find(start)
    if a < 0:
        raise MigrationError(f"{path}: source block start not found: {start!r}")
    b = text.find(end, a)
    if b < 0:
        raise MigrationError(f"{path}: source block end not found: {end!r}")
    return text[a:b + len(end)] if include_end else text[a:b]


def line_of(text, needle, path):
    """The whole line (with its newline) that contains `needle`."""
    a = text.find(needle)
    if a < 0:
        raise MigrationError(f"{path}: line not found: {needle!r}")
    start = text.rfind("\n", 0, a) + 1
    end = text.find("\n", a) + 1
    return text[start:end]


def insert_after_line(text, anchor, block):
    """Insert `block` after the line containing `anchor`; None if the anchor is missing."""
    a = text.find(anchor)
    if a < 0:
        return None
    end = text.find("\n", a) + 1
    return text[:end] + block + text[end:]


def insert_before(text, anchor, block):
    """Insert `block` before the line containing the last `anchor` (a closing root tag)."""
    a = text.rfind(anchor)
    if a < 0:
        return None
    start = text.rfind("\n", 0, a) + 1
    return text[:start] + block + text[start:]


class Migrator:
    def __init__(self, cfg, src, dry_run):
        self.cfg = cfg
        self.src = src
        self.dry_run = dry_run
        self.changed = []

    def source(self, rel):
        return read(self.src / rel)

    def patch(self, rel, func):
        path = self.cfg / rel
        if not path.exists():
            print(f"  ! {rel}: missing, skipped")
            return
        before = read(path)
        notes = []
        after = func(before, notes)
        if after == before:
            print(f"  = {rel}: already migrated")
            return
        print(f"  + {rel}: " + "; ".join(notes))
        self.changed.append(rel)
        if not self.dry_run:
            shutil.copy2(path, path.with_name(path.name + ".bak-2689"))
            path.write_bytes(after.encode("utf-8"))

    def add(self, rel):
        """Copy a file the branch added, unless the server already has one."""
        path = self.cfg / rel
        if path.exists():
            print(f"  = {rel}: already present")
            return
        read(self.src / rel)   # the source must be there and UTF-8
        print(f"  + {rel}: new file")
        self.changed.append(rel)
        if not self.dry_run:
            path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(self.src / rel, path)

    # --- skills.xml -------------------------------------------------------------------------------
    def skills(self, text, notes):
        src = self.source("skills.xml")
        # The flags documentation in the header.
        if "kKeepOnRemort - " not in text:
            doc = slice_between(src, "flags - флаги умения", "mode - режим умения.", "skills.xml",
                                include_end=False)
            old = re.search(r"self_train - .*?(?=mode - режим умения\.)", text, re.S)
            if old:
                text = text[:old.start()] + doc + text[old.end():]
            else:
                patched = insert_before(text, "mode - режим умения.", doc)
                if patched is None:
                    raise MigrationError("skills.xml: no 'mode - режим умения.' line in the header")
                text = patched
            text = text.replace(
                '<skill id="kReligion" name="!молитва или жертва!" abbr="Error" saving="kWill" '
                'difficulty="1" cap="1" mode="kDisabled"/>',
                '<skill id="kBash" saving="kStability" difficulty="200" cap="1000" mode="kEnabled"/>')
            notes.append("header documents flags")

        # self_train -> flags.
        def to_flags(m):
            tag = m.group(0)
            value = re.search(r'\sself_train="([^"]*)"', tag).group(1)
            tag = re.sub(r'\s+self_train="[^"]*"', "", tag)
            if value.strip().lower() not in ("1", "y", "yes", "true"):
                return tag
            flags = re.search(r'flags="([^"]*)"', tag)
            if flags:
                if "kSelfTrain" in flags.group(1).split("|"):
                    return tag
                return tag.replace(flags.group(0), f'flags="{flags.group(1)}|kSelfTrain"')
            return re.sub(r"\s*/>$", '\tflags="kSelfTrain"/>', tag)
        text, n = re.subn(r"<skill\b[^>]*\sself_train=\"[^\"]*\"[^>]*/>", to_flags, text)
        if n:
            notes.append(f"self_train -> flags on {n} skill(s)")

        # The piety skill.
        new_line = line_of(src, '<skill id="kReligion"', "skills.xml")
        current = re.search(r'[ \t]*<skill id="kReligion"[^>]*/>[ \t]*\r?\n', text)
        if current is None or current.group(0) != new_line:
            if current is not None:
                text = text[:current.start()] + text[current.end():]
            patched = insert_after_line(text, '<skill id="kMakeAmulet"', new_line)
            if patched is None:
                patched = insert_before(text, "</skills>", new_line)
            if patched is None:
                raise MigrationError("skills.xml: no place for the kReligion line")
            text = patched
            notes.append("kReligion is the enabled piety skill")
        return text

    # --- classes ----------------------------------------------------------------------------------
    def make_class(self, name):
        def func(text, notes):
            if re.search(r'<skill id="kReligion"', text):
                return text
            src = self.source(f"classes/{name}.xml")
            block = (line_of(src, "<!-- issue #2689: набожность", f"classes/{name}.xml")
                     + line_of(src, '<skill id="kReligion"', f"classes/{name}.xml"))
            # Before the </skills> that closes the class's skill list.
            a = text.find("</skills>")
            if a < 0:
                raise MigrationError(f"classes/{name}.xml: no </skills>")
            start = text.rfind("\n", 0, a) + 1
            notes.append("lists kReligion")
            return text[:start] + block + text[start:]
        return func

    # --- affects.xml ------------------------------------------------------------------------------
    def affects(self, text, notes):
        if '<affect id="kYariloFavor"' in text:
            return text
        src = self.source("affects/affects.xml")
        start = src.find("\t<!-- issue #2689: милость того")
        if start < 0:
            raise MigrationError("affects/affects.xml (source): the favours block not found")
        end_tag = src.find("</affect>", src.find('<affect id="kGeorgeGrace"'))
        block = src[start:src.find("\n", end_tag) + 1]
        patched = insert_after_line(text, '<affect id="kPietas"', block)
        if patched is None:
            patched = insert_before(text, "</affects>", block)
        if patched is None:
            raise MigrationError("affects/affects.xml: no place for the favours")
        notes.append("8 favours")
        return patched

    # --- spells.xml / spells.scheme ---------------------------------------------------------------
    def spells(self, text, notes):
        if '<spell id="kOffering"' in text:
            return text
        src = self.source("spells.xml")
        start = src.find("\t<!-- issue #2689: жертва (язычники)")
        end = src.find("</spell>", src.find('<spell id="kPrayer"'))
        if start < 0 or end < 0:
            raise MigrationError("spells.xml (source): kOffering/kPrayer not found")
        block = src[start:src.find("\n", end) + 1]
        patched = insert_before(text, "</spells>", block)
        if patched is None:
            raise MigrationError("spells.xml: no </spells>")
        notes.append("kOffering, kPrayer")
        return patched

    def scheme(self, text, notes):
        if '<attr name="choice"' in text:
            return text
        src = self.source("spells.scheme")
        new_line = line_of(src, '<attr name="choice"', "spells.scheme")
        tag = text.find('<tag name="action"')
        if tag < 0:
            raise MigrationError("spells.scheme: no <tag name=\"action\">")
        reset = text.find('<attr name="reset"', tag)
        close = text.find("</tag>", tag)
        if 0 <= reset < close:
            end = text.find("\n", reset) + 1
        else:
            end = text.find("\n", tag) + 1
        notes.append("action.choice")
        return text[:end] + new_line + text[end:]

    # --- messages ---------------------------------------------------------------------------------
    def skill_msg(self, text, notes):
        m = re.search(r'(<msg_sheaf id="kReligion">\s*<name )[^>]*/>', text)
        if m is None:
            raise MigrationError("messages/ru/skill_msg.xml: no kReligion sheaf")
        new = m.group(1) + 'val="набожность" abbr="Нб" />'
        if m.group(0) == new:
            return text
        notes.append("kReligion is набожность")
        return text[:m.start()] + new + text[m.end():]

    def affect_msg(self, text, notes):
        src = self.source("messages/ru/affect_msg.xml")
        block = ""
        for favour in FAVOURS:
            if f'<msg_sheaf id="{favour}">' in text:
                continue
            start = src.find(f'\t<msg_sheaf id="{favour}">')
            if start < 0:
                raise MigrationError(f"affect_msg.xml (source): {favour} not found")
            end = src.find("</msg_sheaf>", start)
            block += src[start:src.find("\n", end) + 1]
        if not block:
            return text
        pietas = text.find('<msg_sheaf id="kPietas">')
        if pietas >= 0:
            end = text.find("</msg_sheaf>", pietas)
            end = text.find("\n", end) + 1
            patched = text[:end] + block + text[end:]
        else:
            patched = insert_before(text, "</affect_msg>", block)
        if patched is None:
            raise MigrationError("messages/ru/affect_msg.xml: no place for the favours")
        notes.append("favours' short descriptions")
        return patched

    def spell_msg(self, text, notes):
        src = self.source("messages/ru/spell_msg.xml")
        if 'type="kUnknownChoice" val="Уточните' not in text:
            new_line = line_of(src, 'type="kUnknownChoice" val="Уточните', "spell_msg.xml")
            default = text.find('<msg_sheaf id="kDefault">')
            if default < 0:
                raise MigrationError("messages/ru/spell_msg.xml: no kDefault sheaf")
            wrong = text.find('type="kWrongTarget"', default)
            if 0 <= wrong < text.find("</msg_sheaf>", default):
                end = text.find("\n", wrong) + 1
            else:
                end = text.find("\n", default) + 1
            text = text[:end] + new_line + text[end:]
            notes.append("kDefault kUnknownChoice")
        if '<msg_sheaf id="kOffering">' not in text:
            start = src.find("\t<!-- issue #2689: жертва и молитва")
            end = src.find("</msg_sheaf>", src.find('<msg_sheaf id="kPrayer">'))
            if start < 0 or end < 0:
                raise MigrationError("spell_msg.xml (source): kOffering/kPrayer not found")
            block = src[start:src.find("\n", end) + 1]
            patched = insert_before(text, "</spell_msg>", block)
            if patched is None:
                raise MigrationError("messages/ru/spell_msg.xml: no </spell_msg>")
            text = patched
            notes.append("kOffering, kPrayer sheaves")
        return text

    def run(self):
        self.patch("skills.xml", self.skills)
        for name in CLASS_FILES:
            self.patch(f"classes/{name}.xml", self.make_class(name))
        self.patch("affects/affects.xml", self.affects)
        self.patch("spells.xml", self.spells)
        self.patch("spells.scheme", self.scheme)
        self.patch("messages/ru/skill_msg.xml", self.skill_msg)
        self.patch("messages/ru/affect_msg.xml", self.affect_msg)
        self.patch("messages/ru/spell_msg.xml", self.spell_msg)
        self.add("mechanics/piety.xml")
        self.add("mechanics/piety.scheme")


def main():
    repo_cfg = Path(__file__).resolve().parent.parent / "lib" / "cfg"
    parser = argparse.ArgumentParser(description="Migrate a cfg directory for issue #2689.")
    parser.add_argument("--cfg", required=True, type=Path, help="the server's cfg directory to migrate")
    parser.add_argument("--src", type=Path, default=repo_cfg,
                        help=f"the repository's lib/cfg with the new blocks (default: {repo_cfg})")
    parser.add_argument("--dry-run", action="store_true", help="only report what would change")
    args = parser.parse_args()

    if args.cfg.resolve() == args.src.resolve():
        print("--cfg and --src are the same directory; nothing to do.", file=sys.stderr)
        return 1
    migrator = Migrator(args.cfg, args.src, args.dry_run)
    try:
        migrator.run()
    except MigrationError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    verb = "would change" if args.dry_run else "changed"
    print(f"{len(migrator.changed)} file(s) {verb}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
