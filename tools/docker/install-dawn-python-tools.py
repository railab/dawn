#!/usr/bin/env python3
"""Install local Dawn Python tooling for Docker QA."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
import tomllib
from pathlib import Path


REQ_NAME_RE = re.compile(r"\s*([A-Za-z0-9_.-]+)")


def normalize_name(name: str) -> str:
    return name.replace("_", "-").lower()


def requirement_name(line: str) -> str | None:
    line = line.split("#", 1)[0].strip()
    if not line or line.startswith(("-", "http:", "https:", ".")):
        return None
    match = REQ_NAME_RE.match(line)
    if not match:
        return None
    return normalize_name(match.group(1))


def local_projects(project_root: Path) -> dict[str, Path]:
    projects: dict[str, Path] = {}
    for pyproject in sorted((project_root / "tools").glob("*/pyproject.toml")):
        data = tomllib.loads(pyproject.read_text(encoding="utf-8"))
        name = data.get("project", {}).get("name")
        if isinstance(name, str):
            projects[normalize_name(name)] = pyproject.parent
    return projects


def local_dependencies(project_path: Path) -> list[str]:
    pyproject = project_path / "pyproject.toml"
    data = tomllib.loads(pyproject.read_text(encoding="utf-8"))
    deps = data.get("project", {}).get("dependencies", [])
    names: list[str] = []
    for dep in deps:
        if isinstance(dep, str):
            name = requirement_name(dep)
            if name:
                names.append(name)
    return names


def requirement_roots(path: Path, projects: dict[str, Path]) -> list[str]:
    if not path.exists():
        return []
    roots: list[str] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        name = requirement_name(line)
        if name in projects:
            roots.append(name)
    return roots


def resolve_local_install_order(
    roots: list[str], projects: dict[str, Path]
) -> list[Path]:
    seen: set[str] = set()
    ordered: list[Path] = []

    def visit(name: str) -> None:
        if name in seen or name not in projects:
            return
        seen.add(name)
        for dep in local_dependencies(projects[name]):
            visit(dep)
        ordered.append(projects[name])

    for root in roots:
        visit(root)
    return ordered


def filtered_requirements(
    source: Path, local_names: set[str]
) -> tempfile.NamedTemporaryFile[str] | None:
    if not source.exists():
        return None

    tmp = tempfile.NamedTemporaryFile(
        "w", encoding="utf-8", prefix="dawn-req-", suffix=".txt", delete=False
    )
    with tmp:
        for line in source.read_text(encoding="utf-8").splitlines():
            name = requirement_name(line)
            if name and name in local_names:
                continue
            print(line, file=tmp)
    return tmp


def run(cmd: list[str]) -> None:
    subprocess.run(cmd, check=True)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--project-root", default=".", help="Dawn repository root"
    )
    parser.add_argument(
        "--requirements",
        default="ntfc/requirements.txt",
        help="NTFC requirements file",
    )
    args = parser.parse_args()

    project_root = Path(args.project_root).resolve()
    requirements = project_root / args.requirements
    projects = local_projects(project_root)
    roots = ["dawnpy", "dawnpy-tests"]
    roots.extend(requirement_roots(requirements, projects))

    editables = resolve_local_install_order(roots, projects)
    if editables:
        cmd = [sys.executable, "-m", "pip", "install"]
        for path in editables:
            cmd.extend(["-e", str(path)])
        run(cmd)

    filtered = filtered_requirements(requirements, set(projects))
    if filtered is not None:
        run([sys.executable, "-m", "pip", "install", "-r", filtered.name])


if __name__ == "__main__":
    main()
