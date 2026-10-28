#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D src;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float texelW;
    float texelH;
    float effW;
    float effH;
    float rad;
} ubuf;

void main()
{
    vec2 uv = qt_TexCoord0;
    float tw = ubuf.texelW;
    float th = ubuf.texelH;
    vec4 sum = texture(src, uv) * 0.3125;
    sum += texture(src, uv + vec2(tw, 0.0)) * 0.2344;
    sum += texture(src, uv - vec2(tw, 0.0)) * 0.2344;
    sum += texture(src, uv + vec2(2.0 * tw, 0.0)) * 0.0938;
    sum += texture(src, uv - vec2(2.0 * tw, 0.0)) * 0.0938;
    sum += texture(src, uv + vec2(3.0 * tw, 0.0)) * 0.0156;
    sum += texture(src, uv - vec2(3.0 * tw, 0.0)) * 0.0156;
    sum += texture(src, uv + vec2(0.0, th)) * 0.2344;
    sum += texture(src, uv - vec2(0.0, th)) * 0.2344;
    sum += texture(src, uv + vec2(0.0, 2.0 * th)) * 0.0938;
    sum += texture(src, uv - vec2(0.0, 2.0 * th)) * 0.0938;
    sum += texture(src, uv + vec2(0.0, 3.0 * th)) * 0.0156;
    sum += texture(src, uv - vec2(0.0, 3.0 * th)) * 0.0156;
    float alpha = 1.0;
    if (ubuf.rad > 0.0) {
        vec2 half2v = vec2(ubuf.effW, ubuf.effH) * 0.5;
        vec2 p = qt_TexCoord0 * vec2(ubuf.effW, ubuf.effH);
        vec2 d = abs(p - half2v) - (half2v - vec2(ubuf.rad));
        float dist = length(max(d, vec2(0.0))) - ubuf.rad;
        alpha = 1.0 - smoothstep(-1.0, 1.0, dist);
    }
    fragColor = vec4(sum.rgb * alpha, sum.a * alpha);
}
