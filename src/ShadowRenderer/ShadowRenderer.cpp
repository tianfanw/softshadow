/*
 * ShadowRenderer.cpp
 *
 *  Created on: 2013-08-01
 *      Author: tiawang
 */

#include "ShadowRenderer.h"
#include "gaussianblur.h"
#include <algorithm>

static int createShader(GLuint *p, const char *vSource, const char *fSource) {
	GLint status;

	std::cout << "Creating shader" << std::endl;
	fflush(stdout);
	GLuint vs, fs, program;
    // Compile the vertex shader
    vs = glCreateShader(GL_VERTEX_SHADER);
    if (!vs) {
        fprintf(stdout, "Failed to create vertex shader: %d\n", glGetError());
        goto fail;
    } else {
        glShaderSource(vs, 1, &vSource, 0);
        glCompileShader(vs);
        glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
        if (GL_FALSE == status) {
            GLchar log[256];
            glGetShaderInfoLog(vs, 256, NULL, log);

            fprintf(stdout, "Failed to compile vertex shader: %s\n", log);
            fflush(stdout);
            glDeleteShader(vs);

            goto fail;
        }
    }

    // Compile the fragment shader
    fs = glCreateShader(GL_FRAGMENT_SHADER);
    if (!fs) {
        fprintf(stdout, "Failed to create fragment shader: %d\n", glGetError());
        goto fail;
    } else {
        glShaderSource(fs, 1, &fSource, 0);
        glCompileShader(fs);
        glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
        if (GL_FALSE == status) {
            GLchar log[256];
            glGetShaderInfoLog(fs, 256, NULL, log);

            fprintf(stdout, "Failed to compile fragment shader: %s\n", log);
            fflush(stdout);
            glDeleteShader(vs);
            glDeleteShader(fs);

            goto fail;
        }
    }

    // Create and link the program
    program = glCreateProgram();
    if (program)
    {
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == GL_FALSE)    {
            GLchar log[256];
            glGetProgramInfoLog(fs, 256, NULL, log);

            fprintf(stdout, "Failed to link shader program: %s\n", log);
            fflush(stdout);
            glDeleteProgram(program);
            program = 0;

            goto fail;
        }
    } else {
        fprintf(stdout, "Failed to create a shader program\n");

        glDeleteShader(vs);
        glDeleteShader(fs);
        goto fail;
    }
    *p = program;

    // We don't need the shaders anymore - the program is enough
	glDeleteShader(fs);
	glDeleteShader(vs);
	std::cout << "Created shader" << std::endl;
	fflush(stdout);
    return 0;
fail:
	for (int i=0; i<1000; i++)
	std::cout << "FAILED TO CREATE SHADER" << std::endl;
	fflush(stdout);
    return EXIT_FAILURE;
}

static bool s_shaderCompiled = false;
static GLuint s_defaultHardShadowShader, s_hardShadowShader;
static GLuint s_depthShader, s_depthShader2, s_shadowShader;
static GLuint s_defaultSceneShader, s_sceneShader, s_copyShader;
static GLuint gaussNTapProgram, boxBlurProgram;
static GLuint positionLoc, alphaLoc, colorLoc;
static int gaussTaps = 9;
static int widhght = 256;
static GLfloat sampleHardShadowBound = 2.0 / 3.0;

