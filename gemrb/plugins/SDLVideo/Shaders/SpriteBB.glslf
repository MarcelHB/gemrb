precision highp float;
varying vec2 v_texCoord;
uniform sampler2D s_texture;
const vec3 transparencyColor = vec3(-1.0, 0.0, -1.0);

/* An if-less version of: Set alpha to 0 if the pixel is pink. */

void main() {
  vec4 color = texture2D(s_texture, v_texCoord);
  vec3 sum = transparencyColor + color.rgb;
  float dotProduct = dot(sum, sum);
  gl_FragColor = vec4(color.rgb, sign(dotProduct));
}
