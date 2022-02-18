for /r %%f in (.\source\*.vert) do (
    glslc.exe source\%%~nf.vert -o compiled\%%~nf.vert.spv
)
for /r %%f in (.\source\*.frag) do (
    glslc.exe source\%%~nf.frag -o compiled\%%~nf.frag.spv
)
for /r %%f in (.\source\*.comp) do (
    glslc.exe source\%%~nf.comp -o compiled\%%~nf.comp.spv
)