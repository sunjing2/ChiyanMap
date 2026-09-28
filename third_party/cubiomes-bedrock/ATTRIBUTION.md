# cubiomes-bedrock Attribution

This directory vendors the Bedrock biome generator sources from the `cubiomes`
git submodule of
`https://github.com/FragrantResult186/cubiomes-viewer-bedrock`, specifically
`https://github.com/FragrantResult186/cubiomes-bedrock` at
`080d1f3332f5ff027a360fc781160c891865b92b`.

The viewer repository itself is GPL-3.0-only, but this separately licensed
submodule is MIT (Copyright (c) 2020 Cubitect and contributors); its MIT text
is retained in `LICENSE`. ChiyanMap uses this snapshot through
`src/worldgen/BedrockBiomeValidator.cpp` for deterministic, seed-only Bedrock
world-generation queries.
