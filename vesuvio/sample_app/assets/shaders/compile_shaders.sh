mkdir -p compiled
shopt -s nullglob
for f in source/*.vert
do
    glslc "$f" -o "compiled/$(basename $f ).spv"
done
for f in source/*.frag
do
    glslc "$f" -o "compiled/$(basename $f ).spv"
done
for f in source/*.comp
do
    glslc "$f" -o "compiled/$(basename $f ).spv"
done