int ShadowRenderer::genShaders() {

	if(s_shaderCompiled) return 0;

	{
		// Create shaders
		const char* vSource =
				"precision highp float;"
				"attribute vec4 a_position;"
				"attribute vec4 a_color;"
				"attribute vec2 a_alphacoord;"
				"varying vec4 v_color;"
				"varying float v_shadow;"
				"varying vec2 v_alphacoord;"
				"void main()"
				"{"
				"    gl_Position = vec4(a_position.xy,-a_position.z,1.0);"
				"    v_color = a_color;"
				"    v_shadow = a_position.w * 0.25;"
				"    v_alphacoord = a_alphacoord;"
				"}";

		const char* fSource =
				"precision highp float;"
				"uniform sampler2D u_tex;"
				"varying vec2 v_alphacoord;"
				"varying lowp vec4 v_color;"
				"varying float v_shadow;"
				"void main()"
				"{"
				"    gl_FragColor = vec4(v_shadow * texture2D(u_tex, v_alphacoord).a, 0.0, 0.0, 0.0);"
//				"    gl_FragColor = vec4(texture2D(u_tex, v_alphacoord).a, 0.0, 0.0, 0.0);"
				"}";

		if (EXIT_FAILURE == createShader(&s_hardShadowShader, vSource, fSource))
			return EXIT_FAILURE;

		glUseProgram(s_hardShadowShader);
		positionLoc = glGetAttribLocation(s_hardShadowShader, "a_position");
		colorLoc = glGetAttribLocation(s_hardShadowShader, "a_color");
		alphaLoc = glGetAttribLocation(s_hardShadowShader, "a_alphacoord");
		glUniform1i(glGetUniformLocation(s_hardShadowShader, "u_tex"), 0);
	}

	{
		// Create shaders
		const char* vSource =
				"precision highp float;"
				"attribute vec4 a_position;"
				"attribute vec4 a_color;"
				"varying vec4 v_color;"
				"varying float v_shadow;"
				"void main()"
				"{"
				"    gl_Position = vec4(a_position.xy,-a_position.z,1.0);"
				"    v_color = a_color;"
				"    v_shadow = a_position.w * 0.25;"
				"}";

		const char* fSource =
				"precision highp float;"
				"varying lowp vec4 v_color;"
				"varying float v_shadow;"
				"void main()"
				"{"
				"    gl_FragColor = vec4(v_shadow, 0.0, 0.0, 0.0);"
//				"    gl_FragColor = vec4(1.0, 0.0, 0.0, 0.0);"
				"}";

		if (EXIT_FAILURE == createShader(&s_defaultHardShadowShader, vSource, fSource))
			return EXIT_FAILURE;

		glUseProgram(s_defaultHardShadowShader);
		glBindAttribLocation(s_defaultHardShadowShader, positionLoc, "a_position");
		glBindAttribLocation(s_defaultHardShadowShader, colorLoc, "a_color");
		glLinkProgram(s_defaultHardShadowShader);
	}

	{
		// Create shaders
		const char* vSource =
				"precision mediump float;"
				"attribute vec4 a_position;"
				"attribute vec4 a_color;"
				"attribute vec2 a_alphacoord;"
				"varying vec4 v_pos;"
				"varying vec4 v_color;"
				"varying vec2 v_alphacoord;"
				"void main()"
				"{"
				"    gl_Position = vec4(a_position.xy,-a_position.z,1.0);"
				"    v_pos = a_position * 0.5 + vec4(0.5);"
				"    v_color = a_color;"
				"    v_alphacoord = a_alphacoord;"
				"}";

		const char* fSource =
				"precision mediump float;"
				"uniform sampler2D u_alpha;"
				"uniform vec3 u_normal;"
				"varying vec4 v_pos;"
				"varying vec2 v_alphacoord;"
				"varying vec4 v_color;"
				"void main()"
				"{"
				"    float alpha = texture2D(u_alpha, v_alphacoord).a + v_color.a;"
#if USE_16BIT_DEPTH
				"    lowp float r = floor(v_pos.z * 255.0) / 255.0;"
				"    lowp float g = (v_pos.z - r) * 255.0;"
				"    gl_FragColor = vec4(r, g, u_normal.x / u_normal.z, u_normal.y / u_normal.z);"
#else
				"    gl_FragColor = vec4(v_pos.z, u_normal);"
#endif
				"    if(alpha < 0.5) discard;"
//				"    gl_FragColor = vec4(v_pos.xyz, step(0.5, alpha));"
				"}";

		if (EXIT_FAILURE == createShader(&s_depthShader, vSource, fSource))
			return EXIT_FAILURE;

		glUseProgram(s_depthShader);
		glBindAttribLocation(s_depthShader, positionLoc, "a_position");
		glBindAttribLocation(s_depthShader, colorLoc, "a_color");
		glBindAttribLocation(s_depthShader, alphaLoc, "a_alphacoord");
		glLinkProgram(s_depthShader);
		glUniform1i(glGetUniformLocation(s_depthShader, "u_alpha"), 0);
	}

	{
		// Create shaders
		const char* vSource =
				"precision mediump float;"
				"attribute vec4 a_position;"
				"attribute vec4 a_color;"
				"attribute vec2 a_alphacoord;"
				"varying vec4 v_pos;"
				"varying vec4 v_color;"
				"varying vec2 v_alphacoord;"
				"void main()"
				"{"
				"    gl_Position = vec4(a_position.xy,-a_position.z,1.0);"
				"    v_pos = a_position * 0.5 + vec4(0.5);"
				"    v_color = a_color;"
				"    v_alphacoord = a_alphacoord;"
				"}";

		const char* fSource =
				"precision mediump float;"
				"uniform sampler2D u_alpha;"
				"uniform vec3 u_normal;"
				"varying vec4 v_pos;"
				"varying vec2 v_alphacoord;"
				"varying vec4 v_color;"
				"void main()"
				"{"
#if USE_16BIT_DEPTH
				"    lowp float r = floor(v_pos.z * 255.0) / 255.0;"
				"    lowp float g = (v_pos.z - r) * 255.0;"
				"    gl_FragColor = vec4(r, g, u_normal.x / u_normal.z, u_normal.y / u_normal.z);"
#else
				"    gl_FragColor = vec4(v_pos.z, u_normal);"
#endif
				"}";

		if (EXIT_FAILURE == createShader(&s_depthShader2, vSource, fSource))
			return EXIT_FAILURE;

		glUseProgram(s_depthShader2);
		glBindAttribLocation(s_depthShader2, positionLoc, "a_position");
		glBindAttribLocation(s_depthShader2, colorLoc, "a_color");
		glBindAttribLocation(s_depthShader2, alphaLoc, "a_alphacoord");
		glLinkProgram(s_depthShader2);
		glUniform1i(glGetUniformLocation(s_depthShader2, "u_alpha"), 0);
	}

	{
		// Create shaders
		const char* vSource =
				"precision mediump float;"
				"attribute vec4 a_position;"
				"attribute vec4 a_color;"
				"uniform vec3 u_normal;"
				"uniform vec3 u_lightPos;"
				"varying vec4 v_color;"
				"varying vec2 v_texcoord;"
				"varying vec2 v_pos;"
				"varying lowp float v_inShadow;"
				"varying float v_diff;"
				"void main()"
				"{"
				"    vec4 pos = vec4(a_position.xy,-a_position.z,1.0);"
				"    gl_Position = pos;"
				"    v_color = a_color;"
				"    v_texcoord = 0.5 * pos.xy + vec2(0.5,0.5);"
				"    v_pos = pos.xy;"
				"    v_diff = clamp(dot(normalize(u_lightPos - a_position.xyz), u_normal), 0.0, 1.0);"
				"    v_inShadow = step(0.0, -u_normal.z);"
					"}";
		const char* fSource =
				"precision mediump float;"
				"uniform sampler2D u_tex;"
				"varying vec4 v_color;"
				"varying vec2 v_texcoord;"
				"varying vec2 v_pos;"
				"varying lowp float v_inShadow;"
				"varying float v_diff;"
				"uniform float u_bias;"
				"void main()"
				"{"
#if 0
				// 0 3 18 72
				"    vec2 pos = v_pos;"
//				"    pos = (1.0 - u_bias / 256.0) *  pos;"
				"    vec4 t = texture2D(u_tex, pos * 0.5 + vec2(0.5));"
				"    vec4 pass = step(vec4(3.0, 18.0, 72.0, 0.0), vec4(u_bias));"
				"    pass = vec4(1.0 - pass.x, pass.x * (1.0 - pass.y), pass.y * (1.0 - pass.z), pass.z);"
				"    vec2 boundary;"
				"    boundary.x = dot(pass, t);"
				"    boundary.y = dot(pass, vec4(t.gba, 0.0));"
				"    float invRange = dot(pass, vec4(1.0 / 3.0, 1.0 / 15.0, 1.0 / 54.0, 1.0 / 184.0));"
				"    float offset = dot(pass, vec4(0.0 / 3.0, 3.0 / 15.0, 18.0 / 54.0, 72.0 / 184.0));"
				"    float alpha = invRange * u_bias - offset;"
				"    float shadow = mix(boundary.x, boundary.y, alpha);"
//				"    if(u_bias > 72.0) {"
//				"        shadow = texture2D(u_tex, v_pos * 72.0 / u_bias * 0.5 + vec2(0.5)).a;"
//				"        shadow = shadow / u_bias * 72.0;"
//				"    }"
				"    shadow = t.b;"
				"    if(shadow >= 0.25) { gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0); return;}"
				"    else if(pos.x >= -2.0 / 3.0 && pos.x <= 2.0 /3.0 && pos.y >=-2.0 / 3.0 && pos.y <= 2.0 / 3.0) { }"
				"    else { gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0); return; }"
#else
				"    vec4 t = texture2D(u_tex, v_texcoord);"
				"    float shadow;"
				"    shadow = t.r;"
//				"    if(t.g > 0.35) shadow = 0.2; else"
#endif
				"    gl_FragColor = vec4(v_color.rgb * (1.0 - 0.5 * smoothstep(0.0, 0.25, shadow)), v_color.a);"
//				"    gl_FragColor = vec4(v_color.rgb * (1.0 - 0.5 * clamp(1.0 - v_diff + smoothstep(0.0, 0.25, shadow), 0.0, 1.0)), v_color.a);"
				"    gl_FragColor = vec4(v_color.rgb * (1.0 - 0.5 * smoothstep(0.0, 0.25, shadow + 0.25 * (1.0 - v_diff))), v_color.a);"
//				"    gl_FragColor = texture2D(u_tex, v_texcoord);"
				"}";

		if (EXIT_FAILURE == createShader(&s_defaultSceneShader, vSource, fSource))
			return EXIT_FAILURE;

		glUseProgram(s_defaultSceneShader);
		glBindAttribLocation(s_defaultSceneShader, positionLoc, "a_position");
		glBindAttribLocation(s_defaultSceneShader, colorLoc, "a_color");
		glLinkProgram(s_defaultSceneShader);
		glUniform1i(glGetUniformLocation(s_defaultSceneShader, "u_tex"), 1);
	}

	{
		// Create shaders
		const char* vSource =
				"precision mediump float;"
				"attribute vec4 a_position;"
				"attribute vec4 a_color;"
				"attribute vec2 a_alphacoord;"
				"uniform vec3 u_normal;"
				"uniform vec3 u_lightPos;"
				"varying vec4 v_color;"
				"varying vec2 v_texcoord;"
				"varying vec2 v_alphacoord;"
				"varying lowp float v_inShadow;"
				"varying float v_diff;"
				"void main()"
				"{"
				"    vec4 pos = vec4(a_position.xy,-a_position.z,1.0);"
				"    gl_Position = pos;"
				"    v_color = a_color;"
				"    v_texcoord = 0.5 * pos.xy + vec2(0.5,0.5);"
				"    v_alphacoord = a_alphacoord;"
				"    v_diff = clamp(dot(normalize(u_lightPos - a_position.xyz), u_normal), 0.0, 1.0);"
				"    v_inShadow = step(0.0, -u_normal.z);"
				"}";

		const char* fSource =
				"precision mediump float;"
				"uniform sampler2D u_tex;"
				"uniform sampler2D u_alpha;"
				"varying vec4 v_color;"
				"varying vec2 v_texcoord;"
				"varying vec2 v_alphacoord;"
				"varying lowp float v_inShadow;"
				"varying float v_diff;"
				"void main()"
				"{"

//				"    gl_FragColor = vec4(texture2D(u_tex, v_texcoord).rrr * 4.0, 1.0); return;"
				"    mediump vec4 t = texture2D(u_tex, v_texcoord);"
				"    float shadow = t.r;"
//				"    if(shadow > 0.0125) { gl_FragColor = 0.5*v_color; return; }  else if(shadow > 0.0) {gl_FragColor = 0.2*v_color; return;} else {gl_FragColor =v_color; return; }"
				"    vec4 tex = texture2D(u_alpha, v_alphacoord);"
				"    if(tex.a < 0.5) discard;"
				"    gl_FragColor = vec4(tex.rgb * (1.0 - 0.5 * smoothstep(0.0, 0.25, shadow)), tex.a + v_color.a);"
//				"    gl_FragColor = vec4(texture2D(u_tex, v_texcoord).a, 0.0, 0.0, 0.0);"
				"    gl_FragColor = vec4(tex.rgb * (1.0 - 0.5 * smoothstep(0.0, 0.25, shadow + 0.25 * (1.0 - v_diff))), tex.a + v_color.a);"
//				"    gl_FragColor = texture2D(u_tex, v_texcoord);"
				"}";

		if (EXIT_FAILURE == createShader(&s_sceneShader, vSource, fSource))
			return EXIT_FAILURE;

		glUseProgram(s_sceneShader);
		glBindAttribLocation(s_sceneShader, positionLoc, "a_position");
		glBindAttribLocation(s_sceneShader, colorLoc, "a_color");
		glBindAttribLocation(s_sceneShader, alphaLoc, "a_alphacoord");
		glLinkProgram(s_sceneShader);
		glUniform1i(glGetUniformLocation(s_sceneShader, "u_tex"), 1);
		glUniform1i(glGetUniformLocation(s_sceneShader, "u_alpha"), 0);
	}

	{
		// Create shaders
		const char* vSource =
				"precision mediump float;"
				"attribute vec4 a_position;"
				"attribute vec4 a_color;"
				"varying vec4 v_color;"
				"varying vec2 v_texcoord;"
				"varying vec3 v_pos;"
				"void main()"
				"{"
				"    vec4 pos = vec4(a_position.xy,-a_position.z,1.0);"
				"    gl_Position = pos;"
				"    v_color = a_color;"
				"    v_texcoord = pos.xy * 0.5 + vec2(0.5);"
				"    v_pos = a_position.xyz;"
				"}";

		const char* fSource =
				"precision mediump float;"
				"uniform sampler2D u_tex;"
				"uniform sampler2D u_shadow;"
				"uniform mat3 cMat;"
				"uniform vec2 centerCa, centerCb;"
				"uniform vec2 quadLengthCa, quadLengthCb;"
				"uniform vec2 texQuadLength;"
				"uniform vec3 vl;"
				"uniform vec3 normal;"
				"uniform vec3 newNormal;"
				"uniform float blurCa, blurCb;"
				"uniform float flipped;"
				"varying vec3 v_pos;"
				"varying lowp vec4 v_color;"
				"varying mediump vec2 v_texcoord;"
				"uniform vec4 light;"
				"uniform vec3 quadCenter;"
				"void main()"
				"{"
#if 1
//				"    gl_FragColor = vec4(1.0, 0.0, 0.0, 0.0); return;"
				"    vec4 t = texture2D(u_tex, v_texcoord);"
#if USE_16BIT_DEPTH
				"    float scene_depth = t.r + t.g / 255.0;"
				"    scene_depth = scene_depth * 2.0 - 1.0;"
				"    vec3 scene_normal = normalize(vec3(t.ba, 1.0));"
#else
				"    float scene_depth = t.r * 2.0 - 1.0;"
				"    vec3 scene_normal = t.gba;"
#endif
//				"    float scene_depth = -0.5;"
				"    vec2 shadowCoord = vec2(-0.5, -0.5);"
				"    vec3 vq = vec3(v_pos.x, v_pos.y, scene_depth);"

				"    vec3 vqt = cMat * vq;"
				"    vec2 vo = centerCa * vqt.z + centerCb;"
				"    vo = vqt.xy - vo;"
				"    vec2 size = quadLengthCa * vqt.z + quadLengthCb;"
				"    vec2 scaleSize = texQuadLength / size;"
				"    shadowCoord = vec2(vo.x * flipped, vo.y) * scaleSize;"

				"    float blurRad = (blurCa * vqt.z + blurCb);"
//				"    blurRad = blurRad / normalize(vl - vqt).z;"
//				"    blurRad = blurRad / dot(normalize(light.xyz - vq), scene_normal);"

				"    highp float u_bias = blurRad * scaleSize.x * 256.0;"

#if LERP_TEXTURE_NUM == 4
				// 0 3 18 72
				"    t = texture2D(u_shadow, shadowCoord * 0.5 + vec2(0.5));"
				"    vec4 pass = step(vec4(3.0, 18.0, 72.0, 0.0), vec4(u_bias));"
				"    pass = vec4(1.0 - pass.x, pass.x * (1.0 - pass.y), pass.y * (1.0 - pass.z), pass.z);"
				"    vec2 boundary;"
				"    boundary.x = dot(pass, t);"
				"    boundary.y = dot(pass, vec4(t.gba, 0.0));"
				"    float invRange = dot(pass, vec4(1.0 / 3.0, 1.0 / 15.0, 1.0 / 54.0, 1.0 / 1000.0));"
				"    float offset = dot(pass, vec4(0.0 / 3.0, 3.0 / 15.0, 18.0 / 54.0, 72.0 / 1000.0));"
				"    float alpha = invRange * u_bias - offset;"
#elif LERP_TEXTURE_NUM == 6
				// 0 3 9 24 60 108
				"    vec4 pass1 = vec4(step(3.0, u_bias), step(9.0, u_bias), step(24.0, u_bias), 0.0);"
				"    vec4 pass2 = vec4(pass1.z, step(60.0, u_bias), step(108.0, u_bias), 0.0);"
				"    float t1Scale = 1.0 - 0.5 * pass1.z;"
				"    float t2Scale = 1.0 - 0.5 * pass1.y;"
				"    pass1 = vec4(1.0 - pass1.x, pass1.x * (1.0 - pass1.y), pass1.y * (1.0 - pass1.z), pass1.z);"
				"    pass2 = vec4(1.0 - pass2.x, pass2.x * (1.0 - pass2.y), pass2.y * (1.0 - pass2.z), pass2.z);"
				"    vec2 t1Offset = vec2(mix(0.0, 0.5, pass2.z), mix(0.0, 0.5, pass2.w));"
				"    vec2 t2Offset = vec2(mix(0.0, 0.5, pass2.y), mix(0.0, 0.5, pass2.z));"
				"    vec2 boundary;"
				"    t = texture2D(u_shadow, shadowCoord * t1Scale + t1Offset);"
				"    boundary.x = dot(pass1.xyz, t.rgb) + dot(pass2.yzw, t.aaa);"
				"    t = texture2D(u_shadow, shadowCoord * t2Scale + t2Offset);"
				"    boundary.y = dot(pass1.xyz, t.gba) + dot(pass2.yzw, vec3(t.aa, 0.0));"
				"    float invRange = dot(pass1.xyz, vec3(1.0 / 3.0, 1.0 / 6.0, 1.0 / 15.0)) + dot(pass2.yzw, vec3(1.0 / 36.0, 1.0 / 48.0, 1.0 / 4096.0));"
				"    float offset = dot(pass1.xyz, vec3(0.0 / 3.0, 3.0 / 6.0, 9.0 / 15.0)) + dot(pass2.yzw, vec3(24.0 / 36.0, 60.0 / 48.0, 108.0 / 4096.0));"
				"    float alpha = invRange * u_bias - offset;"
#endif
//				"    float shadow = step(scene_depth, v_pos.z) * mix(boundary.x, boundary.y, alpha);"
				"    highp float shadow = mix(boundary.x, boundary.y, alpha);"
//				"    if(u_bias > 72.0) shadow = t.a;"
//				"    if(u_bias > 72.0) {"
//				"        shadow = texture2D(u_shadow, shadowCoord * 72.0 / u_bias * 0.5 + vec2(0.5)).r;"
//				"        shadow = shadow / u_bias * 72.0;"
//				"    }"
//				"    shadow = t.b;"
//				"    if(shadow < 0.0125) shadow = 1.0; else shadow = 0.05;"
//				"    if(t.r > 0.1) { if(shadow > 0.1) shadow = 1.0; else shadow = 0.05; } else shadow = 0.0;"
//				"    if(u_bias >= 72.0) shadow = 0.05; else shadow = 1.0;"
				"    gl_FragColor = vec4(shadow, 0.0, 0.0, 0.0);"
//				"    gl_FragColor = vec4(shadow * 4.0, shadow * 4.0, shadow * 4.0, 1.0);"
//				"    gl_FragColor = vec4(1.0);"
//				"    gl_FragColor = vec4(texture2D(u_tex, v_texcoord).zzz, 1.0);"
#else
				"    float scene_depth = texture2D(u_tex, v_texcoord).z * 2.0 - 1.0;"
#endif
				"}";

		if (EXIT_FAILURE == createShader(&s_shadowShader, vSource, fSource))
			return EXIT_FAILURE;

		glUseProgram(s_shadowShader);
		glBindAttribLocation(s_shadowShader, positionLoc, "a_position");
		glBindAttribLocation(s_shadowShader, colorLoc, "a_color");
		glLinkProgram(s_shadowShader);
		glUniform1i(glGetUniformLocation(s_shadowShader, "u_tex"), 1);
		glUniform1i(glGetUniformLocation(s_shadowShader, "u_shadow"), 0);
	}

	{
		// Create shaders
		const char* vSource =
				"precision mediump float;"
				"attribute vec4 a_position;"
				"varying vec2 v_texcoord;"
				"void main()"
				"{"
				"    gl_Position = vec4(a_position.xy, 0.0, 1.0);"
				"    v_texcoord = 0.5 * a_position.xy + vec2(0.5,0.5);"
					"}";
		const char* fSource =
				"precision mediump float;"
				"uniform sampler2D u_tex;"
				"varying vec2 v_texcoord;"
				"void main()"
				"{"
				"    gl_FragColor = texture2D(u_tex, v_texcoord);"
				"}";

		if (EXIT_FAILURE == createShader(&s_copyShader, vSource, fSource))
			return EXIT_FAILURE;

		glUseProgram(s_copyShader);
		glBindAttribLocation(s_copyShader, positionLoc, "a_position");
		glLinkProgram(s_copyShader);
		glUniform1i(glGetUniformLocation(s_copyShader, "u_tex"), 1);
	}

	const char* vGaussNTapSource =
			"precision lowp float;"
			"attribute lowp vec2 a_position;"
			"attribute lowp vec2 a_texcoord;"
			"varying lowp vec2 v_texcoord;"
			"void main()"
			"{"
			"    gl_Position = vec4(a_position, 0.0, 1.0);"
			"    v_texcoord = a_texcoord;"
			"}";

	gaussTaps = 9;
	const char* fGaussNTapSource =
			"varying lowp vec2 v_texcoord;"
			"uniform lowp sampler2D g_texture;"
			"uniform mediump float u_bias;"
			"uniform mediump float u_delta;"
			"void main() {"
			// since we have to do this twice, anyway, we'll just swap x and y
			// which will be undone in the second pass
			"   mediump vec2 ts = v_texcoord.ts;"
#if 1
			// this one takes advantange of linear interpolation -- it's a version of the 9 tap, strangely enough
			"   mediump float o1 = u_delta * 1.3846153846;"
			"   mediump float o2 = u_delta * 3.2307692308;"
			"   mediump vec4 sum  = "
			"             0.0702702703 * texture2D(g_texture, vec2(ts.x - o2, ts.y), u_bias)"
			"          +  0.3162162162 * texture2D(g_texture, vec2(ts.x - o1, ts.y), u_bias)"
			"          +  0.2270270270 * texture2D(g_texture, ts                   , u_bias)"
			"          +  0.3162162162 * texture2D(g_texture, vec2(ts.x + o1, ts.y), u_bias)"
			"          +  0.0702702703 * texture2D(g_texture, vec2(ts.x + o2, ts.y), u_bias);"
			"   gl_FragColor = sum;"
#endif
			"}";

	const char* vBoxBlurSource =
			"precision lowp float;"
			"attribute lowp vec2 a_position;"
			"attribute lowp vec2 a_texcoord;"
			"varying lowp vec2 v_texcoord;"
			"void main()"
			"{"
			"    gl_Position = vec4(a_position, 0, 1);"
			"    v_texcoord = a_texcoord;"
			"}";

	const char* fBoxBlurSource =
			"varying lowp vec2 v_texcoord;"
			"uniform lowp sampler2D g_texture;"
			"uniform mediump float u_bias;"
			"uniform mediump float u_delta;"
			"void main() {"
			// since we have to do this twice, anyway, we'll just swap x and y
			// which will be undone in the second pass
			"   gl_FragColor ="
			"          (texture2D(g_texture, vec2(v_texcoord.y - u_delta, v_texcoord.x), u_bias) +"
			"           texture2D(g_texture, vec2(v_texcoord.y + u_delta, v_texcoord.x), u_bias)) * 0.5;"
			"}";


	if ( EXIT_FAILURE == createShader(&gaussNTapProgram, vGaussNTapSource, fGaussNTapSource) )
		return EXIT_FAILURE;

	if ( EXIT_FAILURE == createShader(&boxBlurProgram, vBoxBlurSource, fBoxBlurSource) )
		return EXIT_FAILURE;

	s_shaderCompiled = true;

	return 0;
}

