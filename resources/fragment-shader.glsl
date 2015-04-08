varying vec2 texture_coordinate;
uniform sampler2D mytexture;
varying vec3 projectedPos;

void main(void)
{
    gl_FragColor = texture2D(mytexture, texture_coordinate);
#ifdef FOG
    gl_FragColor *= (5.0 - projectedPos.z) / 5.0;
#endif
    //gl_FragColor.x = gl_FragColor.x * gl_FragColor.y;
    //gl_FragColor.x = gl_FragColor.y = gl_FragColor.z = 1.0 - projectedPos.z / 10.0;
}
