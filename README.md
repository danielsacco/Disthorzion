# Disthorzion
Tube emulator developed using [iPlug2](https://github.com/iPlug2/iPlug2) currently in Alpha stage. I'm a newby at DSP, this project is meant just for learning.  
Based on iPlug2's IPlugEffect example.  
Only tested as VST3 within Reaper on Windows, I have no money for a Mac neither money for an Apple developer license.  
To build this project follow the setup instructions for iPlug2, then download this code and use it under iPlug2/Examples or any other folder at the same level.
I use a folder iPlug2/Projects.

# TODO
- Gain and Drive should be set in dBs
- Create Presets
- Improve GUI
- Highlight 1 or 2 valves selection
- Show DC Block Frequency control for fine tuning
- Remove SnapToMouse, it is fixed in ISliderControlBase::OnMouseDown (Override component)
- Add Help / About