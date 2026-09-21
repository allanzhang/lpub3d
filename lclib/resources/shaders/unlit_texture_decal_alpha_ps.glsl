LC_PIXEL_INPUT vec2 PixelTexCoord;
LC_PIXEL_OUTPUT

uniform mediump vec4 MaterialColor;
uniform sampler2D Texture;

void main()
{
	LC_SHADER_PRECISION vec4 TexelColor = texture2D(Texture, PixelTexCoord);
	LC_SHADER_PRECISION vec3 BaseColor = MaterialColor.rgb;
	LC_SHADER_PRECISION vec3 Rgb = mix(BaseColor, TexelColor.rgb, TexelColor.a);
	LC_SHADER_PRECISION float Alpha = max(MaterialColor.a, TexelColor.a);
	if (Alpha < 0.01)
		discard;

	gl_FragColor = vec4(Rgb, Alpha);
}
