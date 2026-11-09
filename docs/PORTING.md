# Rayman 2 -- porting notes

Toolchain and runtime facts of this port, symptom first.

| Symptom | Fact |
|---|---|
| `dc.py doctor` on a fresh machine | needed `winget install KhronosGroup.VulkanSDK` and `python dreamcomp/tools/dc.py deps` (SDL3 into `dreamcomp/.deps/`) |
| `fatal error RC1107: invalid usage` building `ftver.rc` (FreeType) | the Windows SDK `rc.exe` under a French Visual Studio 2026; first configure by hand with `-DCMAKE_RC_COMPILER=llvm-rc` (plus the flags `dc.py build` uses), then `dc.py build .` reuses the cache |
| Port is not under `dreamcomp/ports/` | dreamcomp is this repo's `dreamcomp/` submodule; run its tools as `python dreamcomp/tools/dc.py <cmd> .` |
