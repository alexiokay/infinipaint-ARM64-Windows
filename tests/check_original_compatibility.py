"""Guard original geometry source, not a claim of full pixel/input parity."""
from pathlib import Path
import json,re,unittest
ROOT=Path(__file__).resolve().parents[1]
def body(text,name):
    text=text[re.search(r"(?:std::vector<BrushPoint>|void) "+name+r"\(",text).start():]
    start=text.index("{")+1
    n=1
    for i in range(start,len(text)):
        n+=(text[i]=="{")-(text[i]=="}")
        if not n:return text[start:i].strip()
    raise AssertionError("unclosed function")
def normalized(text):
    return re.sub(r"\s+","",text)
class OriginalCompatibility(unittest.TestCase):
    def test_original_geometry(self):
        code=(ROOT/"src/CanvasComponents/BrushComponentCode.cpp").read_text(encoding="utf-8")
        reference=json.loads((ROOT/"tests/upstream_geometry_reference.json").read_text(encoding="utf-8"))
        for name,expected in reference["functions"].items():
            actual=body(code,name)
            if name=="mouse_motion":
                actual=actual[actual.index("BrushComponentCode::BrushPoint p;"):]
            self.assertEqual(normalized(actual),normalized(expected),name)
    def test_explicit_capture(self):
        code=(ROOT/"src/DrawingProgram/Tools/BrushTool.cpp").read_text(encoding="utf-8")
        self.assertIn("toolConfig.brush.samplePath(),",code)
        self.assertIn("genData.penPath, genData.boundedCurves, genData.penDisplayScale",code)
        config=(ROOT/"src/BrushPressureConfig.hpp").read_text(encoding="utf-8")
        self.assertIn("return engine == Engine::Samples;",config)
if __name__=="__main__":unittest.main()
