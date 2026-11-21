const { test } = require('node:test');
const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const context = vm.createContext({});
vm.runInContext(readFileSync(path.join(__dirname, '../common/legacy_gl.js'), 'utf8'), context);
const repair = context.PangeaGL.repairTexEnvShader;

test('repairs combined texture loads with texture matrices (Nanosaur 2 first level)', () => {
  const bad = `vec4 tej_env1_texload0 = texture2D(u_texUnit1, (u_textureMatrix1 * v_texCoord1);
vec3 color = tej_env1_texload0.xy).rgb + previous.rgb;
float alpha = tej_env1_texload0.xy).a * previous.a;`;
  const fixed = repair(bad);
  assert.match(fixed, /texture2D\(u_texUnit1, \(u_textureMatrix1 \* v_texCoord1\)\.xy\);/);
  assert.match(fixed, /tej_env1_texload0\.rgb \+ previous\.rgb/);
  assert.match(fixed, /tej_env1_texload0\.a \* previous\.a/);
  assert.equal(repair(fixed), fixed);
});

test('leaves already-correct shaders and ordinary texture loads unchanged', () => {
  for (const source of [
    'vec4 c = texture2D(u_texUnit0, v_texCoord0.xy);',
    'vec4 c = texture2D(u_texUnit1, (u_textureMatrix1 * v_texCoord1).xy);',
    'gl_FragColor = v_color;'
  ]) assert.equal(repair(source), source);
});
