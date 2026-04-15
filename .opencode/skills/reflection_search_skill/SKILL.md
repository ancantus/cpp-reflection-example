---
name: reflection-search-skill
description: Find / search knowledge on C++26 static reflection
---

## Purpose
Use this skill when answering questions about C++26 reflection, annotations, or reflection-driven metaprogramming in this project.

Treat the files in `docs/` as the primary source of truth before using any repository search tool.

## Required Reference Order
1. Read `docs/Reflection for C++26.html`
2. Read `docs/Annotations for Reflection.html`
3. Read `docs/interesting_links.md`
4. If needed, consult `docs/define_static_{string,object,array}.html`
5. Only then search the codebase or other project files

## Core Terms
- `std::meta::info`
- `^^`
- `[: refl :]`
- `reflection operator`
- `splicer`
- `annotate`
- `meta.reflection.*`
- `P2996`
- `P3394`
- `define_static_{string,object,array}`

## Search Strategy
- Start with exact proposal terms before broad searches.
- Use the table of contents at the top of the .html pages to quickly find if the documentation contains relevent info.
- Prefer paper IDs, section names, and syntax tokens from the docs.
- If a search result conflicts with the docs, trust the newest relevant paper revision and mention that revision.
- Do not rely on general model memory when the docs already cover the topic.

## Answering Rules
- Cite the doc file used for each claim.
- Separate confirmed facts from inference.
- Assume 'unreleased wording' is the final wording of the standard. This project uses a fork of the gcc compiler built on these documents
- If the docs do not answer the question, say what is missing before broadening the search.

## Useful Anchors
- `Reflection for C++26.html` describes the reflection operator, `std::meta::info`, consteval metafunctions, and splicers.
- `Annotations for Reflection.html` describes annotations that reflection can observe.
- `define_static_{string,object,array}.html` is a follow-up paper split out of the reflection work and may be relevant for static-storage wording.
- `interesting_links.md` points to related project examples and external references.

