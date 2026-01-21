#version 330 core

uniform sampler2D ssaoTexture;

in vec2 vertTex;

out float out_col;

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoTexture, 0));
    float res = 0.0;
    for (int x = -2; x < 2; ++x)
    {
        for (int y = -2; y < 2; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            res += texture(ssaoTexture, vertTex + offset).r;
        }
    }
    out_col = res / (4.0 * 4.0);
}