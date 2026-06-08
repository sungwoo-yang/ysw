#pragma once
/*
==========================================================================
  Effects 사용 가이드  (OpenGL 3.3 core / GLSL 330)
==========================================================================

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  1. FOG  (fog.frag + fullscreen.vert)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  렌더 순서:
    1) glBindFramebuffer → sceneFbo  (color + depth 텍스처 붙임)
    2) 씬 렌더
    3) glBindFramebuffer → 0 (화면)
    4) glUseProgram(progFog)
       uniform sampler2D u_scene  → sceneFbo 컬러 텍스처
       uniform sampler2D u_depth  → sceneFbo 뎁스 텍스처
       uniform vec3  u_fog_color    = {0.6, 0.7, 0.8}
       uniform float u_fog_density  = 0.05f
       uniform float u_fog_start    = 10.0f  (LINEAR 모드만)
       uniform float u_fog_end      = 100.0f (LINEAR 모드만)
       uniform float u_near         = 0.1f
       uniform float u_far          = 1000.0f
       uniform int   u_fog_mode     = 0  (0=EXP2, 1=EXP, 2=LINEAR)
    5) glDrawArrays(GL_TRIANGLES, 0, 3)

  FBO 생성 시 depth 텍스처:
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ...);

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  2. LASER  (laser.vert + laser_core.frag + laser_glow.frag)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  쿼드 빌보드 생성 (C++ 의사코드):
    vec2 start, end;        // 레이저 시작/끝 월드 좌표
    vec2 dir   = normalize(end - start);
    vec2 perp  = {-dir.y, dir.x};  // 수직
    float half_w = laserWidth * 0.5f;

    vertex[0] = {start + perp * half_w, {0, 0}};
    vertex[1] = {start - perp * half_w, {0, 1}};
    vertex[2] = {end   + perp * half_w, {1, 0}};
    vertex[3] = {end   - perp * half_w, {1, 1}};
    // UV.x = 0→1 (레이저 방향), UV.y = 0/1 (가장자리) → 셰이더 내부에서 0.5 중앙

  렌더 순서:
    // Pass 1: 코어 (가산)
    glBlendFunc(GL_ONE, GL_ONE);
    glUseProgram(progLaserCore);
    uniform vec4  u_color         = {1.0, 0.3, 0.1, 3.0}  // RGB + 강도
    uniform float u_time          = elapsed;
    uniform float u_core_width    = 0.08f
    uniform float u_noise_strength= 0.15f
    glDrawElements(...)

    // Pass 2: 글로우 (소프트 발광)
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
    glUseProgram(progLaserGlow);
    uniform vec4  u_glow_color    = {1.0, 0.5, 0.2, 0.8}
    uniform float u_glow_radius   = 0.6f
    uniform float u_glow_falloff  = 2.0f
    glDrawElements(...)

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // 블렌딩 복구

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  3. WATER  (water.vert + water.frag)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  렌더 순서:
    1) 씬을 sceneFbo에 렌더  (water 제외)
    2) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    3) glUseProgram(progWater)
       uniform sampler2D u_grab_tex    → sceneFbo 컬러 텍스처 (굴절)
       uniform sampler2D u_normal_map  → 타일링 노멀맵 텍스처
       uniform sampler2D u_foam_tex    → 폼 텍스처
       uniform vec4  u_refr_color      = {0.34, 0.85, 0.92, 1}
       uniform vec4  u_refl_color      = {0.34, 0.85, 0.92, 1}
       uniform float u_refl_strength   = 1.7f
       uniform float u_wave_scale      = 0.5f
       uniform vec4  u_wave_speed      = {19, 9, -16, -7}  // 원본 Ori 값
       uniform float u_distort         = 3.0f
       uniform float u_foam_strength   = 100.0f
       uniform float u_specular_power  = 2.0f
       uniform float u_specular_str    = 1.5f
       uniform vec4  u_specular_color  = {0.6, 0.6, 0.8, 1}
       uniform float u_time            = elapsed
       uniform vec3  u_light_dir       = normalize({0.5, 1, 0})
       uniform vec3  u_view_dir        = {0, 0, 1}  // 2D라면 항상 앞
    4) 수면 쿼드 렌더

  노멀맵 권장: tileable 512×512, 파란 계열 노멀맵
  폼 텍스처 권장: tileable 256×256 흑백, 흰색 = 폼

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  4. GOD RAYS  (god_rays.frag + fullscreen.vert)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  렌더 순서:
    1) occluderFbo 렌더
       - 하늘/배경 빛 영역 → 흰색(1,1,1)
       - 오브젝트(캐릭터, 지형) → 검정(0,0,0)
       - 해상도 작게 해도 됨 (씬의 1/2 또는 1/4)

    2) god_rays 패스 (raysTexture 생성)
       glBindFramebuffer → raysFbo
       glUseProgram(progGodRays)
       uniform sampler2D u_occlusion  → occluderFbo 텍스처
       uniform vec2  u_light_pos      → 빛 위치 (화면 UV, 예: {0.5, 0.8})
       uniform int   u_num_samples    = 64
       uniform float u_density        = 0.8f
       uniform float u_weight         = 0.01f
       uniform float u_decay          = 0.975f
       uniform float u_exposure       = 0.8f
       glDrawArrays(GL_TRIANGLES, 0, 3)

    3) 씬 위에 가산 합성
       glBindFramebuffer → 0
       glBlendFunc(GL_ONE, GL_ONE)
       // raysTexture를 화면에 blit (간단한 텍스처 쿼드 or SeinPostProcessing composite에 포함)
       glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)  // 복구

  빛 위치 → 스크린 UV 변환 (C++):
    vec4 clip = projection * view * vec4(lightWorldPos, 1.0);
    vec2 ndc  = vec2(clip.x, clip.y) / clip.w;
    vec2 uv   = ndc * 0.5 + 0.5;   // u_light_pos 에 넘겨줌

==========================================================================
*/
