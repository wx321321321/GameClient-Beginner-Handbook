#!/usr/bin/env python3
"""Build Apple Books-friendly EPUB 3 files from the handbook Markdown notes.

The builder intentionally has no Pandoc/Calibre dependency.  It creates a
standards-shaped, reflowable EPUB, embeds referenced local images, generates
navigation, and validates the resulting archive before reporting success.
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import mimetypes
import os
import posixpath
import re
import sys
import unicodedata
import uuid
import zipfile
from dataclasses import dataclass, field
from datetime import datetime, timezone
from html import escape
from pathlib import Path, PurePosixPath
from typing import Iterable
from urllib.parse import unquote, urlsplit

from lxml import etree, html as lxml_html
from markdown_it import MarkdownIt
from mdit_py_plugins.footnote import footnote_plugin
from mdit_py_plugins.tasklists import tasklists_plugin
from PIL import Image, ImageDraw, ImageFont


XHTML_NS = "http://www.w3.org/1999/xhtml"
EPUB_NS = "http://www.idpf.org/2007/ops"
OPF_NS = "http://www.idpf.org/2007/opf"
DC_NS = "http://purl.org/dc/elements/1.1/"
NCX_NS = "http://www.daisy.org/z3986/2005/ncx/"


@dataclass(frozen=True)
class VolumeSpec:
    source_dir: str
    title: str
    part_label: str
    order: tuple[str, ...]
    accent: tuple[int, int, int]


@dataclass(frozen=True)
class BookSpec:
    title: str
    filename: str
    cover_lines: tuple[str, ...]
    cover_mark: str
    volumes: tuple[VolumeSpec, ...]
    accent: tuple[int, int, int]
    combined: bool = False


@dataclass
class Heading:
    level: int
    text: str
    element_id: str


@dataclass
class Chapter:
    source: Path
    volume: VolumeSpec
    volume_index: int
    index_in_volume: int
    global_index: int
    href: str
    title: str = ""
    headings: list[Heading] = field(default_factory=list)
    xhtml: bytes = b""


@dataclass
class PartPage:
    volume: VolumeSpec
    volume_index: int
    href: str
    xhtml: bytes = b""


@dataclass
class Asset:
    source: Path
    href: str
    media_type: str
    data: bytes


CPP = VolumeSpec(
    source_dir="01_CPP&Computer_Basics",
    title="C++ 与计算机基础",
    part_label="第一编",
    order=(
        "computer-architecture.md",
        "operating-systems.md",
        "computer-networks.md",
        "memory-and-compilation.md",
        "cpp-keywords-and-callables.md",
        "pointers-and-references.md",
        "classes-inheritance-and-polymorphism.md",
        "templates.md",
        "stl-and-algorithms.md",
        "processes-threads-and-synchronization.md",
        "design-patterns.md",
        "lua.md",
        "rotations-and-quaternions.md",
    ),
    accent=(62, 190, 204),
)

GRAPHICS = VolumeSpec(
    source_dir="02_Graphics",
    title="计算机图形学",
    part_label="第二编",
    order=("transforms-and-skinning.md", "rendering-and-gpu.md"),
    accent=(244, 147, 61),
)

UE = VolumeSpec(
    source_dir="03_UE",
    title="Unreal Engine",
    part_label="第三编",
    order=(
        "animation-overview.md",
        "garbage-collection-and-reflection.md",
        "runtime-type-checking.md",
        "animation-system-comparison.md",
    ),
    accent=(118, 126, 247),
)


BOOKS = (
    BookSpec(
        title="C++ 与计算机基础",
        filename="01_CPP与计算机基础_AppleBooks.epub",
        cover_lines=("C++ 与", "计算机基础"),
        cover_mark="C++",
        volumes=(CPP,),
        accent=CPP.accent,
    ),
    BookSpec(
        title="计算机图形学",
        filename="02_计算机图形学_AppleBooks.epub",
        cover_lines=("计算机图形学",),
        cover_mark="GFX",
        volumes=(GRAPHICS,),
        accent=GRAPHICS.accent,
    ),
    BookSpec(
        title="Unreal Engine 学习笔记",
        filename="03_UnrealEngine学习笔记_AppleBooks.epub",
        cover_lines=("Unreal Engine", "学习笔记"),
        cover_mark="UE",
        volumes=(UE,),
        accent=UE.accent,
    ),
    BookSpec(
        title="游戏客户端入门手册",
        filename="游戏客户端入门手册_合订本_AppleBooks.epub",
        cover_lines=("游戏客户端", "入门手册"),
        cover_mark="DEV",
        volumes=(CPP, GRAPHICS, UE),
        accent=(76, 176, 255),
        combined=True,
    ),
)


BOOK_CSS = r"""@charset "UTF-8";
:root {
  color-scheme: light dark;
  --ink: #20242b;
  --muted: #667080;
  --line: #d9dee7;
  --soft: #f3f5f8;
  --accent: #287f8b;
}
html { -webkit-text-size-adjust: 100%; }
body {
  margin: 0;
  padding: 1.2em 1em 2.5em;
  color: var(--ink);
  background: transparent;
  font-family: -apple-system, BlinkMacSystemFont, "PingFang SC", "Hiragino Sans GB",
               "Microsoft YaHei", "Noto Sans CJK SC", sans-serif;
  font-size: 1em;
  line-height: 1.78;
  text-align: justify;
  -webkit-hyphens: auto;
  hyphens: auto;
  overflow-wrap: break-word;
}
main { max-width: 46em; margin: 0 auto; }
.chapter-kicker, .part-kicker {
  margin: 0 0 .9em;
  color: var(--accent);
  font-size: .78em;
  font-weight: 700;
  letter-spacing: .08em;
  text-align: left;
}
h1, h2, h3, h4, h5, h6 {
  color: inherit;
  line-height: 1.35;
  text-align: left;
  page-break-after: avoid;
  break-after: avoid;
  -webkit-hyphens: none;
  hyphens: none;
}
h1 { margin: .2em 0 1em; font-size: 1.85em; }
h2 { margin: 1.8em 0 .75em; padding-bottom: .25em; border-bottom: .08em solid var(--line); font-size: 1.42em; }
h3 { margin: 1.55em 0 .6em; font-size: 1.18em; }
h4 { margin: 1.35em 0 .45em; font-size: 1.04em; }
h5, h6 { margin: 1.2em 0 .4em; font-size: .98em; }
p { margin: .72em 0; orphans: 2; widows: 2; }
ul, ol { margin: .65em 0 .85em; padding-left: 1.55em; }
li { margin: .26em 0; }
li > p { margin: .25em 0; }
a { color: #16738b; text-decoration: none; }
a:visited { color: #6658a6; }
strong { font-weight: 700; }
blockquote {
  margin: 1em 0;
  padding: .25em .9em;
  border-left: .25em solid var(--accent);
  color: #4f5968;
  background: var(--soft);
}
blockquote > :first-child { margin-top: .35em; }
blockquote > :last-child { margin-bottom: .35em; }
code, pre {
  font-family: ui-monospace, "SFMono-Regular", Menlo, Monaco, Consolas,
               "Liberation Mono", "Courier New", monospace;
  direction: ltr;
  text-align: left;
}
:not(pre) > code {
  padding: .08em .32em;
  border: .06em solid var(--line);
  border-radius: .28em;
  background: var(--soft);
  font-size: .88em;
  overflow-wrap: anywhere;
}
pre {
  position: relative;
  margin: 1em 0;
  padding: 1em;
  border: .06em solid var(--line);
  border-left: .28em solid var(--accent);
  border-radius: .45em;
  background: var(--soft);
  font-size: .78em;
  line-height: 1.55;
  white-space: pre-wrap;
  word-break: break-word;
  overflow-wrap: anywhere;
  page-break-inside: avoid;
}
pre[data-language]::before {
  content: attr(data-language);
  display: block;
  margin: -.45em 0 .55em;
  color: var(--muted);
  font-family: -apple-system, BlinkMacSystemFont, sans-serif;
  font-size: .78em;
  font-weight: 700;
  letter-spacing: .06em;
  text-transform: uppercase;
}
pre code { padding: 0; border: 0; background: transparent; font-size: 1em; }
table {
  display: block;
  width: 100%;
  max-width: 100%;
  margin: 1.1em 0;
  border-collapse: collapse;
  overflow-x: auto;
  font-size: .86em;
  text-align: left;
}
th, td { padding: .52em .62em; border: .06em solid var(--line); vertical-align: top; }
th { background: var(--soft); font-weight: 700; }
tr { page-break-inside: avoid; }
img, svg { display: block; max-width: 100%; height: auto; margin: 1.1em auto; page-break-inside: avoid; }
hr { margin: 2.2em 0 1.2em; border: 0; border-top: .06em solid var(--line); }
.source-note, .chapter-nav { color: var(--muted); font-size: .78em; text-align: left; }
.chapter-nav { margin-top: 1.4em; }
.empty-note, .missing-image {
  padding: .8em 1em;
  border: .06em dashed var(--line);
  border-radius: .4em;
  color: var(--muted);
  background: var(--soft);
  text-align: left;
}
.task-list-item { list-style-type: none; }
.task-list-item-checkbox { margin-right: .5em; }
.footnotes { margin-top: 2em; border-top: .06em solid var(--line); font-size: .86em; }
.part-page { padding-top: 18vh; }
.part-number { color: var(--accent); font-size: .9em; font-weight: 700; letter-spacing: .1em; }
.part-title { margin-top: .25em; font-size: 2.15em; }
.part-list { margin-top: 2em; }
.toc h1 { margin-bottom: 1.2em; }
.toc ol { padding-left: 1.35em; }
.toc li { margin: .42em 0; text-align: left; }
.toc li ol { margin-top: .3em; font-size: .9em; }
.cover-body { margin: 0; padding: 0; text-align: center; }
.cover-body img { width: 100%; height: auto; margin: 0; }
@media (prefers-color-scheme: dark) {
  :root { --ink: #edf1f6; --muted: #aeb8c5; --line: #4b5360; --soft: #262c35; }
  a { color: #70c7dc; }
  a:visited { color: #b7a9f4; }
  blockquote { color: #c4ccd7; }
}
@media (max-width: 30em) {
  body { padding: .8em .65em 2em; }
  h1 { font-size: 1.62em; }
  h2 { font-size: 1.3em; }
  pre { padding: .75em; font-size: .73em; }
  th, td { padding: .42em .48em; }
}
"""


CONTAINER_XML = b"""<?xml version="1.0" encoding="UTF-8"?>
<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
  <rootfiles>
    <rootfile full-path="EPUB/package.opf" media-type="application/oebps-package+xml"/>
  </rootfiles>
</container>
"""


def norm_path_key(path: Path) -> str:
    return os.path.normcase(str(path.resolve()))


def read_markdown(path: Path, warnings: list[str]) -> str:
    raw = path.read_bytes()
    for encoding in ("utf-8-sig", "gb18030"):
        try:
            text = raw.decode(encoding)
            if encoding != "utf-8-sig":
                warnings.append(f"{path}: used fallback encoding {encoding}")
            return text.replace("\r\n", "\n").replace("\r", "\n")
        except UnicodeDecodeError:
            pass
    warnings.append(f"{path}: undecodable bytes replaced")
    return raw.decode("utf-8", errors="replace")


def clean_text(value: str) -> str:
    return re.sub(r"\s+", " ", value).strip()


def humanize_stem(path: Path) -> str:
    text = path.stem.replace("_", " ").replace("&", " & ")
    return clean_text(text) or "未命名章节"


def heading_slug(text: str, number: int) -> str:
    normalized = unicodedata.normalize("NFKC", text).casefold()
    normalized = re.sub(r"[^\w\u3400-\u9fff]+", "-", normalized, flags=re.UNICODE)
    normalized = normalized.strip("-")[:48]
    digest = hashlib.sha1(text.encode("utf-8")).hexdigest()[:7]
    if normalized and normalized.isascii():
        suffix = normalized
    else:
        suffix = digest
    return f"sec-{number:03d}-{suffix}"


def discover_markdowns(root: Path, volume: VolumeSpec) -> list[Path]:
    source_dir = root / volume.source_dir
    # README files are repository navigation; EPUB navigation is generated below.
    discovered = [
        p for p in source_dir.rglob("*")
        if p.is_file() and p.suffix.lower() == ".md" and p.name.casefold() != "readme.md"
    ]
    by_relative = {p.relative_to(source_dir).as_posix().casefold(): p for p in discovered}
    ordered: list[Path] = []
    seen: set[str] = set()
    for name in volume.order:
        key = PurePosixPath(name).as_posix().casefold()
        path = by_relative.get(key)
        if path is not None:
            ordered.append(path)
            seen.add(norm_path_key(path))
    ordered.extend(
        sorted(
            (p for p in discovered if norm_path_key(p) not in seen),
            key=lambda p: p.relative_to(source_dir).as_posix().casefold(),
        )
    )
    return ordered


def make_markdown_renderer() -> MarkdownIt:
    renderer = MarkdownIt(
        "gfm-like",
        {
            "html": False,
            "linkify": False,
            "typographer": False,
            "breaks": True,
        },
    )
    # The notes contain C++ spellings such as new[](size_t), which CommonMark
    # otherwise mistakes for an empty Markdown link.  There are no authored
    # hyperlinks in the source set, while the separate image rule remains on.
    renderer.disable(["link", "autolink", "linkify"])
    renderer.use(tasklists_plugin, enabled=True, label=True)
    renderer.use(footnote_plugin)
    return renderer


def is_ascii_visual_line(line: str) -> bool:
    """Recognize hand-drawn tables/packet diagrams that need monospace layout."""
    stripped = line.strip()
    if len(stripped) < 8:
        return False
    if any(character in stripped for character in "┌┐└┘├┤┬┴┼│━─"):
        return True
    if stripped.count("|") >= 2:
        return True
    if (stripped.count("+") >= 2 and stripped.count("-") >= 6) or stripped.count("-") >= 24:
        return True
    if re.fullmatch(r"[\d\s]+", stripped) and len(stripped) >= 30:
        return True
    if len(re.findall(r"(?: {2,}|\t+)", line)) >= 2 and len(stripped) >= 24:
        return True
    return False


def prepare_markdown(source_text: str) -> str:
    """Apply presentation-only fixes without changing any source Markdown file."""
    repository_navigation = "[返回本章目录](README.md) · [返回手册首页](../README.md)"
    expanded: list[str] = []
    fence_character = ""
    fence_length = 0
    for line in source_text.split("\n"):
        match = re.match(r"^\s{0,3}(`{3,}|~{3,})", line)
        if match:
            marker = match.group(1)
            if not fence_character:
                fence_character, fence_length = marker[0], len(marker)
            elif marker[0] == fence_character and len(marker) >= fence_length:
                fence_character, fence_length = "", 0
            expanded.append(line)
            continue
        if fence_character:
            expanded.append(line)
        elif line == repository_navigation:
            # EPUB has its own navigation. Keep the same text in code samples.
            continue
        else:
            # These notes use the two literal characters "\n" as a visual line break.
            expanded.extend(line.split(r"\n"))

    outside_fence: list[bool] = []
    fence_character = ""
    fence_length = 0
    for line in expanded:
        match = re.match(r"^\s{0,3}(`{3,}|~{3,})", line)
        if match:
            marker = match.group(1)
            outside_fence.append(False)
            if not fence_character:
                fence_character, fence_length = marker[0], len(marker)
            elif marker[0] == fence_character and len(marker) >= fence_length:
                fence_character, fence_length = "", 0
            continue
        outside_fence.append(not fence_character)

    result: list[str] = []
    index = 0
    while index < len(expanded):
        if outside_fence[index] and is_ascii_visual_line(expanded[index]):
            end = index
            while end < len(expanded) and outside_fence[end] and is_ascii_visual_line(expanded[end]):
                end += 1
            if end - index >= 3:
                result.append("```text")
                result.extend(expanded[index:end])
                result.append("```")
                index = end
                continue
        result.append(expanded[index])
        index += 1
    return "\n".join(result)


class AssetRegistry:
    def __init__(self) -> None:
        self._by_source: dict[str, Asset] = {}
        self.assets: list[Asset] = []

    def add(self, path: Path) -> Asset:
        key = norm_path_key(path)
        if key in self._by_source:
            return self._by_source[key]
        data = path.read_bytes()
        extension = path.suffix.lower()
        extension = ".jpg" if extension == ".jpeg" else extension
        media_type = mimetypes.guess_type(path.name)[0] or "application/octet-stream"
        if media_type == "image/jpg":
            media_type = "image/jpeg"
        digest = hashlib.sha256(data).hexdigest()[:14]
        href = f"images/img-{len(self.assets) + 1:03d}-{digest}{extension}"
        asset = Asset(source=path, href=href, media_type=media_type, data=data)
        self.assets.append(asset)
        self._by_source[key] = asset
        return asset


def is_external_url(value: str) -> bool:
    scheme = urlsplit(value).scheme.casefold()
    return scheme in {"http", "https", "mailto", "tel", "data"}


def resolve_local_reference(source: Path, value: str, repo_root: Path) -> Path | None:
    split = urlsplit(value)
    candidate_text = unquote(split.path).replace("/", os.sep)
    if not candidate_text:
        return None
    if candidate_text.startswith(os.sep):
        candidate = repo_root / candidate_text.lstrip("\\/")
    else:
        candidate = source.parent / candidate_text
    try:
        return candidate.resolve()
    except OSError:
        return None


def replace_element(element: etree._Element, replacement: etree._Element) -> None:
    replacement.tail = element.tail
    parent = element.getparent()
    if parent is not None:
        parent.replace(element, replacement)


def serialize_children(wrapper: etree._Element) -> str:
    chunks: list[str] = []
    if wrapper.text:
        chunks.append(escape(wrapper.text))
    for child in wrapper:
        chunks.append(etree.tostring(child, encoding="unicode", method="xml"))
    return "".join(chunks)


def xhtml_document(
    title: str,
    body: str,
    body_class: str = "",
    stylesheet_href: str = "../styles/book.css",
) -> bytes:
    class_attribute = f' class="{escape(body_class, quote=True)}"' if body_class else ""
    document = f"""<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE html>
<html xmlns="{XHTML_NS}" xmlns:epub="{EPUB_NS}" xml:lang="zh-CN" lang="zh-CN">
  <head>
    <meta charset="utf-8"/>
    <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
    <title>{escape(title)}</title>
    <link rel="stylesheet" type="text/css" href="{escape(stylesheet_href, quote=True)}"/>
  </head>
  <body{class_attribute}>
    {body}
  </body>
</html>
"""
    return document.encode("utf-8")


def render_chapter(
    chapter: Chapter,
    repo_root: Path,
    book_title: str,
    markdown: MarkdownIt,
    assets: AssetRegistry,
    chapter_lookup: dict[str, Chapter],
    warnings: list[str],
) -> None:
    source_text = read_markdown(chapter.source, warnings)
    prepared_text = prepare_markdown(source_text)
    rendered = markdown.render(prepared_text) if source_text.strip() else ""
    wrapper = lxml_html.fragment_fromstring(rendered or "<div></div>", create_parent="div")

    heading_nodes = wrapper.xpath(".//h1 | .//h2 | .//h3 | .//h4 | .//h5 | .//h6")
    chapter.title = clean_text("".join(heading_nodes[0].itertext())) if heading_nodes else humanize_stem(chapter.source)
    if not chapter.title:
        chapter.title = humanize_stem(chapter.source)

    if not heading_nodes:
        title_node = etree.Element("h1")
        title_node.text = chapter.title
        wrapper.insert(0, title_node)
        heading_nodes = [title_node]
        if not source_text.strip():
            note = etree.Element("p")
            note.set("class", "empty-note")
            note.text = "该 Markdown 文件当前为空。"
            wrapper.append(note)

    # Every XHTML document gets one H1.  Subsequent source headings are shifted
    # just enough to form a valid, readable hierarchy without changing wording.
    previous_level = 1
    first_source_level = int(heading_nodes[0].tag[-1])
    for number, node in enumerate(heading_nodes):
        if number == 0:
            normalized_level = 1
        else:
            source_level = int(node.tag[-1])
            proposed_level = 2 if source_level <= first_source_level else source_level - first_source_level + 1
            normalized_level = min(6, proposed_level, previous_level + 1)
            normalized_level = max(2, normalized_level)
        node.tag = f"h{normalized_level}"
        previous_level = normalized_level

    used_ids: set[str] = set()
    for number, node in enumerate(heading_nodes, start=1):
        text = clean_text("".join(node.itertext())) or f"小节 {number}"
        candidate = heading_slug(text, number)
        while candidate in used_ids:
            candidate += "-x"
        used_ids.add(candidate)
        node.set("id", candidate)
        chapter.headings.append(Heading(level=int(node.tag[-1]), text=text, element_id=candidate))

    for pre in wrapper.xpath(".//pre"):
        code_nodes = pre.xpath("./code")
        if not code_nodes:
            continue
        classes = (code_nodes[0].get("class") or "").split()
        language = next((value[9:] for value in classes if value.startswith("language-")), "")
        if language:
            pre.set("data-language", language)

    for image in list(wrapper.xpath(".//img")):
        value = (image.get("src") or "").strip()
        if not value:
            continue
        if is_external_url(value):
            warnings.append(f"{chapter.source}: remote image left as link: {value}")
            link = etree.Element("a")
            link.set("href", value)
            link.set("class", "missing-image")
            link.text = clean_text(image.get("alt") or "外部图片")
            replace_element(image, link)
            continue
        path = resolve_local_reference(chapter.source, value, repo_root)
        if path is None or not path.is_file():
            warnings.append(f"{chapter.source}: missing image: {value}")
            missing = etree.Element("span")
            missing.set("class", "missing-image")
            missing.text = f"图片缺失：{value}"
            replace_element(image, missing)
            continue
        asset = assets.add(path)
        image.set("src", f"../{asset.href}")
        image.set("alt", clean_text(image.get("alt") or path.stem))
        image.attrib.pop("srcset", None)
        image.attrib.pop("loading", None)

    for link in wrapper.xpath(".//a[@href]"):
        value = (link.get("href") or "").strip()
        if not value or value.startswith("#") or is_external_url(value):
            if is_external_url(value):
                link.set("rel", "external")
            continue
        target = resolve_local_reference(chapter.source, value, repo_root)
        target_chapter = chapter_lookup.get(norm_path_key(target)) if target is not None else None
        if target_chapter is not None:
            fragment = urlsplit(value).fragment
            link.set("href", target_chapter.href.split("/", 1)[-1] + (f"#{fragment}" if fragment else ""))

    relative_source = chapter.source.relative_to(repo_root).as_posix()
    chapter_number = f"第 {chapter.index_in_volume} 章"
    if len({item.volume_index for item in chapter_lookup.values()}) > 1:
        chapter_number = f"{chapter.volume.part_label} · {chapter_number}"
    fragment = serialize_children(wrapper)
    body = f"""<main epub:type="chapter">
      <p class="chapter-kicker">{escape(chapter_number)} · {escape(chapter.volume.title)}</p>
      <div class="markdown-body">{fragment}</div>
      <hr/>
      <p class="source-note">来源：{escape(relative_source)}</p>
      <p class="chapter-nav"><a href="../nav.xhtml">返回目录</a></p>
    </main>"""
    chapter.xhtml = xhtml_document(f"{chapter.title} — {book_title}", body)


def nav_subheadings(chapter: Chapter) -> list[Heading]:
    if len(chapter.headings) <= 1:
        return []
    return [heading for heading in chapter.headings[1:] if heading.level <= 2]


def build_part_page(part: PartPage, chapters: list[Chapter], book_title: str) -> None:
    links = "".join(
        f'<li><a href="{escape(chapter.href.split("/", 1)[-1], quote=True)}">{escape(chapter.title)}</a></li>'
        for chapter in chapters
    )
    body = f"""<main class="part-page" epub:type="part">
      <p class="part-number">{escape(part.volume.part_label)}</p>
      <h1 class="part-title">{escape(part.volume.title)}</h1>
      <p>本编收录 {len(chapters)} 篇 Markdown 笔记。</p>
      <ol class="part-list">{links}</ol>
      <p class="chapter-nav"><a href="../nav.xhtml">返回目录</a></p>
    </main>"""
    part.xhtml = xhtml_document(f"{part.volume.part_label} {part.volume.title} — {book_title}", body)


def make_cover_png(book: BookSpec, chapter_count: int) -> bytes:
    width, height = 1600, 2560
    accent = book.accent
    top = (16, 25, 40)
    bottom = (5, 10, 19)
    image = Image.new("RGB", (width, height), top)
    draw = ImageDraw.Draw(image)
    for y in range(height):
        fraction = y / (height - 1)
        color = tuple(round(top[i] * (1 - fraction) + bottom[i] * fraction) for i in range(3))
        draw.line((0, y, width, y), fill=color)

    overlay = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    odraw = ImageDraw.Draw(overlay)
    for x in range(-height, width + height, 210):
        odraw.line((x, 0, x + height, height), fill=(*accent, 16), width=3)
    for radius, alpha in ((520, 18), (360, 22), (210, 28)):
        cx, cy = 1380, 420
        odraw.ellipse((cx - radius, cy - radius, cx + radius, cy + radius), outline=(*accent, alpha), width=5)
    odraw.rounded_rectangle((92, 92, 1508, 2468), radius=38, outline=(*accent, 90), width=3)
    image = Image.alpha_composite(image.convert("RGBA"), overlay)
    draw = ImageDraw.Draw(image)

    font_candidates = (
        Path(r"C:\Windows\Fonts\msyhbd.ttc"),
        Path(r"C:\Windows\Fonts\msyh.ttc"),
        Path(r"C:\Windows\Fonts\simhei.ttf"),
    )
    font_path = next((path for path in font_candidates if path.is_file()), None)

    def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
        chosen = Path(r"C:\Windows\Fonts\msyhbd.ttc") if bold and Path(r"C:\Windows\Fonts\msyhbd.ttc").is_file() else font_path
        return ImageFont.truetype(str(chosen), size=size) if chosen else ImageFont.load_default()

    small = font(38)
    mark_font = font(86, bold=True)
    title_font = font(148, bold=True)
    subtitle_font = font(46)
    meta_font = font(34)

    draw.rounded_rectangle((120, 150, 430, 285), radius=28, fill=(*accent, 255))
    draw.text((155, 169), book.cover_mark, font=mark_font, fill=(6, 14, 24, 255))
    draw.text((120, 365), "GAME CLIENT · BEGINNER HANDBOOK", font=small, fill=(*accent, 255))

    y = 790
    for line in book.cover_lines:
        current_font = title_font
        while draw.textbbox((0, 0), line, font=current_font)[2] > 1360 and getattr(current_font, "size", 0) > 92:
            current_font = font(current_font.size - 8, bold=True)
        draw.text((120, y), line, font=current_font, fill=(245, 248, 252, 255))
        y += int(getattr(current_font, "size", 120) * 1.35)

    draw.rounded_rectangle((120, y + 55, 510, y + 69), radius=7, fill=(*accent, 255))
    draw.text((120, y + 135), "可重排 EPUB · Apple Books 版", font=subtitle_font, fill=(204, 215, 228, 255))
    draw.text((120, y + 220), f"{chapter_count} 篇 Markdown 笔记 · 内嵌目录与图片", font=meta_font, fill=(151, 166, 184, 255))
    draw.text((120, 2320), "GAMECLIENT-BEGINNER-HANDBOOK", font=meta_font, fill=(151, 166, 184, 255))

    buffer = io.BytesIO()
    image.convert("RGB").save(buffer, format="PNG", optimize=True)
    return buffer.getvalue()


def cover_xhtml(book: BookSpec) -> bytes:
    body = f"""<section epub:type="cover">
      <img src="../images/cover.png" alt="{escape(book.title, quote=True)}封面"/>
    </section>"""
    return xhtml_document(f"{book.title}封面", body, body_class="cover-body")


def make_nav(book: BookSpec, chapters: list[Chapter], parts: list[PartPage]) -> bytes:
    part_by_index = {part.volume_index: part for part in parts}

    def chapter_item(chapter: Chapter) -> str:
        subitems = nav_subheadings(chapter)
        children = ""
        if subitems:
            children = "<ol>" + "".join(
                f'<li><a href="text/{escape(chapter.href.split("/", 1)[-1], quote=True)}#{escape(item.element_id, quote=True)}">{escape(item.text)}</a></li>'
                for item in subitems
            ) + "</ol>"
        return f'<li><a href="{escape(chapter.href, quote=True)}">{escape(chapter.title)}</a>{children}</li>'

    if book.combined:
        entries: list[str] = []
        for volume_index, volume in enumerate(book.volumes, start=1):
            part = part_by_index[volume_index]
            nested = "".join(chapter_item(chapter) for chapter in chapters if chapter.volume_index == volume_index)
            entries.append(
                f'<li><a href="{escape(part.href, quote=True)}">{escape(volume.part_label)} · {escape(volume.title)}</a><ol>{nested}</ol></li>'
            )
        toc_entries = "".join(entries)
    else:
        toc_entries = "".join(chapter_item(chapter) for chapter in chapters)

    body = f"""<main class="toc">
      <nav epub:type="toc" id="toc" role="doc-toc">
        <h1>目录</h1>
        <ol>{toc_entries}</ol>
      </nav>
      <nav epub:type="landmarks" hidden="hidden">
        <ol>
          <li><a epub:type="cover" href="text/cover.xhtml">封面</a></li>
          <li><a epub:type="toc" href="#toc">目录</a></li>
          <li><a epub:type="bodymatter" href="{escape((parts[0].href if parts else chapters[0].href), quote=True)}">正文</a></li>
        </ol>
      </nav>
    </main>"""
    return xhtml_document(f"目录 — {book.title}", body, stylesheet_href="styles/book.css")


def make_ncx(book: BookSpec, book_id: str, chapters: list[Chapter], parts: list[PartPage]) -> bytes:
    order = 1

    def point(label: str, src: str) -> str:
        nonlocal order
        current = order
        order += 1
        return f"""<navPoint id="nav-{current}" playOrder="{current}">
          <navLabel><text>{escape(label)}</text></navLabel>
          <content src="{escape(src, quote=True)}"/>
        </navPoint>"""

    if book.combined:
        nav_points: list[str] = []
        part_by_index = {part.volume_index: part for part in parts}
        for volume_index, volume in enumerate(book.volumes, start=1):
            nav_points.append(point(f"{volume.part_label} · {volume.title}", part_by_index[volume_index].href))
            nav_points.extend(
                point(chapter.title, chapter.href)
                for chapter in chapters
                if chapter.volume_index == volume_index
            )
        nav_map = "".join(nav_points)
        depth = 1
    else:
        nav_map = "".join(point(chapter.title, chapter.href) for chapter in chapters)
        depth = 1

    ncx = f"""<?xml version="1.0" encoding="utf-8"?>
<ncx xmlns="{NCX_NS}" version="2005-1" xml:lang="zh-CN">
  <head>
    <meta name="dtb:uid" content="urn:uuid:{book_id}"/>
    <meta name="dtb:depth" content="{depth}"/>
    <meta name="dtb:totalPageCount" content="0"/>
    <meta name="dtb:maxPageNumber" content="0"/>
  </head>
  <docTitle><text>{escape(book.title)}</text></docTitle>
  <navMap>{nav_map}</navMap>
</ncx>
"""
    return ncx.encode("utf-8")


def make_package(
    book: BookSpec,
    book_id: str,
    modified: str,
    chapters: list[Chapter],
    parts: list[PartPage],
    assets: AssetRegistry,
) -> bytes:
    manifest = [
        '<item id="nav" href="nav.xhtml" media-type="application/xhtml+xml" properties="nav"/>',
        '<item id="ncx" href="toc.ncx" media-type="application/x-dtbncx+xml"/>',
        '<item id="css" href="styles/book.css" media-type="text/css"/>',
        '<item id="cover-page" href="text/cover.xhtml" media-type="application/xhtml+xml"/>',
        '<item id="cover-image" href="images/cover.png" media-type="image/png" properties="cover-image"/>',
    ]
    spine = ['<itemref idref="cover-page" linear="yes"/>']
    for part in parts:
        item_id = f"part-{part.volume_index:02d}"
        manifest.append(f'<item id="{item_id}" href="{escape(part.href, quote=True)}" media-type="application/xhtml+xml"/>')
    for chapter in chapters:
        item_id = f"chapter-{chapter.global_index:03d}"
        manifest.append(f'<item id="{item_id}" href="{escape(chapter.href, quote=True)}" media-type="application/xhtml+xml"/>')
    for number, asset in enumerate(assets.assets, start=1):
        manifest.append(
            f'<item id="image-{number:03d}" href="{escape(asset.href, quote=True)}" media-type="{escape(asset.media_type, quote=True)}"/>'
        )

    for volume_index, _volume in enumerate(book.volumes, start=1):
        matching_part = next((part for part in parts if part.volume_index == volume_index), None)
        if matching_part is not None:
            spine.append(f'<itemref idref="part-{volume_index:02d}"/>')
        for chapter in chapters:
            if chapter.volume_index == volume_index:
                spine.append(f'<itemref idref="chapter-{chapter.global_index:03d}"/>')

    package = f"""<?xml version="1.0" encoding="utf-8"?>
<package xmlns="{OPF_NS}" version="3.0" unique-identifier="book-id" xml:lang="zh-CN" prefix="rendition: http://www.idpf.org/vocab/rendition/#">
  <metadata xmlns:dc="{DC_NS}">
    <dc:identifier id="book-id">urn:uuid:{book_id}</dc:identifier>
    <dc:title>{escape(book.title)}</dc:title>
    <dc:language>zh-CN</dc:language>
    <dc:creator>GameClient-Beginner-Handbook</dc:creator>
    <dc:subject>游戏客户端开发</dc:subject>
    <dc:description>由当前 Markdown 学习笔记生成的可重排 EPUB 电子书。</dc:description>
    <meta property="dcterms:modified">{modified}</meta>
    <meta property="rendition:layout">reflowable</meta>
    <meta property="rendition:orientation">auto</meta>
    <meta property="rendition:spread">auto</meta>
    <meta name="cover" content="cover-image"/>
  </metadata>
  <manifest>{''.join(manifest)}</manifest>
  <spine toc="ncx" page-progression-direction="ltr">{''.join(spine)}</spine>
  <guide><reference type="cover" title="封面" href="text/cover.xhtml"/></guide>
</package>
"""
    return package.encode("utf-8")


def zip_write_bytes(archive: zipfile.ZipFile, name: str, data: bytes, *, stored: bool = False) -> None:
    info = zipfile.ZipInfo(name)
    info.date_time = (2026, 9, 4, 0, 0, 0)
    info.compress_type = zipfile.ZIP_STORED if stored else zipfile.ZIP_DEFLATED
    info.external_attr = 0o100644 << 16
    archive.writestr(info, data)


def validate_epub(path: Path) -> dict[str, int]:
    errors: list[str] = []
    xml_parser = etree.XMLParser(resolve_entities=False, no_network=True, recover=False)
    with zipfile.ZipFile(path, "r") as archive:
        names = archive.namelist()
        name_set = set(names)
        if not names or names[0] != "mimetype":
            errors.append("mimetype is not the first ZIP entry")
        elif archive.getinfo("mimetype").compress_type != zipfile.ZIP_STORED:
            errors.append("mimetype is compressed")
        elif archive.read("mimetype") != b"application/epub+zip":
            errors.append("mimetype content is invalid")
        bad_member = archive.testzip()
        if bad_member:
            errors.append(f"CRC failure: {bad_member}")

        required = {"META-INF/container.xml", "EPUB/package.opf", "EPUB/nav.xhtml"}
        for name in sorted(required - name_set):
            errors.append(f"missing required file: {name}")

        parsed: dict[str, etree._Element] = {}
        for name in names:
            if name.endswith((".xml", ".opf", ".xhtml", ".ncx")):
                try:
                    parsed[name] = etree.fromstring(archive.read(name), parser=xml_parser)
                except etree.XMLSyntaxError as exc:
                    errors.append(f"invalid XML in {name}: {exc}")

        package = parsed.get("EPUB/package.opf")
        if package is not None:
            manifest: dict[str, str] = {}
            for item in package.xpath("//opf:manifest/opf:item", namespaces={"opf": OPF_NS}):
                item_id = item.get("id") or ""
                href = item.get("href") or ""
                manifest[item_id] = href
                target = posixpath.normpath(posixpath.join("EPUB", unquote(urlsplit(href).path)))
                if target not in name_set:
                    errors.append(f"manifest target missing: {target}")
            for itemref in package.xpath("//opf:spine/opf:itemref", namespaces={"opf": OPF_NS}):
                if (itemref.get("idref") or "") not in manifest:
                    errors.append(f"spine idref missing from manifest: {itemref.get('idref')}")

        id_cache: dict[str, set[str]] = {}
        for name, root in parsed.items():
            if not name.endswith(".xhtml"):
                continue
            if root.tag != f"{{{XHTML_NS}}}html":
                errors.append(f"wrong XHTML namespace: {name}")
            ids = [value for value in root.xpath("//@id") if value]
            if len(ids) != len(set(ids)):
                errors.append(f"duplicate id in {name}")
            id_cache[name] = set(ids)
            for node in root.xpath("//*[@href or @src]"):
                value = node.get("href") or node.get("src") or ""
                if not value or is_external_url(value):
                    continue
                split = urlsplit(value)
                if split.path:
                    target = posixpath.normpath(posixpath.join(posixpath.dirname(name), unquote(split.path)))
                else:
                    target = name
                if target not in name_set:
                    errors.append(f"broken resource link in {name}: {value}")
                    continue
                if split.fragment and target.endswith(".xhtml") and split.fragment not in id_cache.get(target, set()):
                    target_root = parsed.get(target)
                    target_ids = set(target_root.xpath("//@id")) if target_root is not None else set()
                    id_cache[target] = target_ids
                    if split.fragment not in target_ids:
                        errors.append(f"broken fragment link in {name}: {value}")

    if errors:
        raise RuntimeError("EPUB validation failed:\n- " + "\n- ".join(errors))
    return {
        "archive_entries": len(names),
        "xhtml_documents": sum(1 for name in names if name.endswith(".xhtml")),
        "embedded_images": sum(1 for name in names if name.startswith("EPUB/images/img-")),
        "bytes": path.stat().st_size,
    }


def build_book(book: BookSpec, repo_root: Path, output_dir: Path) -> dict[str, object]:
    warnings: list[str] = []
    chapters: list[Chapter] = []
    global_index = 0
    for volume_index, volume in enumerate(book.volumes, start=1):
        paths = discover_markdowns(repo_root, volume)
        for index_in_volume, source in enumerate(paths, start=1):
            global_index += 1
            href = f"text/v{volume_index:02d}-ch{index_in_volume:03d}.xhtml"
            chapters.append(
                Chapter(
                    source=source,
                    volume=volume,
                    volume_index=volume_index,
                    index_in_volume=index_in_volume,
                    global_index=global_index,
                    href=href,
                )
            )
    if not chapters:
        raise RuntimeError(f"No Markdown files discovered for {book.title}")

    chapter_lookup = {norm_path_key(chapter.source): chapter for chapter in chapters}
    assets = AssetRegistry()
    markdown = make_markdown_renderer()
    for chapter in chapters:
        render_chapter(chapter, repo_root, book.title, markdown, assets, chapter_lookup, warnings)

    parts: list[PartPage] = []
    if book.combined:
        for volume_index, volume in enumerate(book.volumes, start=1):
            part = PartPage(volume=volume, volume_index=volume_index, href=f"text/part-{volume_index:02d}.xhtml")
            build_part_page(
                part,
                [chapter for chapter in chapters if chapter.volume_index == volume_index],
                book.title,
            )
            parts.append(part)

    source_fingerprint = "\n".join(
        f"{chapter.source.relative_to(repo_root).as_posix()}:{hashlib.sha256(chapter.source.read_bytes()).hexdigest()}"
        for chapter in chapters
    )
    book_id = str(uuid.uuid5(uuid.NAMESPACE_URL, f"{book.title}\n{source_fingerprint}"))
    latest_mtime = max(chapter.source.stat().st_mtime for chapter in chapters)
    modified = datetime.fromtimestamp(latest_mtime, timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    files: dict[str, bytes] = {
        "META-INF/container.xml": CONTAINER_XML,
        "EPUB/styles/book.css": BOOK_CSS.encode("utf-8"),
        "EPUB/text/cover.xhtml": cover_xhtml(book),
        "EPUB/images/cover.png": make_cover_png(book, len(chapters)),
        "EPUB/nav.xhtml": make_nav(book, chapters, parts),
        "EPUB/toc.ncx": make_ncx(book, book_id, chapters, parts),
    }
    for part in parts:
        files[f"EPUB/{part.href}"] = part.xhtml
    for chapter in chapters:
        files[f"EPUB/{chapter.href}"] = chapter.xhtml
    for asset in assets.assets:
        files[f"EPUB/{asset.href}"] = asset.data
    files["EPUB/package.opf"] = make_package(book, book_id, modified, chapters, parts, assets)

    output_path = output_dir / book.filename
    temporary_path = output_path.with_suffix(output_path.suffix + ".tmp")
    with zipfile.ZipFile(temporary_path, "w") as archive:
        zip_write_bytes(archive, "mimetype", b"application/epub+zip", stored=True)
        for name in sorted(files):
            zip_write_bytes(archive, name, files[name])
    temporary_path.replace(output_path)
    validation = validate_epub(output_path)
    return {
        "title": book.title,
        "path": str(output_path),
        "chapters": len(chapters),
        "source_images": len(assets.assets),
        "warnings": warnings,
        "validation": validation,
    }


def parse_args(argv: Iterable[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--root", type=Path, default=Path(__file__).resolve().parents[1],
        help="Repository root containing the three chapter directories.",
    )
    parser.add_argument(
        "--output", type=Path, default=None,
        help="Output directory; when omitted, use ROOT/output/AppleBooks_EPUB.",
    )
    parser.add_argument(
        "--only", choices=("cpp", "graphics", "ue", "combined", "all"), default="all",
        help="Build one volume, the combined book, or all four books.",
    )
    return parser.parse_args(argv)


def main(argv: Iterable[str] | None = None) -> int:
    args = parse_args(argv)
    repo_root = args.root.resolve()
    output_dir = (args.output or (repo_root / "output" / "AppleBooks_EPUB")).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    selection = {
        "cpp": (BOOKS[0],),
        "graphics": (BOOKS[1],),
        "ue": (BOOKS[2],),
        "combined": (BOOKS[3],),
        "all": BOOKS,
    }[args.only]
    reports = [build_book(book, repo_root, output_dir) for book in selection]
    print(json.dumps({"output_dir": str(output_dir), "books": reports}, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise
