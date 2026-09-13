"""Run the compiled replay utility on synthetic input and verify artifacts."""
import csv,subprocess,sys,tempfile
from pathlib import Path
import xml.etree.ElementTree as ET
with tempfile.TemporaryDirectory(prefix="pen-replay-test-") as folder:
    dest=Path(folder)/"new-output"
    subprocess.run([sys.argv[1],"--self-test",str(dest)],check=True)
    ET.parse(dest/"comparison.svg")
    with (dest/"vertices.csv").open() as f: rows=list(csv.DictReader(f))
    groups={}
    for row in rows:
        key=(row["correction"],row["pressure"],row["rendering"])
        groups.setdefault(key,[]).append(row)
    assert len(groups)==16
    for key,points in groups.items():
        assert len(points)==(120 if key[2]=="polyline" else 953)
        assert abs(float(points[-1]["x_dip"])-119)<1e-8
    assert "Not full upstream compatibility replay" in (dest/"README.txt").read_text()
    # Existing output must not be overwritten.
    before=(dest/"vertices.csv").read_bytes()
    assert subprocess.run([sys.argv[1],"--self-test",str(dest)]).returncode!=0
    assert (dest/"vertices.csv").read_bytes()==before
print("Replay artifacts and overwrite protection passed")
