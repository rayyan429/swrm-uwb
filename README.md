# SWRM UWB Localization

UWB indoor positioning for the SWRM bots (ESP32 + Ai-Thinker BU03, DW3000).

The DW3000 driver (Qorvo's API, Arduino port by [Makerfabs](https://github.com/Makerfabs/Makerfabs-ESP32-UWB-DW3000)) is downloaded by PlatformIO at build time rather than kept in this repo, because its files carry Decawave/Qorvo copyright with no open-source licence. Ranging is in `lib/uwb_ranging`. See [docs/ROADMAP.md](docs/ROADMAP.md) for progress.

Based on jremington/UWB-Indoor-Localization_Arduino (GPL-3.0).
