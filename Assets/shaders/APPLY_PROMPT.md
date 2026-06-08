# 셰이더 적용 프롬프트

아래 내용을 그대로 Claude / Cursor 에 붙여넣어 사용하세요.

---

```
나는 C++/OpenGL(GLSL 330 core) 2D 게임을 만들고 있다.
아래에 이미 작성된 셰이더 파일 목록과 각각의 역할, 필요한 uniform이 정리되어 있다.
이것들을 내 게임 렌더 루프에 연결하는 C++ 코드를 작성해줘.

──────────────────────────────────────────────────────
## 셰이더 파일 목록
──────────────────────────────────────────────────────

### 공용 버텍스
- fullscreen.vert
  VAO 없이 glDrawArrays(GL_TRIANGLES, 0, 3) 으로 화면 전체 삼각형 그림.
  out vec2 v_uv (0~1 UV)

### [포스트 프로세싱] Bloom 파이프라인
- bright_pass.frag   (fullscreen.vert 사용)
  uniform sampler2D u_scene, float u_threshold
  → 밝은 영역만 추출

- gaussian_blur.frag (fullscreen.vert 사용)
  uniform sampler2D u_tex, vec2 u_texel_size, vec2 u_direction
  → u_direction=(1,0) H패스, (0,1) V패스 9-tap 가우시안

- upsample.frag      (fullscreen.vert 사용)
  uniform sampler2D u_tex, vec2 u_texel_size, float u_radius
  → 텐트 필터 업샘플. 반드시 glBlendFunc(GL_ONE, GL_ONE) 상태에서 사용

- composite.frag     (fullscreen.vert 사용)
  uniform sampler2D u_scene, u_bloom, u_grain
  uniform float u_bloom_intensity
  uniform float u_contrast, u_brightness
  uniform vec4  u_bezier_r, u_bezier_g, u_bezier_b
  uniform float u_desat
  uniform vec4  u_grain_offset_scale
  uniform float u_blur_strength
  uniform vec2  u_blur_dir
  uniform float u_twirl_angle
  uniform vec4  u_twirl_center_radius
  → Bloom + 색보정(베지어) + 채도 + 필름 그레인 + 모션블러 + 트윌 최종 합성

### [포스트 프로세싱] 안개
- fog.frag           (fullscreen.vert 사용)
  uniform sampler2D u_scene, u_depth
  uniform vec3  u_fog_color
  uniform float u_fog_density
  uniform float u_fog_start, u_fog_end   (LINEAR 모드용)
  uniform float u_near, u_far
  uniform int   u_fog_mode   (0=EXP2, 1=EXP, 2=LINEAR)

### [포스트 프로세싱] 빛줄기(God Rays)
- god_rays.frag      (fullscreen.vert 사용)
  uniform sampler2D u_occlusion
  uniform vec2  u_light_pos   (스크린 UV 0~1)
  uniform int   u_num_samples
  uniform float u_density, u_weight, u_decay, u_exposure

### [월드 오브젝트] 레이저  (2-pass)
- laser.vert
  in vec3 a_pos, in vec2 a_uv
  uniform mat4 u_mvp
  → 레이저 방향으로 늘린 빌보드 쿼드

- laser_core.frag    (laser.vert 사용)
  uniform sampler2D u_noise_tex
  uniform vec4 u_color, float u_time
  uniform float u_core_width, u_noise_strength
  → Pass1: glBlendFunc(GL_ONE, GL_ONE)

- laser_glow.frag    (laser.vert 사용)
  uniform vec4 u_glow_color
  uniform float u_glow_radius, u_glow_falloff
  → Pass2: glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE)

### [월드 오브젝트] 물 표면
- water.vert
  in vec3 a_pos, in vec2 a_uv
  uniform mat4 u_mvp, u_model

- water.frag         (water.vert 사용)
  uniform sampler2D u_grab_tex    (씬 FBO 컬러, 굴절용)
  uniform sampler2D u_normal_map  (타일링 노멀맵)
  uniform sampler2D u_foam_tex
  uniform vec4  u_refr_color      = (0.34, 0.85, 0.92, 1)
  uniform vec4  u_refl_color      = (0.34, 0.85, 0.92, 1)
  uniform float u_refl_strength   = 1.7
  uniform float u_wave_scale      = 0.5
  uniform vec4  u_wave_speed      = (19, 9, -16, -7)
  uniform float u_distort         = 3.0
  uniform float u_foam_strength   = 100.0
  uniform float u_specular_power  = 2.0
  uniform float u_specular_str    = 1.5
  uniform vec4  u_specular_color  = (0.6, 0.6, 0.8, 1)
  uniform float u_time
  uniform vec3  u_light_dir, u_view_dir
  → glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), alpha=0.85

──────────────────────────────────────────────────────
## 필요한 FBO 목록
──────────────────────────────────────────────────────

아래 FBO들을 게임 초기화 시 생성해줘.
FBO는 FboTarget 구조체(create/destroy/bind 메서드 있음)로 관리한다고 가정한다.

1. sceneFbo       (GL_RGB16F 컬러 + GL_DEPTH_COMPONENT24 뎁스)
   → 씬 전체 렌더 대상. 해상도 = 화면 해상도

2. occluderFbo    (GL_RGB8 컬러만)
   → God Rays용 오클루더 마스크. 해상도 = 화면의 1/2

3. raysFbo        (GL_RGB16F 컬러만)
   → God Rays 결과 저장. 해상도 = 화면의 1/2

4. bloomPyramid[6] (GL_RGB16F, 각각 해상도 절반씩)
   bloomPong[6]    (bloom H/V 핑퐁용 임시)
   → Bloom 다운샘플 피라미드

5. waterGrabFbo   (GL_RGB8 컬러만)
   → 물 굴절용 씬 스냅샷. 해상도 = 화면 해상도

──────────────────────────────────────────────────────
## 한 프레임의 렌더 순서 (전체 파이프라인)
──────────────────────────────────────────────────────

다음 순서로 매 프레임 렌더하는 C++ 함수 RenderFrame() 을 작성해줘.

[Step 1] sceneFbo 에 씬 렌더 (물 오브젝트 제외)
  - glBindFramebuffer(sceneFbo)
  - 배경, 캐릭터, 지형 등 일반 오브젝트 렌더
  - 뎁스 버퍼도 여기에 씀 (fog에서 읽음)

[Step 2] waterGrabFbo 에 씬 스냅샷 복사
  - sceneFbo 컬러 → waterGrabFbo 로 blit
  - (물이 씬을 굴절시키려면 물 렌더 전에 씬을 따로 저장해야 함)

[Step 3] sceneFbo 에 물 렌더
  - u_grab_tex = waterGrabFbo.tex
  - water.vert + water.frag 로 수면 쿼드 렌더
  - glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

[Step 4] sceneFbo 에 레이저 렌더 (2-pass)
  - Pass1: laser_core.frag, glBlendFunc(GL_ONE, GL_ONE)
  - Pass2: laser_glow.frag, glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE)
  - 렌더 후 glBlendFunc 원래대로 복구

[Step 5] occluderFbo 에 오클루더 마스크 렌더
  - 하늘/빛 영역 → 흰색 (1,1,1)
  - 지형/오브젝트 → 검정 (0,0,0)
  - 해상도 1/2 이므로 glViewport 조정 필요

[Step 6] God Rays 생성 → raysFbo
  - fullscreen.vert + god_rays.frag
  - u_occlusion = occluderFbo.tex
  - u_light_pos = 빛 월드좌표를 스크린 UV로 변환해서 넘김
    변환: vec4 clip = proj*view*vec4(lightPos,1); uv = clip.xy/clip.w*0.5+0.5

[Step 7] Bloom 파이프라인
  - BrightPass:  sceneFbo.colorTex → bloomPyramid[0]
  - 다운샘플 루프 (i=0 ~ bloomIterations-1):
      H blur: bloomPyramid[i] → bloomPong[i]
      V blur: bloomPong[i]    → bloomPyramid[i]
      다음 레벨로 복사: bloomPyramid[i] → bloomPyramid[i+1] (해상도 절반)
  - 업샘플 루프 (역순, glBlendFunc ONE ONE):
      bloomPyramid[i] → bloomPyramid[i-1] (누적 가산)

[Step 8] 안개 패스 → fogFbo (또는 별도 RT)
  - fullscreen.vert + fog.frag
  - u_scene = sceneFbo.colorTex
  - u_depth = sceneFbo.depthTex

[Step 9] 최종 Composite → 화면 (FBO 0)
  - fullscreen.vert + composite.frag
  - u_scene = fogFbo.colorTex  (안개 적용된 씬)
  - u_bloom = bloomPyramid[0].tex
  - u_grain = grainTex (CPU에서 생성한 노이즈 텍스처)
  - God Rays 합성: composite 전 또는 후에
      glBlendFunc(GL_ONE, GL_ONE)
      raysFbo.tex 를 화면에 blit (간단한 additive quad)

──────────────────────────────────────────────────────
## 요청 사항
──────────────────────────────────────────────────────

위 파이프라인을 바탕으로 다음을 작성해줘:

1. 셰이더 파일 로드 + 컴파일 + 링크 헬퍼 함수
   GLuint load_shader_program(const char* vert_path, const char* frag_path)

2. FBO 생성 함수 (컬러만 / 컬러+뎁스 두 버전)

3. RenderFrame() 함수 (Step 1~9 전체 흐름)
   - 각 Step 앞에 한국어 주석으로 단계 설명
   - uniform 설정 코드 포함
   - glViewport 변경 필요한 곳 명시

4. 창 크기 변경 시 FBO 재생성하는 OnResize(int w, int h) 함수

5. 게임 루프에서 호출하는 순서 예시 (main loop 뼈대)

언어: C++17, OpenGL 3.3 core, glad 사용 가정.
FBO 구조체는 아래처럼 이미 있다고 가정:

struct FboTarget {
    GLuint fbo = 0, tex = 0;
    int w = 0, h = 0;
    void create(int w, int h, GLenum internalFormat, bool withDepth);
    void destroy();
    void bind();
    GLuint depthTex = 0;  // withDepth=true 일 때만 유효
};
```
