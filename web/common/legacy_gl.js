/* Compatibility fixes for the pinned Emscripten 4.0.20 legacy GL generator. */
'use strict';
globalThis.PangeaGL = {
  repairTexEnvShader(source) {
    // TexEnvJIT's duplicate-load regex stops at the texture matrix's closing
    // parenthesis, producing an incomplete texture2D call and dangling .xy).
    // Repair only that exact generated form; keep all combiners and transforms.
    const broken = /vec4 (tej_env\d+_texload\d+) = texture2D\((u_texUnit\d+), \((u_textureMatrix\d+ \* v_texCoord\d+)\);/g;
    const loads = [...source.matchAll(broken)];
    for (const [statement, name, sampler, coordinates] of loads) {
      source = source.replace(statement, `vec4 ${name} = texture2D(${sampler}, (${coordinates}).xy);`);
      source = source.replaceAll(`${name}.xy)`, name);
    }
    return source;
  }
};
