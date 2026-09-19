#!/usr/bin/env python3
"""Turn sim/page.html into a C++ header holding it as a raw string literal.

The generated header is committed, so the build works without Python. Run
this after editing page.html:   python3 tools/embed_page.py
"""
import pathlib

src = pathlib.Path(__file__).resolve().parent.parent / "sim" / "page.html"
dst = pathlib.Path(__file__).resolve().parent.parent / "sim" / "page_html.h"
html = src.read_text()
assert ')HTML"' not in html, "page.html contains the raw-string terminator"
dst.write_text(
    "#pragma once\n"
    "// GENERATED from sim/page.html by tools/embed_page.py - do not edit.\n"
    "namespace sim {\n"
    'static const char* kPageHtml = R"HTML(' + html + ')HTML";\n'
    "} // namespace sim\n"
)
print("wrote", dst.relative_to(pathlib.Path.cwd()) if dst.is_relative_to(pathlib.Path.cwd()) else dst)
