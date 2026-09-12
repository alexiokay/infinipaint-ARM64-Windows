"""Source wiring checks, supplemented by executable brush_pressure C++ tests."""
from pathlib import Path
import re
import unittest
ROOT = Path(__file__).resolve().parents[1]
def source(path):
    return (ROOT / path).read_text(encoding="utf-8")
class PressureSettingWiring(unittest.TestCase):
    def test_configuration_and_modes(self):
        config = source("src/BrushPressureConfig.hpp")
        self.assertIn("pressureResponse = Response::Original", config)
        self.assertIn('j.contains("preservePenPressure")', config)
        self.assertIn("using BrushToolConfig = BrushPressure::Config", source("src/DrawingProgram/ToolConfiguration.hpp"))
    def test_both_brush_panels_and_contact_down(self):
        brush = source("src/DrawingProgram/Tools/BrushTool.cpp")
        for panel in ("gui_toolbox", "gui_phone_toolbox"):
            body = brush.split(f"void BrushTool::{panel}(", 1)[1].split("\nvoid ", 1)[0]
            self.assertIn("gui_inspector();", body)
        for label in ("Original smoothing (default)", "Preserve samples", "Uniform peak width", "Width propagation"):
            self.assertIn(f'"{label}"', brush)
        self.assertNotIn('"Preserve per-point pen pressure"', brush)
        self.assertIn("toolConfig.brush.samplePath(), toolConfig.brush.pressureResponse == BrushPressure::Response::Peak", brush)
        motion = brush.split("void BrushTool::input_mouse_motion_callback", 1)[1].split("\nvoid ", 1)[0]
        self.assertNotIn("pressureResponse", motion)
        self.assertIn("motion.penContact && motion.penId == genData.penId", motion)
        self.assertIn("if (!genData.penPath) BrushComponentCode::fix_tip", brush)
        for file in ("src/Toolbar.cpp", "src/Screens/FileSelectScreen.cpp"):
            self.assertNotIn('"Brush pressure smoothing factor"', source(file))
    def test_eraser_and_original_path(self):
        self.assertIn("bool useDirectPenPath = false", source("src/CanvasComponents/BrushComponentCode.hpp"))
        self.assertRegex(source("src/DrawingProgram/Tools/EraserTool.cpp"), r"mouse_button\([^;]*, false\);")
        core = source("src/CanvasComponents/BrushComponentCode.cpp")
        self.assertIn("genData.penPath = useDirectPenPath && button.deviceType == InputManager::MouseDeviceType::PEN;", core)
        self.assertIn("const size_t changed = peakGrew ? 0 : genData.stabilizer.changedBegin();", core)
        self.assertIn("genData.sampleWidths.output(samples[i].width)", core)
        self.assertIn("smooth_out_points(genData.brushPoints, drawP.world.main.conf.tabletOptions.brushPressureSmoothingFactor);", core)
if __name__ == "__main__":
    unittest.main()