ShadowRenderer::ShadowRenderer(int surfaceWidth, int surfaceHeight) {

	m_surfaceWidth = surfaceWidth;
	m_surfaceHeight = surfaceHeight;

	genShaders();

	glGenFramebuffers(1, &m_blurFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_blurFbo);

	glGenTextures(1, &m_blurTex);
	glBindTexture(GL_TEXTURE_2D, m_blurTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_blurTex, 0);

	if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
		exit(EXIT_FAILURE);
	}

	glGenFramebuffers(1, &m_shadowFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFbo);

	glGenTextures(1, &m_shadowTex);
	glBindTexture(GL_TEXTURE_2D, m_shadowTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_surfaceWidth, m_surfaceHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glGenTextures(1, &m_depthTex);
	glBindTexture(GL_TEXTURE_2D, m_depthTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_surfaceWidth, m_surfaceHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
#if USE_TWO_FBO
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_shadowTex, 0);
#endif

	glGenRenderbuffers(1, &m_shadowRender);
	glBindRenderbuffer(GL_RENDERBUFFER, m_shadowRender);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, m_surfaceWidth, m_surfaceHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_shadowRender);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_shadowRender);

//	GLuint depthBuffer;
//	glGenTextures(1, &depthBuffer);
//	glBindTexture(GL_TEXTURE_2D, depthBuffer);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_surfaceWidth, m_surfaceHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, 0);
//	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffer, 0);

	if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
		exit(EXIT_FAILURE);
	}

	glGenFramebuffers(1, &m_depthFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_depthFbo);

