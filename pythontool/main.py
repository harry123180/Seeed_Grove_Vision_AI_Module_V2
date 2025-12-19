#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Grove Vision AI V2 Tool
主入口點
"""

import sys
import os
import traceback

# 將 src 加入路徑
sys.path.insert(0, os.path.dirname(__file__))

from src.main_window import GroveVisionAITool


def main():
    try:
        app = GroveVisionAITool()
        app.protocol("WM_DELETE_WINDOW", app.on_closing)
        app.mainloop()
    except Exception as e:
        print("=" * 50)
        print("[FATAL ERROR] Application crashed!")
        print("=" * 50)
        print(f"Error: {e}")
        traceback.print_exc()
        print("=" * 50)
        input("Press Enter to exit...")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nUser interrupted")
    except Exception as e:
        print(f"[Startup Error] {e}")
        traceback.print_exc()
        input("Press Enter to exit...")
