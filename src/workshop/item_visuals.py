"""Resolve Workshop item codes to existing Reimagined HD inventory and ground assets."""
import json
from pathlib import Path


def add_mappings(path, mappings):
    raw = path.read_bytes()
    entries = json.loads(raw.decode("utf-8-sig"))
    existing = {key: value for entry in entries for key, value in entry.items()}
    additions = []
    for key, value in mappings.items():
        if key in existing:
            if existing[key] != value:
                raise ValueError(f"Conflicting asset for {key} in {path}")
        else:
            additions.append({key: value})
    if additions:
        prefix = raw.rstrip()
        assert prefix.endswith(b"]")
        path.write_bytes(prefix[:-1].rstrip() + b",\n" + json.dumps(additions, indent=2)[1:].encode() + b"\n")
    result = {k: v for e in json.loads(path.read_text(encoding="utf-8-sig")) for k, v in e.items()}
    assert all(result[k] == v for k, v in mappings.items())


def install(root, masks):
    hd = Path(root) / "data/hd/items"
    relic = "uniquecharm/relic_of_the_ancients"
    entries = {f"W{mask:03X}": {"asset": "quest/bark_scroll"} for mask in masks}
    assert len(entries) == 275
    entries["Wpln"] = {"asset":"quest/bark_scroll"}
    entries["Wsig"] = {"asset": relic}
    seals=json.loads((Path(root)/"workshop/FORGING-seals.json").read_text(encoding="utf8"))
    entries.update({key:{"asset":"quest/bark_scroll"} for key in seals})
    add_mappings(hd / "items.json", entries)
    add_mappings(hd / "uniques.json", {"workshopastralsigil": dict.fromkeys(("normal", "uber", "ultra"), relic)})
