from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from endgame.panel_assets import install
install(sys.argv[1])
