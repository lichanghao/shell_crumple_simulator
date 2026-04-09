#!/usr/bin/env python3

import sys
import xml.etree.ElementTree as ET


def local_name(tag: str) -> str:
    if "}" in tag:
        return tag.split("}", 1)[1]
    return tag


def child_by_name(node: ET.Element, name: str) -> ET.Element | None:
    for child in node:
        if local_name(child.tag) == name:
            return child
    return None


def fail(path: str, message: str) -> int:
    print(f"{path}: {message}", file=sys.stderr)
    return 1


def validate_vtu(path: str, root: ET.Element) -> int:
    if local_name(root.tag) != "VTKFile":
        return fail(path, "root tag must be VTKFile")
    if root.attrib.get("type") != "UnstructuredGrid":
        return fail(path, "VTKFile type must be UnstructuredGrid")

    unstructured_grid = child_by_name(root, "UnstructuredGrid")
    if unstructured_grid is None:
        return fail(path, "missing UnstructuredGrid node")

    piece = child_by_name(unstructured_grid, "Piece")
    if piece is None:
        return fail(path, "missing Piece node")

    if child_by_name(piece, "Points") is None:
        return fail(path, "missing Points node")
    cells = child_by_name(piece, "Cells")
    if cells is None:
        return fail(path, "missing Cells node")

    required_cell_arrays = {"connectivity", "offsets", "types"}
    found_cell_arrays = set()
    for child in cells:
        if local_name(child.tag) == "DataArray":
            name = child.attrib.get("Name")
            if name:
                found_cell_arrays.add(name)
    missing = required_cell_arrays - found_cell_arrays
    if missing:
        return fail(path, f"missing Cells DataArray(s): {', '.join(sorted(missing))}")

    return 0


def validate_pvd(path: str, root: ET.Element) -> int:
    if local_name(root.tag) != "VTKFile":
        return fail(path, "root tag must be VTKFile")
    if root.attrib.get("type") != "Collection":
        return fail(path, "VTKFile type must be Collection")

    collection = child_by_name(root, "Collection")
    if collection is None:
        return fail(path, "missing Collection node")

    datasets = [child for child in collection if local_name(child.tag) == "DataSet"]
    if not datasets:
        return fail(path, "missing DataSet rows")

    for dataset in datasets:
        if "timestep" not in dataset.attrib:
            return fail(path, "DataSet is missing timestep attribute")
        if "file" not in dataset.attrib:
            return fail(path, "DataSet is missing file attribute")

    return 0


def validate_file(path: str) -> int:
    try:
        root = ET.parse(path).getroot()
    except ET.ParseError as exc:
        return fail(path, f"XML parse error: {exc}")
    except OSError as exc:
        return fail(path, f"cannot read file: {exc}")

    if path.endswith(".vtu"):
        return validate_vtu(path, root)
    if path.endswith(".pvd"):
        return validate_pvd(path, root)
    return fail(path, "unsupported file type (expected .vtu or .pvd)")


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print("usage: validate_vtk_xml.py <file> [<file> ...]", file=sys.stderr)
        return 2

    for path in argv[1:]:
        rc = validate_file(path)
        if rc != 0:
            return rc
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
