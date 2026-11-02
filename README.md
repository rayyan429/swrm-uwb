# SWRM UWB Localization

UWB indoor positioning for the SWRM bots (ESP32 + Ai-Thinker BU03, DW3000).

**Hardware note:** the firmware in `uwb_anchor/` and `uwb_tag/` still uses the DW1000 library and needs porting to the DW3000 before it will run on the BU03. See milestone 0 in [docs/ROADMAP.md](docs/ROADMAP.md).

Based on jremington/UWB-Indoor-Localization_Arduino (GPL-3.0).