#if USE_TWO_FBO
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_depthTex, 0);
#endif

	glGenRenderbuffers(1, &m_depthRender);
	glBindRenderbuffer(GL_RENDERBUFFER, m_depthRender);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, m_surfaceWidth, m_surfaceHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRender);
	if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
		exit(EXIT_FAILURE);
	}


	glGenFramebuffers(1, &m_sceneFbo);
	glGenRenderbuffers(1, &m_sceneRender);
	glGenTextures(1, &m_sceneTex);
	glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFbo);

	glBindRenderbuffer(GL_RENDERBUFFER, m_sceneRender);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, m_surfaceWidth, m_surfaceHeight);

	glBindTexture(GL_TEXTURE_2D, m_sceneTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_surfaceWidth, m_surfaceHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_sceneTex, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_sceneRender);
	if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
		exit(EXIT_FAILURE);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	m_lightPos = vec3(0.0, 0.0, 2.0);
	m_lightRadius = 0.5;
	m_groundZ = -0.5;
	m_groundPlane[0] = vec3(-1.0, -1.0, m_groundZ);
	m_groundPlane[1] = vec3( 1.0, -1.0, m_groundZ);
	m_groundPlane[2] = vec3(-1.0,  1.0, m_groundZ);
	m_groundPlane[3] = vec3( 1.0,  1.0, m_groundZ);

	m_sceneUpdated = false;

	// generate default shadow texture
	TexInfo defaultTexInfo;
	defaultTexInfo._alphaTex = 0;
	defaultTexInfo._hardShadowCoord[0] = vec2(-sampleHardShadowBound, -sampleHardShadowBound);
	defaultTexInfo._hardShadowCoord[1] = vec2( sampleHardShadowBound, -sampleHardShadowBound);
	defaultTexInfo._hardShadowCoord[2] = vec2(-sampleHardShadowBound,  sampleHardShadowBound);
	defaultTexInfo._hardShadowCoord[3] = vec2( sampleHardShadowBound,  sampleHardShadowBound);

	defaultTexInfo._alphaCoord[0] = vec2(0.0, 0.0);
	defaultTexInfo._alphaCoord[1] = vec2(1.0, 0.0);
	defaultTexInfo._alphaCoord[2] = vec2(0.0, 1.0);
	defaultTexInfo._alphaCoord[3] = vec2(1.0, 1.0);

	defaultTexInfo._texQuadLength = defaultTexInfo._hardShadowCoord[3] - defaultTexInfo._hardShadowCoord[0];

	genShadowTexture(defaultTexInfo);
	m_textures.insert(make_pair(defaultTexInfo._alphaTex, defaultTexInfo));
}

