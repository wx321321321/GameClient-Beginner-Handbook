#!/usr/bin/env python3
"""Number course headings and migrate local Markdown fragments without touching code.

Default mode previews the changes. Use --write after all article edits finish;
--check exits nonzero if the same operation would change any file.
"""
from __future__ import annotations

import argparse
import json
import os
import re
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import quote, unquote, urlsplit

from markdown_it import MarkdownIt
from markdown_it.helpers import parseLinkDestination

PARSER = MarkdownIt("commonmark", {"html": False}).enable("table")
NAV_START = "<!-- course-navigation:start -->"
NAV_END = "<!-- course-navigation:end -->"
IGNORED_DIRS = {".git", ".local-archive", ".venv", "venv", "node_modules", "output", "build", "dist", "__pycache__"}


class PlainText(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.parts: list[str] = []

    def handle_data(self, data: str) -> None:
        self.parts.append(data)


def heading_text(markdown: str) -> str:
    collector = PlainText()
    collector.feed(PARSER.renderInline(markdown))
    return re.sub(r"\s+", " ", "".join(collector.parts)).strip()


def heading_anchor(text: str) -> str:
    """The same GitHub-style fragment convention as the EPUB builder."""
    return re.sub(r"[^\w\- ]", "", text.lower(), flags=re.UNICODE).replace(" ", "-")


def strip_number(title: str) -> str:
    # Do not strip meaningful titles such as '64 位地址' or '3D 旋转'.
    pattern = r"^(?:第\s*[0-9一二三四五六七八九十百零〇]+\s*[章节篇编][：:、.．]?\s*|\d+(?:\.\d+)+(?:[.、．)）])?\s+|\d+[.、．)）]\s*)"
    while (cleaned := re.sub(pattern, "", title, count=1)) != title:
        title = cleaned
    return title


def unique_anchor(text: str, used: set[str]) -> str:
    base = heading_anchor(text)
    candidate = base
    suffix = 0
    while candidate in used:
        suffix += 1
        candidate = f"{base}-{suffix}"
    used.add(candidate)
    return candidate


def line_offsets(text: str) -> list[int]:
    offsets = [0]
    for line in text.splitlines(keepends=True):
        offsets.append(offsets[-1] + len(line))
    return offsets


def replace_spans(text: str, replacements: list[tuple[int, int, str]]) -> str:
    for start, end, value in sorted(replacements, reverse=True):
        text = text[:start] + value + text[end:]
    return text


def number_headings(text: str, prefix: str) -> tuple[str, dict[str, str], str, int]:
    tokens = PARSER.parse(text)
    offsets = line_offsets(text)
    changes: list[tuple[int, int, str]] = []
    old_used: set[str] = set()
    bare_used: set[str] = set()
    new_used: set[str] = set()
    mapping: dict[str, str] = {}
    aliases: dict[str, str] = {}
    counts = [0] * 7
    h1_count = 0
    title = ""
    previous_level = 0
    for index, token in enumerate(tokens):
        if token.type != "heading_open":
            continue
        level = int(token.tag[1:])
        content = tokens[index + 1].content
        bare = strip_number(content)
        if level == 1:
            h1_count += 1
            if h1_count > 1:
                raise ValueError(f"{prefix}: expected exactly one H1")
            title = heading_text(bare)
            number = prefix
        else:
            if not h1_count or level > previous_level + 1:
                raise ValueError(f"{prefix}: heading level jumps to H{level}: {content}")
            counts[level] += 1
            for deeper in range(level + 1, 7):
                counts[deeper] = 0
            number = prefix + "." + ".".join(str(counts[n]) for n in range(2, level + 1))
        previous_level = level
        new_content = f"{number} {bare}"
        old_anchor = unique_anchor(heading_text(content), old_used)
        bare_anchor = unique_anchor(heading_text(bare), bare_used)
        new_anchor = unique_anchor(heading_text(new_content), new_used)
        mapping[old_anchor] = new_anchor
        aliases[bare_anchor] = new_anchor
        assert token.map is not None
        start, end = token.map
        changes.append((offsets[start], offsets[end], f"{'#' * level} {new_content}\n"))
    if h1_count != 1:
        raise ValueError(f"{prefix}: expected exactly one H1")
    # Actual old fragments take precedence over number-free convenience aliases.
    for fragment, target in aliases.items():
        mapping.setdefault(fragment, target)
    return replace_spans(text, changes), mapping, title, len(changes)


def code_mask(text: str) -> bytearray:
    """Protect fenced/indented blocks and inline code, including multiline spans."""
    mask = bytearray(len(text))
    offsets = line_offsets(text)
    for token in PARSER.parse(text):
        if token.type in {"fence", "code_block"} and token.map:
            start, end = (offsets[i] for i in token.map)
            mask[start:end] = b"\x01" * (end - start)
    ticks = list(re.finditer(r"`+", text))
    index = 0
    while index < len(ticks):
        opening = ticks[index]
        preceding_slashes = len(text[:opening.start()]) - len(text[:opening.start()].rstrip("\\"))
        if mask[opening.start()] or preceding_slashes % 2:
            index += 1
            continue
        closing_index = index + 1
        while closing_index < len(ticks):
            closing = ticks[closing_index]
            if not mask[closing.start()] and len(closing.group()) == len(opening.group()):
                mask[opening.start():closing.end()] = b"\x01" * (closing.end() - opening.start())
                index = closing_index
                break
            closing_index += 1
        index += 1
    return mask


def rewrite_links(text: str, source: Path, root: Path, mappings: dict[Path, dict[str, str]]) -> tuple[str, int]:
    mask = code_mask(text)
    starts = [match.end() for match in re.finditer(r"(?<!\\)\]\(\s*", text)]
    starts += [match.end() for match in re.finditer(r"(?m)^ {0,3}\[[^\]\n]+\]:[ \t]*", text)]
    replacements: list[tuple[int, int, str]] = []
    seen: set[tuple[int, int]] = set()
    for start in starts:
        if start >= len(text) or mask[start]:
            continue
        parsed = parseLinkDestination(text, start, len(text))
        if not parsed.ok or (start, parsed.pos) in seen:
            continue
        seen.add((start, parsed.pos))
        if any(mask[start:parsed.pos]):
            continue
        destination = parsed.str
        split = urlsplit(destination)
        if split.scheme or split.netloc or not split.fragment:
            continue
        target = source if not split.path else (
            root / unquote(split.path).lstrip("/") if split.path.startswith("/")
            else source.parent / unquote(split.path)
        ).resolve()
        replacement_fragment = mappings.get(target, {}).get(unquote(split.fragment))
        if replacement_fragment is None or replacement_fragment == unquote(split.fragment):
            continue
        raw = text[start:parsed.pos]
        fragment_start = raw.find("#")
        if fragment_start < 0:
            continue
        encoded = "%" in split.fragment
        replacement = quote(replacement_fragment, safe="-_") if encoded else replacement_fragment
        suffix = ">" if raw.startswith("<") and raw.endswith(">") else ""
        replacements.append((start, parsed.pos, raw[:fragment_start + 1] + replacement + suffix))
    return replace_spans(text, replacements), len(replacements)


def load_course_map(path: Path) -> list[dict]:
    data = json.loads(path.read_text(encoding="utf-8-sig"))
    chapters = data["chapters"]
    if data.get("version") != 1 or not chapters:
        raise ValueError("Unsupported or empty course map")
    keys: set[str] = set()
    paths: set[str] = set()
    for expected, chapter in enumerate(chapters, 1):
        if chapter["number"] != expected or chapter["key"] in keys or not chapter["articles"]:
            raise ValueError("Course chapters must be sequential, unique and nonempty")
        keys.add(chapter["key"])
        for filename in chapter["articles"]:
            relative = Path(chapter["directory"]) / filename
            if relative.is_absolute() or ".." in relative.parts or relative.suffix.lower() != ".md":
                raise ValueError(f"Invalid course path: {relative}")
            if relative.as_posix().casefold() in paths:
                raise ValueError(f"Duplicate course path: {relative}")
            paths.add(relative.as_posix().casefold())
    return chapters


def navigation(text: str, source: Path, articles: list[tuple[Path, str, str]], index: int, root: Path) -> str:
    # Only replace our own marker block; authored code is never a marker.
    mask = code_mask(text)
    removals = []
    pattern = re.compile(r"(?m)^" + re.escape(NAV_START) + r"\n.*?^" + re.escape(NAV_END) + r"\n*", re.S)
    for match in pattern.finditer(text):
        if not mask[match.start()]:
            removals.append((match.start(), match.end(), ""))
    text = replace_spans(text, removals)
    links = []
    for position, label in ((index - 1, "上一节"), (index + 1, "下一节")):
        if 0 <= position < len(articles):
            path, prefix, title = articles[position]
            relative = Path(os.path.relpath(path, source.parent)).as_posix()
            safe_title = title.replace("[", r"\[").replace("]", r"\]")
            links.append(f"[{label}：{prefix} {safe_title}]({relative})")
    links.insert(1 if links else 0, "[本章目录](README.md)")
    home = Path(os.path.relpath(root / "README.md", source.parent)).as_posix()
    links.append(f"[手册首页]({home})")
    block = NAV_START + "\n" + " · ".join(links) + "\n" + NAV_END + "\n\n"
    token = next(t for t in PARSER.parse(text) if t.type == "heading_open" and t.tag == "h1")
    assert token.map is not None
    end = line_offsets(text)[token.map[1]]
    tail = text[end:].lstrip("\n")
    first_line = tail.split("\n", 1)[0]
    old_navigation = {
        "[返回本章目录](README.md) · [返回手册首页](../README.md)",
        "[本章目录](README.md)",
        "[返回本章目录](README.md)",
        "[本章目录](README.md) · [手册首页](../README.md)",
    }
    if first_line in old_navigation:
        tail = tail[len(first_line):].lstrip("\n")
    return text[:end].rstrip("\n") + "\n\n" + block + tail


def markdown_files(root: Path) -> list[Path]:
    """Prune archives and generated trees before walking into them."""
    found: set[Path] = set()
    for folder, directories, filenames in os.walk(root, topdown=True, followlinks=False):
        directories[:] = [name for name in directories if name not in IGNORED_DIRS and not name.startswith(".")]
        for name in filenames:
            if Path(name).suffix.lower() != ".md":
                continue
            path = (Path(folder) / name).resolve()
            try:
                path.relative_to(root)
            except ValueError:
                continue
            found.add(path)
    return sorted(found)


def plan(root: Path, config: Path, with_navigation: bool) -> tuple[dict[Path, str], dict]:
    root = root.resolve()
    files = markdown_files(root)
    originals = {p: p.read_text(encoding="utf-8-sig") for p in files}
    texts = dict(originals)
    mappings = {}
    articles = []
    heading_count = 0
    for chapter in load_course_map(config):
        for index, filename in enumerate(chapter["articles"], 1):
            path = (root / chapter["directory"] / filename).resolve()
            if path not in texts:
                raise FileNotFoundError(f"Course article missing: {path}")
            prefix = f"{chapter['number']}.{index}"
            texts[path], mappings[path], title, count = number_headings(texts[path], prefix)
            articles.append((path, prefix, title))
            heading_count += count
    if with_navigation:
        for index, (path, _prefix, _title) in enumerate(articles):
            texts[path] = navigation(texts[path], path, articles, index, root)
    link_count = 0
    for path, text in texts.items():
        texts[path], count = rewrite_links(text, path, root, mappings)
        link_count += count
    changes = {path: text for path, text in texts.items() if text != originals[path]}
    report = {
        "articles": len(articles), "headings": heading_count,
        "rewritten_fragments": link_count, "changed_files": len(changes),
        "paths": [path.relative_to(root).as_posix() for path in changes],
    }
    return changes, report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--map", type=Path, default=Path(__file__).with_name("course-map.json"))
    parser.add_argument("--navigation", action="store_true", help="Add/update previous/next article navigation")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--write", action="store_true")
    mode.add_argument("--check", action="store_true")
    args = parser.parse_args()
    changes, report = plan(args.root, args.map, args.navigation)
    if args.write:
        for path, text in changes.items():
            path.write_text(text, encoding="utf-8", newline="\n")
    report["mode"] = "write" if args.write else "check" if args.check else "preview"
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 1 if args.check and changes else 0


if __name__ == "__main__":
    raise SystemExit(main())
