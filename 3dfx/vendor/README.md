# Vendored dependencies

Shared files for the demos in `3dfx/`. Keep original vendor files intact.

- `glide/`: complete extracted Glide 2.43 SDK (254 files), including headers,
  DOS and Windows libraries, drivers, utilities, samples and documentation.
  Source: https://3dfxarchive.com/downloads/glide_sdk-243.zip
  Extracted from `gwebvgr.exe`, `gwebvgr2.exe`, and `gwebvgr3.exe` with 7-Zip.
- `runtime/GLIDE2X.OVL`: Voodoo 1 DOS runtime used by our DOSBox-X demo.
  Source: https://github.com/efliks/nostalgic-glide-demo/blob/master/glide2x.ovl
- `runtime/DOS4GW.EXE`: DOS extender distributed with the Open Watcom
  Current-build toolchain downloaded on 2026-11-03.
  Source: https://github.com/open-watcom/open-watcom-v2/releases/tag/Current-build

Original copyright notices and terms in the SDK remain intact. These files
are included in Git. Open Watcom itself is downloaded separately by the
cube setup script. Runtime copies in individual demo folders are generated
from `runtime/` by their build scripts.