ShadowRenderer::~ShadowRenderer() {
	for(size_t i = 0; i < m_objects.size(); i++)
		delete m_objects[i];
	m_objects.clear();
	m_textures.clear();
	std::cout << "ShadowRenderer destructed!" << std::endl;
}

void ShadowRenderer::setLight(vec3 lightPos, GLfloat lightRadius) {
	m_lightPos = lightPos;
	m_lightRadius = lightRadius;
	for(size_t i = 0; i < m_objects.size(); i++)
		m_objects[i]->setLight(m_lightPos, m_lightRadius);
	m_sceneUpdated = false;
}

void ShadowRenderer::setGroundZ(GLfloat groundZ) {
	m_groundZ = groundZ;
	for(int i = 0; i < 4; i++)
		m_groundPlane[i].z = m_groundZ;
	for(size_t i = 0; i < m_objects.size(); i++)
		m_objects[i]->setGroundZ(m_groundZ);
	m_sceneUpdated = false;
}

GLuint ShadowRenderer::addObject(vec3 center, vec2 size, vec3 color) {

	Quad* newObject = new Quad(center, size, color);
	newObject->setLight(m_lightPos, m_lightRadius);
	newObject->setGroundZ(m_groundZ);
	m_objects.push_back(newObject);
	GLuint objectId = m_objects.size() - 1;
	bindTexture(objectId, 0);
	m_renderProperty.push_back(RenderProperty(objectId, center.z, false));
	m_objectsUpdated = false;
	m_sceneUpdated = false;
	return objectId;
}

static void NEONBlur(pixel* input, unsigned int widhght, unsigned int blurRadius) {

	pixel* xblur = input;
	pixel* yblur = (pixel*)malloc(sizeof(pixel)*widhght*widhght);
	if ( blurRadius > 1) {
		// we want to approximate a gauss blur of radius rad with 3 box blurs
		// if we had box blurs that worked for fractional radii, then we'd
		// just use rad/3 for each blur.
		// but we don't.
		//
		// instead, we'll find s and t such that rad = 3*d + rem.  (d>=0, rem>=0)
		// here are the cases.
		//
		// if d == 0:
		//    we don't have enough to do 3 blurs.
		//    do a total of "rem" 1-radius blurs
		// else d > 0:
		//    if rem == 0:
		//        it's an exact fit, so do 3 "d" radius blurs
		// else:
		//    perform r (d+1)blurs and (3-r) (d)blurs ( notice that r*(d+1) + (3-r)*d = r*d + r + 3*d - r*d
		//                                                                            = 3*d + r (which is good).
		unsigned int d = blurRadius / 3;
		unsigned int r = blurRadius % 3;
		// fprintf(stderr,"Gauss blur rad  %d = 3 * %d + %d\n", blurRadius, d, r);

		if ( d == 0 ) {
			// do as many radius 1 box blurs as will fit in the remainder
			// fprintf(stderr,"since d == 0, doing %d box blurs of radius 1\n", r);
			for ( ; r > 0 ; r-- ) {
				NEONboxBlur(xblur,yblur,widhght,1);
				NEONboxBlur(yblur,xblur,widhght,1);
			}
		} else {
			if ( r == 0 ) { // perfect fit -- do 3 box blurs of radius rad/3
				// fprintf(stderr,"since r == 0, doing 3 box blurs of radius %d\n", d);
				NEONboxBlur(xblur,yblur,widhght,d);
				NEONboxBlur(yblur,xblur,widhght,d);
				NEONboxBlur(xblur,yblur,widhght,d);
				NEONboxBlur(yblur,xblur,widhght,d);
				NEONboxBlur(xblur,yblur,widhght,d);
				NEONboxBlur(yblur,xblur,widhght,d);
			} else {
				unsigned int midrad = (r == 1)?d:d+1;
				// fprintf(stderr,"since r != 0, doing 3 box blurs of radius %d, %d and %d\n", d+1, midrad, d);
				NEONboxBlur(xblur,yblur,widhght,d+1);
				NEONboxBlur(yblur,xblur,widhght,d+1);
				NEONboxBlur(xblur,yblur,widhght,midrad);
				NEONboxBlur(yblur,xblur,widhght,midrad);
				NEONboxBlur(xblur,yblur,widhght,d);
				NEONboxBlur(yblur,xblur,widhght,d);
			}
		}
	}
	free(yblur);
}

