#!/usr/bin/env python3
"""Dependency-free static checks for the public portfolio."""

from __future__ import annotations

import ast
import re
import sys
from pathlib import Path
from xml.etree import ElementTree


ROOT = Path(__file__).resolve().parents[1]
TEXT_SUFFIXES = {".md", ".py", ".cpp", ".hpp", ".h", ".yaml", ".yml", ".xml", ".txt", ".lua"}
EXCLUDED_PARTS = {".git", "build", "install", "log", "__pycache__"}
FORBIDDEN = {
    "private-key marker": re.compile(r"BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY"),
    "Linux user path": re.compile(r"/home/[A-Za-z0-9._-]+/"),
    "Windows user path": re.compile(r"[A-Za-z]:\\\\Users\\\\[^\\\\]+\\\\"),
}


def text_files() -> list[Path]:
    return [
        path
        for path in ROOT.rglob("*")
        if path.is_file()
        and path.suffix.lower() in TEXT_SUFFIXES
        and not any(part in EXCLUDED_PARTS for part in path.parts)
    ]


def check_python(errors: list[str]) -> None:
    for path in ROOT.glob("src/*/launch/*.py"):
        try:
            ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
        except Exception as exc:  # pragma: no cover - diagnostic path
            errors.append(f"Python syntax: {path.relative_to(ROOT)}: {exc}")


def check_xml(errors: list[str]) -> None:
    for path in ROOT.glob("src/*/package.xml"):
        try:
            ElementTree.parse(path)
        except Exception as exc:  # pragma: no cover - diagnostic path
            errors.append(f"XML: {path.relative_to(ROOT)}: {exc}")


def check_markdown_links(errors: list[str]) -> None:
    link_pattern = re.compile(r"\[[^\]]+\]\(([^)]+)\)")
    for path in [ROOT / "README.md", *ROOT.glob("docs/*.md"), *ROOT.glob("src/*/README.md")]:
        text = path.read_text(encoding="utf-8")
        for target in link_pattern.findall(text):
            if target.startswith(("http://", "https://", "mailto:", "#")):
                continue
            clean_target = target.split("#", 1)[0]
            if clean_target and not (path.parent / clean_target).resolve().exists():
                errors.append(f"Broken link: {path.relative_to(ROOT)} -> {target}")


def check_sensitive_paths(errors: list[str]) -> None:
    for path in text_files():
        text = path.read_text(encoding="utf-8", errors="replace")
        for label, pattern in FORBIDDEN.items():
            if pattern.search(text):
                errors.append(f"{label}: {path.relative_to(ROOT)}")


def main() -> int:
    errors: list[str] = []
    check_python(errors)
    check_xml(errors)
    check_markdown_links(errors)
    check_sensitive_paths(errors)
    if errors:
        print("Portfolio validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print("Portfolio validation passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
