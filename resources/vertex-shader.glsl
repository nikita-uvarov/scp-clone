varying vec2 texture_coordinate;
varying vec3 projectedPos;

void main()
{
    // Transforming The Vertex
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    projectedPos = gl_Position.xyz;
    
    // Passing The Texture Coordinate Of Texture Unit 0 To The Fragment Shader
    texture_coordinate = vec2(gl_MultiTexCoord0);
}