static void downSample(const pixel* input, unsigned int widhght, pixel* result) {

	unsigned int halfWidhght = widhght / 2;
	unsigned int i, j;
	for(i = 0; i < halfWidhght; i++) {
		for(j = 0; j < halfWidhght; j++) {
			// average r channel only, which is the only channel that contains shadow value
			result[i * halfWidhght + j].r = (input[(2 * i) * widhght + (2 * j)].r
			                              + input[(2 * i) * widhght + (2 * j + 1)].r
			                              + input[(2 * i + 1) * widhght + (2 * j)].r
			                              + input[(2 * i + 1) * widhght + (2 * j + 1)].r) / 4;
		}
	}
}

int ShadowRenderer::genShadowTexture(TexInfo& texInfo) {

	glBindTexture(GL_TEXTURE_2D, texInfo._alphaTex);
	glBindFramebuffer(GL_FRAMEBUFFER, m_blurFbo);

	glClearColor(0.0,0.0,0.0,0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, widhght, widhght);

	if(texInfo._alphaTex == 0) {
		glUseProgram(s_defaultHardShadowShader);
	} else {
		glUseProgram(s_hardShadowShader);
	}
	glEnableVertexAttribArray(positionLoc);
	glEnableVertexAttribArray(alphaLoc);

	// extended the quad a little bit to avoid gap in the shadow
	vec2 hardShadowCoord[4];
	hardShadowCoord[0] = texInfo._hardShadowCoord[0] + vec2(-1.0 /256.0, -1.0 /256.0);
	hardShadowCoord[1] = texInfo._hardShadowCoord[1] + vec2( 1.0 /256.0, -1.0 /256.0);
	hardShadowCoord[2] = texInfo._hardShadowCoord[2] + vec2(-1.0 /256.0,  1.0 /256.0);
	hardShadowCoord[3] = texInfo._hardShadowCoord[3] + vec2( 1.0 /256.0,  1.0 /256.0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
//	glVertexAttribPointer(positionLoc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), texInfo._hardShadowCoord);
	glVertexAttribPointer(positionLoc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), hardShadowCoord);
	for(int i = 0; i < 4; i++)
		std::cout << texInfo._hardShadowCoord[i] << std::endl;
	glVertexAttribPointer(alphaLoc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), texInfo._alphaCoord);
	glDrawArrays(GL_TRIANGLE_STRIP, 0 , 4);

	glDisableVertexAttribArray(positionLoc);
	glDisableVertexAttribArray(alphaLoc);

	static pixel *origTex = 0;
	static pixel *downSampledTex = 0;
	static pixel *finalTex = 0;
	unsigned int halfWidhght = widhght / 2;

	if ( !origTex ) origTex = (pixel*)malloc(sizeof(pixel)*widhght*widhght);
	if ( !finalTex ) finalTex = (pixel*)malloc(sizeof(pixel)*widhght*widhght);
	if ( !downSampledTex ) downSampledTex = (pixel*)malloc(sizeof(pixel)*halfWidhght*halfWidhght);

	glBindTexture(GL_TEXTURE_2D, m_blurTex);

#ifdef MIPMAP_TEXTURE
#elif LERP_TEXTURE_NUM == 4
	// 0 3 24 72

	// r channel, blurRad = 0.0
	glReadPixels(0,0,widhght,widhght,GL_RGBA, GL_UNSIGNED_BYTE, origTex);


	for(int i = 0; i < widhght*widhght; i++)
		finalTex[i].r = origTex[i].r;

//	std::cout << "origTex[127 * 256 + 35].r = " << (int)origTex[127 * 256 + 35].r << endl;
//	std::cout << "origTex[127 * 256 + 36].r = " << (int)origTex[127 * 256 + 36].r << endl;
//	std::cout << "origTex[127 * 256 + 37].r = " << (int)origTex[127 * 256 + 37].r << endl;
//	std::cout << "origTex[127 * 256 + 38].r = " << (int)origTex[127 * 256 + 38].r << endl;
//	std::cout << "origTex[127 * 256 + 39].r = " << (int)origTex[127 * 256 + 39].r << endl;
//	std::cout << "origTex[127 * 256 + 40].r = " << (int)origTex[127 * 256 + 40].r << endl;
//	std::cout << "origTex[127 * 256 + 41].r = " << (int)origTex[127 * 256 + 41].r << endl;
//	std::cout << "origTex[127 * 256 + 42].r = " << (int)origTex[127 * 256 + 42].r << endl;
//	std::cout << "origTex[127 * 256 + 43].r = " << (int)origTex[127 * 256 + 43].r << endl;
//	std::cout << "origTex[127 * 256 + 44].r = " << (int)origTex[127 * 256 + 44].r << endl;
//	std::cout << std::endl;

	// g channel, blurRad = 3.0
	NEONBlur(origTex, widhght, 3.0);
	for(int i = 0; i < widhght*widhght; i++)
		finalTex[i].g = origTex[i].r;

	// b channel, blurRad = 3.0 + 15.0 = 18.0
	NEONBlur(origTex, widhght, 15.0);
	for(int i = 0; i < widhght*widhght; i++)
		finalTex[i].b = origTex[i].r;

	// a channel, blurRad = 18.0 + 54.0 = 72.0
	NEONBlur(origTex, widhght, 54.0);
	for(int i = 0; i < widhght*widhght; i++)
		finalTex[i].a = origTex[i].r;

#elif LERP_TEXTURE_NUM == 6
	// 0 3 9 24 60 108 inf

	// r channel, blurRad = 0.0
	glReadPixels(0,0,widhght,widhght,GL_RGBA, GL_UNSIGNED_BYTE, origTex);
	for(int i = 0; i < widhght*widhght; i++)
		finalTex[i].r = origTex[i].r;

	// g channel, blurRad = 3.0
	NEONBlur(origTex, widhght, 3.0);
	for(int i = 0; i < widhght*widhght; i++)
		finalTex[i].g = origTex[i].r;

	// b channel, blurRad = 3.0 + 6.0 = 9.0
	NEONBlur(origTex, widhght, 6.0);
	for(int i = 0; i < widhght*widhght; i++)
		finalTex[i].b = origTex[i].r;

	// a channel

	// leftbottom, blurRad = 6.0 + 2 * 9.0 = 24.0
	downSample(origTex, widhght, downSampledTex);
	NEONBlur(downSampledTex, halfWidhght, 9.0);
	for(int i = 0; i < halfWidhght; i++) {
		for(int j = 0; j < halfWidhght; j++) {
			finalTex[i * widhght + j].a = downSampledTex[i * halfWidhght + j].r;
		}
	}

	// rightbottom, blurRad = 24.0 + 2 * 18.0 = 60.0
	NEONBlur(downSampledTex, halfWidhght, 18.0);
	for(int i = 0; i < halfWidhght; i++) {
		for(int j = 0; j < halfWidhght; j++) {
			finalTex[i * widhght + j + halfWidhght].a = downSampledTex[i * halfWidhght + j].r;
		}
	}

	// lefttop, blurRad = 60.0 + 2 * 24.0 = 108.0
	NEONBlur(downSampledTex, halfWidhght, 24.0);
	for(int i = 0; i < halfWidhght; i++) {
		for(int j = 0; j < halfWidhght; j++) {
			finalTex[(i + halfWidhght) * widhght + j].a = downSampledTex[i * halfWidhght + j].r;
		}
	}

	// righttop, blurRad = inf(blank)

