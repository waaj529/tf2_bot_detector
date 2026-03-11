# TF2 Bot Detector Fedora/Nobara Fork Notes

This fork exists because the upstream repository, PazerOP/tf2_bot_detector, was archived and no longer accepts maintenance changes.

The goal of this fork is to preserve the original project while restoring practical compatibility for Fedora, Nobara, and modern Linux users running Team Fortress 2 through Steam/Proton.

Credits:
- Original project concept and implementation: PazerOP and upstream contributors
- This fork keeps upstream attribution intact and only adds compatibility, maintenance, and release work needed for current Linux environments

Initial fork policy:
- Keep changes minimal and focused on Linux compatibility first
- Preserve Windows behavior unless a change is required to isolate platform code cleanly
- Defer updater, WinRT, Discord Rich Presence, and packaging until the Linux gameplay loop is stable