#endif

	glGenTextures(1, &texInfo._shadowTex);
	glBindTexture(GL_TEXTURE_2D, texInfo._shadowTex);

#ifdef MIPMAP_TEXTURE
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, widhght, widhght, 0, GL_RGBA, GL_UNSIGNED_BYTE, finalTex);
	glGenerateMipmap(GL_TEXTURE_2D);
#else
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, widhght, widhght, 0, GL_RGBA, GL_UNSIGNED_BYTE, finalTex);
#endif

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_surfaceWidth, m_surfaceHeight);

	return 0;
}

GLuint ShadowRenderer::addTexture(GLuint tex, GLfloat aspect, GLfloat texWidth, GLfloat texHeight, bool shareShadow, GLuint shareWith) {
	TexInfo texInfo;
	if(shareShadow) {
		if(m_textures.find(shareWith) == m_textures.end()) {
			std::cout << "Can't find Texture " << shareWith << std::endl;
			return 0;
		}
		texInfo = m_textures[shareWith];
		texInfo._alphaTex = tex;
		texInfo._alphaCoord[0] = vec2(0.0, 0.0);
		texInfo._alphaCoord[1] = vec2(texWidth, 0.0);
		texInfo._alphaCoord[2] = vec2(0.0, texHeight);
		texInfo._alphaCoord[3] = vec2(texWidth, texHeight);
		m_textures.insert(make_pair(texInfo._alphaTex, texInfo));
	}
	else {
		texInfo._alphaTex = tex;
		GLfloat ratio;
		if(aspect > 1.0) {
			ratio = 1.0 / aspect;
			texInfo._hardShadowCoord[0] = vec2(-sampleHardShadowBound, -sampleHardShadowBound * ratio);
			texInfo._hardShadowCoord[1] = vec2( sampleHardShadowBound, -sampleHardShadowBound * ratio);
			texInfo._hardShadowCoord[2] = vec2(-sampleHardShadowBound,  sampleHardShadowBound * ratio);
			texInfo._hardShadowCoord[3] = vec2( sampleHardShadowBound,  sampleHardShadowBound * ratio);
		} else {
			ratio = aspect;
			texInfo._hardShadowCoord[0] = vec2(-sampleHardShadowBound * ratio, -sampleHardShadowBound);
			texInfo._hardShadowCoord[1] = vec2( sampleHardShadowBound * ratio, -sampleHardShadowBound);
			texInfo._hardShadowCoord[2] = vec2(-sampleHardShadowBound * ratio,  sampleHardShadowBound);
			texInfo._hardShadowCoord[3] = vec2( sampleHardShadowBound * ratio,  sampleHardShadowBound);
		}

		texInfo._alphaCoord[0] = vec2(0.0, 0.0);
		texInfo._alphaCoord[1] = vec2(texWidth, 0.0);
		texInfo._alphaCoord[2] = vec2(0.0, texHeight);
		texInfo._alphaCoord[3] = vec2(texWidth, texHeight);
	}

	texInfo._texQuadLength = texInfo._hardShadowCoord[3] - texInfo._hardShadowCoord[0];

	genShadowTexture(texInfo);
	m_textures.insert(make_pair(texInfo._alphaTex, texInfo));
	std::cout << "Blurred texture " << m_textures[texInfo._alphaTex]._shadowTex << " is generated for texture " << texInfo._alphaTex << std::endl;
	return tex;
}

int ShadowRenderer::bindTexture(GLuint objectId, GLuint texture) {
	if(objectId >= m_objects.size() || objectId < 0) {
		std::cout << "Can't find the object." << std::endl;
		return 1;
	}

	if(m_textures.find(texture) == m_textures.end()) {
		std::cout << "Can't find Texture " << texture << "." << std::endl;
		return 1;
	}

	m_objects[objectId]->setTexture(m_textures[texture], m_textures[texture]);
	std::cout << "Object " << objectId << " is now bound with texture " << texture
			<< "(blurred texture " << m_textures[texture]._shadowTex << ")" << std::endl;
	return 0;
}

int ShadowRenderer::bindTexture(GLuint objectId, GLuint frontTexture, GLuint backTexture) {
	if(objectId >= m_objects.size() || objectId < 0) {
		std::cout << "Can't find the object." << std::endl;
		return 1;
	}

	if(m_textures.find(frontTexture) == m_textures.end()) {
		std::cout << "Can't find Texture " << frontTexture << "." << std::endl;
		return 1;
	}

	if(m_textures.find(backTexture) == m_textures.end()) {
		std::cout << "Can't find Texture " << backTexture << "." << std::endl;
		return 1;
	}

	if(m_textures[frontTexture]._shadowTex != m_textures[backTexture]._shadowTex) {
		std::cout << "Warning: Texture " << frontTexture << " and Texture " << backTexture
				<< " don't share the same shadow, discard binding with back texture." << std::endl;

		m_objects[objectId]->setTexture(m_textures[frontTexture], m_textures[frontTexture]);
		std::cout << "Object " << objectId << " is now bound with texture " << frontTexture
				<< "(blurred texture " << m_textures[frontTexture]._shadowTex << ")" << std::endl;
	} else {
		m_objects[objectId]->setTexture(m_textures[frontTexture], m_textures[backTexture]);
		std::cout << "Object " << objectId << " is now bound with front texture " << frontTexture << " and back texture " << backTexture
				<< "(blurred texture " << m_textures[frontTexture]._shadowTex << ")" << std::endl;
	}
	return 0;
}


void ShadowRenderer::drawGroundPlane() {
	glBindTexture(GL_TEXTURE_2D, 0);
	glEnableVertexAttribArray(positionLoc);
	glDisableVertexAttribArray(colorLoc);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), m_groundPlane);
	glVertexAttrib4f(colorLoc, 1.0, 1.0, 1.0, 1.0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableVertexAttribArray(positionLoc);
}

void ShadowRenderer::renderDepth() {

	glEnable(GL_DEPTH_TEST);
#if USE_TWO_FBO
	glBindFramebuffer(GL_FRAMEBUFFER, m_depthFbo);
#else
	glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFbo);
#endif
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_depthTex, 0);
	glDepthMask(true);
	glColorMask(true, true, true, true);
	glClearColor(1.0,1.0,0.0,0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(s_depthShader2);

	glUniform3fv(glGetUniformLocation(s_depthShader2, "u_normal"), 1, vec3(0.0, 0.0, 1.0));
	drawGroundPlane();

	for(size_t i = 0; i < m_objects.size(); i++) {
		if(!m_objects[i]->isTransparent() && m_objects[i]->isVisible()) {
			glBindTexture(GL_TEXTURE_2D, m_objects[i]->getTexture()._alphaTex);
			glUniform3fv(glGetUniformLocation(s_depthShader2, "u_normal"), 1, m_objects[i]->getNormal());
			m_objects[i]->drawQuad(positionLoc, alphaLoc, colorLoc);
		}
	}

	glUseProgram(s_depthShader);

	for(size_t i = 0; i < m_objects.size(); i++) {
		if(m_objects[i]->isTransparent() && m_objects[i]->isVisible()) {
			glBindTexture(GL_TEXTURE_2D, m_objects[i]->getTexture()._alphaTex);
			glUniform3fv(glGetUniformLocation(s_depthShader, "u_normal"), 1, m_objects[i]->getNormal());
			m_objects[i]->drawQuad(positionLoc, alphaLoc, colorLoc);
		}
	}

#if USE_TWO_FBO
    // depth test for the second pass
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFbo);
	glDepthMask(true);
    glClear(GL_DEPTH_BUFFER_BIT);

    glColorMask(false, false, false, false);

	glUseProgram(s_depthShader2);
	drawGroundPlane();

	for(size_t i = 0; i < m_objects.size(); i++) {
		if(!m_objects[i]->isTransparent() && m_objects[i]->isVisible()) {
			glBindTexture(GL_TEXTURE_2D, m_objects[i]->getTexture()._alphaTex);
			m_objects[i]->drawQuad(positionLoc, alphaLoc, colorLoc);
		}
	}

	glUseProgram(s_depthShader);

	for(size_t i = 0; i < m_objects.size(); i++) {
		if(m_objects[i]->isTransparent() && m_objects[i]->isVisible()) {
			glBindTexture(GL_TEXTURE_2D, m_objects[i]->getTexture()._alphaTex);
			m_objects[i]->drawQuad(positionLoc, alphaLoc, colorLoc);
		}
	}
#endif

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glColorMask(true, true, true, true);
}

void ShadowRenderer::renderShadow() {

	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFbo);
#if !USE_TWO_FBO
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_shadowTex, 0);
#endif
	glColorMask(true, true, true, true);
	glClearColor(0.0,0.0,0.0,0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDepthMask(false);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	glUseProgram(s_shadowShader);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_depthTex);

	glActiveTexture(GL_TEXTURE0);

	glUniform4fv(glGetUniformLocation(s_shadowShader, "light"), 1, vec4(m_lightPos, m_lightRadius));

	for(size_t i = 0; i < m_objects.size(); i++) {
		if(m_objects[i]->isVisible()) {
			glBindTexture(GL_TEXTURE_2D, m_objects[i]->getTexture()._shadowTex);
			m_objects[i]->drawShadow(s_shadowShader, positionLoc, alphaLoc, colorLoc);
		}
	}

	glDisable(GL_BLEND);
	glDepthMask(true);
}

void ShadowRenderer::renderScene() {

	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFbo);
//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDepthMask(true);
	glColorMask(true, true, true, true);
	glClearColor(1.0,1.0,1.0,1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_shadowTex);
//	glBindTexture(GL_TEXTURE_2D, m_depthTex);
	glActiveTexture(GL_TEXTURE0);

	glUseProgram(s_defaultSceneShader);
	glUniform3fv(glGetUniformLocation(s_defaultSceneShader, "u_lightPos"), 1, m_lightPos);

	glUniform3fv(glGetUniformLocation(s_defaultSceneShader, "u_normal"), 1, vec3(0.0, 0.0, 1.0));
	drawGroundPlane();

//	 Opaque objects
	for(size_t i = 0; i < m_objects.size(); i++) {
		if((!m_objects[i]->isTransparent()) && m_objects[i]->isVisible()) {
			GLuint texture =  m_objects[i]->getTexture()._alphaTex;
			if(texture == 0) {
				glUseProgram(s_defaultSceneShader);
				glUniform3fv(glGetUniformLocation(s_defaultSceneShader, "u_normal"), 1, m_objects[i]->getNormal());
			} else {
				glUseProgram(s_sceneShader);
				glUniform3fv(glGetUniformLocation(s_sceneShader, "u_normal"), 1, m_objects[i]->getNormal());
			}
			glBindTexture(GL_TEXTURE_2D, texture);
			m_objects[i]->drawQuad(positionLoc, alphaLoc, colorLoc);
		}
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Transparent objects
	for(size_t i = 0; i < m_objects.size(); i++) {
		if(m_objects[i]->isTransparent() && m_objects[i]->isVisible()) {
			GLuint texture =  m_objects[i]->getTexture()._alphaTex;
			if(texture == 0) {
				glUseProgram(s_defaultSceneShader);
				glUniform3fv(glGetUniformLocation(s_defaultSceneShader, "u_normal"), 1, m_objects[i]->getNormal());
				glUniform3fv(glGetUniformLocation(s_defaultSceneShader, "u_lightPos"), 1, m_lightPos);
			} else {
				glUseProgram(s_sceneShader);
				glUniform3fv(glGetUniformLocation(s_sceneShader, "u_normal"), 1, m_objects[i]->getNormal());
				glUniform3fv(glGetUniformLocation(s_sceneShader, "u_lightPos"), 1, m_lightPos);
			}
			glBindTexture(GL_TEXTURE_2D, texture);
			m_objects[i]->drawQuad(positionLoc, alphaLoc, colorLoc);
		}
	}

	glDisable(GL_BLEND);

	glDisable(GL_DEPTH_TEST);
	glUseProgram(s_defaultSceneShader);
//	m_objects[1]->drawHardQuad(positionLoc, colorLoc);
//	for(size_t i = 0; i < m_objects.size(); i++) {
//		if(m_objects[i]->isVisible())
//			m_objects[i]->drawOuterQuad(positionLoc, colorLoc);
//	}
	glEnable(GL_DEPTH_TEST);
}

bool sortRenderProperty (const RenderProperty& r1, const RenderProperty& r2) {
	return r1._depth < r2._depth;
}

void ShadowRenderer::render() {
	for(GLuint i = 0; i < m_objects.size(); i++) {
		if(!m_objects[i]->isPosUpdated() && m_objects[i]->isVisible()) {
			m_objectsUpdated = false;
			m_sceneUpdated = false;
			break;
		}
	}


	if(!m_objectsUpdated) {
		for(GLuint i = 0; i < m_objects.size(); i++)
			m_objects[i]->updateShadowBound();
//		for(GLuint i = 0; i < m_renderProperty.size(); i++) {
//			m_renderProperty[i]._objectId = i;
//			m_renderProperty[i]._depth = m_objects[i]->center().z;
//			m_renderProperty[i]._transparent = m_objects[i]->isTransparent();
//		}
////		std::sort(m_renderProperty.begin(), m_renderProperty.end(), sortRenderProperty);
//		for(GLuint i = 0; i < m_renderProperty.size(); i++) {
//			std::cout << i << " object is " << m_renderProperty[i]._objectId
//					<< ", depth = " << m_renderProperty[i]._depth
//					<< ", transparent = " << m_renderProperty[i]._transparent << std::endl;
//		}
	}

	if(!m_sceneUpdated) {
		if(!m_objectsUpdated) {
//			std::cout << "render depth..." << std::endl;
			renderDepth();
//			m_objectsUpdated = true;
		}
		renderShadow();
//		std::cout << "render scene..." << std::endl;
		renderScene();
//		m_sceneUpdated = true;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDepthMask(true);
	glColorMask(true, true, true, true);
	glClearColor(1.0,1.0,1.0,1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_sceneTex);
	glActiveTexture(GL_TEXTURE0);
	glUseProgram(s_copyShader);

//	glActiveTexture(GL_TEXTURE1);
//	glBindTexture(GL_TEXTURE_2D, m_objects[0]->getTexture()._shadowTex);
//	glActiveTexture(GL_TEXTURE0);
//	glUseProgram(s_defaultSceneShader);
//	static bool add = true;
//	static float bias = 0.0;
//	if(add) {
//		bias += 0.3;
//		if(bias >= 216.0) add = false;
//	} else {
//		bias -= 0.3;
//		if(bias <= 0.0) add = true;
//	}
//	glUniform1f(glGetUniformLocation(s_defaultSceneShader, "u_bias"), bias);

	drawGroundPlane();
}
