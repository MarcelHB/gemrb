#include "Matrix.h"
#include "OpenGLEnv.h"
#include "SDL20GLVideoN.h"
#include "GLTextureSprite2D.h"

using namespace GemRB;

// C++11+ todos:
// * NULL -> nullptr
// * &(vector[0]) -> vector.data()
// * Uint32 -> uint32_t
// * constructor params
// * typename in iterators -> auto
// * enum -> enum class

size_t GLRenderBuffer::lastId = 0;

GLRenderBuffer::GLRenderBuffer(
  const Region& r,
  const Region& parentRegion,
  Video::BufferFormat format,
  GLSLProgram& program
) :
  VideoBuffer(r),
  id(lastId++),
  shotted(false),
  spriteProgram(program),
  backBuffer(r.h * r.w, 0),
  format(format),
  glTexture(0),
  matrix(
    2.0f/r.w,     0.0f, 0.0f,  0.0f,
        0.0f, -2.0f/r.h, 0.0f,  0.0f,
        0.0f,     0.0f, 1.0f,  0.0f,
       -1.0f,     1.0f, 0.0f,  1.0f
  ),
  parentRegion(parentRegion),
  state(NEW)
{ }

GLRenderBuffer::~GLRenderBuffer() {
  if (state == TEXTURIZED) {
    glDeleteTextures(1, &glTexture);
  }
}

#include <fstream>
#include <sstream>

void GLRenderBuffer::Backup() {
  if (state == ACTIVE) {
#ifdef USE_GL
    glReadBuffer(GL_BACK);
#endif
    glReadPixels(0, 0, rect.w, rect.h, GL_RGBA, GL_UNSIGNED_BYTE, &(backBuffer[0]));

  /*if (!shotted) {
    std::stringstream fname;
    fname << "backup_" << id << ".ppm";

    std::stringstream header;
    header << "P6\n" << rect.w << " " << rect.h << "\n255\n";
    std::ofstream fs(fname.str(), std::ios_base::binary);
    fs << header.str();
    size_t dim = rect.w * rect.h;
    for (size_t i = 0; i < dim; ++i) {
      fs.write(((char*)&(backBuffer[i])), 3);
    }
    fs.close();
    this->shotted = true;
  }*/

    this->state = BACKUP;
  }
}

void GLRenderBuffer::Clear() {
  if (state == TEXTURIZED) {
    glDeleteTextures(1, &glTexture);
  }

  this->state = NEW;
}

void GLRenderBuffer::CopyPixels(const Region&, const void*, const int*, ...) {
  // TODO: mostly relevant for movie players
  if (state != ACTIVE) {
    assert(false);
  }
}

void GLRenderBuffer::CreateTexture() {
  glGenTextures(1, &glTexture);
  glBindTexture(GL_TEXTURE_2D, glTexture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
#ifdef USE_GL
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA,
    rect.w,
    rect.h,
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    (GLvoid*) &(backBuffer[0])
  );

  this->state = TEXTURIZED;
}

void GLRenderBuffer::Draw() const {
  spriteProgram.Use();

  glm::vec4 coordinates(
    rect.x * 1.0f,
    rect.y * 1.0f,
    (rect.x + rect.w) * 1.0f,
    (rect.y + rect.h) * 1.0f
  );

  GLfloat textureCoords[] = {
    0.0f, 1.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
  };

  GLfloat data[] = {
    coordinates[0], coordinates[1], textureCoords[0], textureCoords[1],
    coordinates[2], coordinates[1], textureCoords[2], textureCoords[3],
    coordinates[0], coordinates[3], textureCoords[4], textureCoords[5],
    coordinates[2], coordinates[3], textureCoords[6], textureCoords[7]
  };

  glm::mat4 matrix(
    2.0f/parentRegion.w,     0.0f, 0.0f,  0.0f,
        0.0f, -2.0f/parentRegion.h, 0.0f,  0.0f,
        0.0f,     0.0f,      1.0f,  0.0f,
       -1.0f,     1.0f,      0.0f,  1.0f
  );

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, glTexture);

  spriteProgram.SetUniformMatrixValue("u_matrix", 4, 1, &(matrix[0][0]));

  spriteProgram.SetUniformValue("u_alphaModifier", 1, 1.0f);
  spriteProgram.SetUniformValue("u_tint", 4, 1.0f, 1.0f, 1.0f, 1.0f);
  GLint a_position = spriteProgram.GetAttribLocation("a_position");
  GLint a_texCoord = spriteProgram.GetAttribLocation("a_texCoord");

  GLuint buffer;
  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

  glVertexAttribPointer(a_position, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
  glVertexAttribPointer(a_texCoord, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), reinterpret_cast<void*>(2 * sizeof(GLfloat)));

  glEnableVertexAttribArray(a_position);
  glEnableVertexAttribArray(a_texCoord);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDisableVertexAttribArray(a_texCoord);
  glDisableVertexAttribArray(a_position);
  glDeleteBuffers(1, &buffer);
}

void GLRenderBuffer::Enable() {
  if (state != ACTIVE) {
    glViewport(0, 0, rect.w, rect.h);

    this->state = ACTIVE;
  }
}

const glm::mat4& GLRenderBuffer::Matrix() const {
  return matrix;
}

/* Needed because RenderOnDisplay is const. */
void GLRenderBuffer::PrepareToRender() {
  if (state == ACTIVE) {
    Backup();
  }

  if (state == BACKUP) {
    CreateTexture();
  }
}

bool GLRenderBuffer::RenderOnDisplay(void*) const {
  if (state == TEXTURIZED) {
    Draw();

    return true;
  }

  return false;
}

void GLRenderBuffer::Reuse() {
  /* `backBuffer` is either black or contains the last bitmap. */
  if (state == NEW || state == ACTIVE) {
    this->state = BACKUP;
  }
}

GLVideoDriver::GLVideoDriver() :
  window(NULL),
  context(NULL),
  renderer(NULL),
  shaderPrograms(NUM_SHADER_PROGRAMS, NULL)
{}

GLVideoDriver::~GLVideoDriver() {
  SDL_DestroyRenderer(renderer);
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);

  for (std::vector<GLSLProgram*>::iterator it = shaderPrograms.begin(); it != shaderPrograms.end(); ++it) {
    if (*it != NULL) {
      (*it)->Release();
    }
  }
}

void GLVideoDriver::BlitSpriteBAMClipped(
  const Sprite2D*,
  const Sprite2D*,
  const Region&,
  const Region&,
  unsigned int,
  const Color*
) { /* Rumors say no here. */ }

void GLVideoDriver::BlitSpriteNativeClipped(
  const Sprite2D *sprite,
  const Sprite2D *mask,
  const SDL_Rect& src,
  const SDL_Rect& dest,
  unsigned int flags,
  const SDL_Color *tint
) {
  Color colorTint(255, 255, 255, 255);
  if (tint != NULL) {
    colorTint.r = tint->r;
    colorTint.g = tint->g;
    colorTint.b = tint->b;
    colorTint.a = tint->a;
  }

  GLfloat alphaModifier = flags & BLIT_HALFTRANS ? 0.5f : 1.0f;

  float spriteDimensions[] = { sprite->Width * 1.0f, sprite->Height * 1.0f };

  GLfloat x = src.x/spriteDimensions[0];
  GLfloat y = src.y/spriteDimensions[1];
  GLfloat w = src.w/spriteDimensions[0];
  GLfloat h = src.h/spriteDimensions[1];

  GLfloat textureCoords[] = {
    x,     y,
    x + w, y,
    x,     y + h,
    x + w, y + h,
  };

  GLfloat tmp;
  if (flags & BLIT_MIRRORX) {
    tmp = textureCoords[0];
    textureCoords[0] = textureCoords[2];
    textureCoords[2] = tmp;
    tmp = textureCoords[4];
    textureCoords[4] = textureCoords[6];
    textureCoords[6] = tmp;
  }

  if (flags & BLIT_MIRRORY) {
    tmp = textureCoords[1];
    textureCoords[1] = textureCoords[5];
    textureCoords[5] = tmp;
    tmp = textureCoords[3];
    textureCoords[3] = textureCoords[7];
    textureCoords[7] = tmp;
  }

  assert(drawingBuffer);
  glm::vec4 coordinates(
    dest.x * 1.0f,
    dest.y * 1.0f,
    (dest.x + dest.w) * 1.0f,
    (dest.y + dest.h) * 1.0f
  );

  GLfloat data[] = {
    coordinates[0], coordinates[1], textureCoords[0], textureCoords[1],
    coordinates[2], coordinates[1], textureCoords[2], textureCoords[3],
    coordinates[0], coordinates[3], textureCoords[4], textureCoords[5],
    coordinates[2], coordinates[3], textureCoords[6], textureCoords[7]
  };

  GLSLProgram* program = GetShaderProgram(SPRITE_RGBA);
  // `sprite` is const
  GLTextureSprite2D glSprite(static_cast<const GLTextureSprite2D&>(*sprite));

  if (glSprite.IsPaletted()) {
    if (flags & BLIT_GREY) {
      program = GetShaderProgram(SPRITE_PAL_GRAY);
    } else if (flags & BLIT_SEPIA) {
      program = GetShaderProgram(SPRITE_PAL_SEPIA);
    } else {
      program = GetShaderProgram(SPRITE_PAL);
    }

    glActiveTexture(GL_TEXTURE1);
    GLuint palTexture = glSprite.GetPaletteTexture();
    glBindTexture(GL_TEXTURE_2D, palTexture);
  }

  program->Use();

  GLRenderBuffer *glRenderBuffer = static_cast<GLRenderBuffer*>(drawingBuffer);
  program->SetUniformMatrixValue("u_matrix", 4, 1, &(glRenderBuffer->Matrix()[0][0]));

  glActiveTexture(GL_TEXTURE0);
  GLuint texture = glSprite.GetTexture();
  glBindTexture(GL_TEXTURE_2D, texture);

  if (mask) {
    glActiveTexture(GL_TEXTURE2);
    GLTextureSprite2D glMaskSprite = static_cast<const GLTextureSprite2D&>(*mask);

    GLuint maskTexture = glMaskSprite.GetMaskTexture();
    glBindTexture(GL_TEXTURE_2D, maskTexture);
  } else {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  program->SetUniformValue("u_tint", 4, colorTint.r/255.0f, colorTint.g/255.0f, colorTint.b/255.0f, colorTint.a/255.0f);
  program->SetUniformValue("u_alphaModifier", 1, alphaModifier);

  GLint shadowMode = 1;
  if (flags & BLIT_NOSHADOW) {
    shadowMode = 0;
  } else if (flags & BLIT_TRANSSHADOW) {
    shadowMode = 2;
  }
  program->SetUniformValue("u_shadowMode", 1, shadowMode);

  GLint a_position = program->GetAttribLocation("a_position");
  GLint a_texCoord = program->GetAttribLocation("a_texCoord");

  GLuint buffer;
  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

  glVertexAttribPointer(a_position, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
  glVertexAttribPointer(a_texCoord, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), reinterpret_cast<void*>(2 * sizeof(GLfloat)));

  glEnableVertexAttribArray(a_position);
  glEnableVertexAttribArray(a_texCoord);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDisableVertexAttribArray(a_texCoord);
  glDisableVertexAttribArray(a_position);

  glDeleteBuffers(1, &buffer);
}

int GLVideoDriver::CreateDriverDisplay(const Size& size, int, const char* title) {
  this->screenSize = size;

  Log(MESSAGE, "SDL 2 GL Driver", "Creating display");

  Uint32 windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;

#if TARGET_OS_IPHONE || ANDROID
  // SDL_WINDOW_RESIZABLE:
  //  this allows the user to flip the device upsidedown if they wish and have the game rotate.
  //  it also for some unknown reason is required for retina displays
  // SDL_WINDOW_BORDERLESS, SDL_WINDOW_FULLSCREEN:
  //  This is needed to remove the status bar on Android/iOS.
  //  since we are in fullscreen this has no effect outside Android/iOS
  windowFlags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;
  // this hint is set in the wrapper for iPad at a higher priority. set it here for iPhone
  // don't know if Android makes use of this.
  SDL_SetHintWithPriority(SDL_HINT_ORIENTATIONS, "LandscapeRight LandscapeLeft", SDL_HINT_DEFAULT);
#endif

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#ifndef USE_GL
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_EGL, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif

  this->window =
    SDL_CreateWindow(
      title,
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      screenSize.w,
      screenSize.h,
      windowFlags
    );

  if (window == NULL) {
    Log(ERROR, "SDL 2 GL Driver", "Failed to create window: %s", SDL_GetError());
    return GEM_ERROR;
  }

  this->context = SDL_GL_CreateContext(window);
  if (context == NULL) {
    Log(ERROR, "SDL 2 GL Driver", "Failed to create GL context: %s", SDL_GetError());
    return GEM_ERROR;
  }

  SDL_GL_MakeCurrent(window, context);
  this->renderer = SDL_CreateRenderer(window, -1, 0);

  if (renderer == NULL) {
    Log(ERROR, "SDL 2 GL Driver", "Failed to create renderer: %s", SDL_GetError());
    return GEM_ERROR;
  }

  SDL_RenderSetLogicalSize(renderer, screenSize.w, screenSize.h);
  SDL_GetRendererOutputSize(renderer, &(screenSize.w), &(screenSize.h));

#ifdef USE_GL
  glewInit();
#endif
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  SDL_StopTextInput();

  return GEM_OK;
}

Sprite2D* GLVideoDriver::CreateSprite(
  int w,
  int h,
  int bpp,
  ieDword rMask,
  ieDword gMask,
  ieDword bMask,
  ieDword aMask,
  void* pixels,
  bool hasColorKey,
  int index
) {
  GLTextureSprite2D* spr = new GLTextureSprite2D(w, h, bpp, pixels, rMask, gMask, bMask, aMask);

  if (hasColorKey) {
    spr->SetColorKey(index);
  }

  return spr;
}

Sprite2D* GLVideoDriver::CreatePalettedSprite(
  int w,
  int h,
  int bpp,
  void* pixels,
  Color* palette,
  bool hasColorKey,
  int index
) {
  if (palette == NULL) {
    return NULL;
  }

  GLTextureSprite2D* spr = new GLTextureSprite2D(w, h, bpp, pixels);
  spr->SetPaletteManager(&paletteManager);
  Palette* pal = new Palette(palette);
  spr->SetPalette(pal);
  pal->release();

  if (hasColorKey) {
    spr->SetColorKey(index);
  }

  return spr;
}

Sprite2D* GLVideoDriver::CreateSprite8(
  int w,
  int h,
  void* pixels,
  Palette*
  palette,
  bool hasColorKey,
  int index
) {
  return CreatePalettedSprite(w, h, 8, pixels, palette->col, hasColorKey, index);
}


void GLVideoDriver::CreateShaderProgram(GlShaderProgram program) {
  GLSLProgram* shaderProgram = NULL;

  switch (program) {
    case SPRITE_RGBA:
      shaderProgram = GLSLProgram::CreateFromFiles("Shaders/Sprite.glslv", "Shaders/Sprite32.glslf");
      break;
    case SPRITE_BACKBUFFER:
      shaderProgram = GLSLProgram::CreateFromFiles("Shaders/Sprite.glslv", "Shaders/SpriteBB.glslf");
      break;
    case SPRITE_PAL:
      shaderProgram = GLSLProgram::CreateFromFiles("Shaders/Sprite.glslv", "Shaders/SpritePal.glslf");
      break;
    case SPRITE_PAL_GRAY:
      shaderProgram = GLSLProgram::CreateFromFiles("Shaders/Sprite.glslv", "Shaders/SpritePalGrayed.glslf");
      break;
    case SPRITE_PAL_SEPIA:
      shaderProgram = GLSLProgram::CreateFromFiles("Shaders/Sprite.glslv", "Shaders/SpritePalSepia.glslf");
      break;
    case PRIMITIVE:
      shaderProgram = GLSLProgram::CreateFromFiles("Shaders/Primitive.glslv", "Shaders/Primitive.glslf");
      break;
  }

  if (!shaderProgram) {
    std::string error = GLSLProgram::GetLastError();
    Log(FATAL, "SDL 2 GL Driver", "Can't build shader program: %s", error.c_str());
    assert(false);
  }

  shaderProgram->Use();
  shaderProgram->SetUniformValue("s_texture", 1, 0);
  shaderProgram->SetUniformValue("s_palette", 1, 1);
  shaderProgram->SetUniformValue("s_mask", 1, 2);

  shaderPrograms[program] = shaderProgram;
}

SDLVideoDriver::vid_buf_t* GLVideoDriver::CurrentRenderBuffer() {
  return NULL;
}

void GLVideoDriver::DrawLine(const Point& a, const Point& b, const Color& color, bool) {
  std::vector<float> points(4);
  points[0] = a.x;
  points[1] = a.y;
  points[2] = b.x;
  points[3] = b.y;

  DrawLines(points, color);
}

void GLVideoDriver::DrawLines(const std::vector<Point>& points, const Color& color, bool) {
  std::vector<float> floatPoints(points.size() * 2);
  ConvertPointList(points, floatPoints);
  DrawLines(floatPoints, color);
}

void GLVideoDriver::DrawLines(const std::vector<SDL_Point>& sdlLines, const SDL_Color& color, bool) {
  std::vector<float> points(sdlLines.size() * 2);
  ConvertPointList(sdlLines, points);
  DrawLines(points, reinterpret_cast<const Color&>(color));
}

void GLVideoDriver::DrawLines(const std::vector<float>& points, const Color& color) {
  DrawPrimitives(GL_LINES, color, points);
}

void GLVideoDriver::DrawPoint(const Point& point, const Color& color, bool) {
  std::vector<float> points(2);
  points[0] = point.x;
  points[1] = point.y;

  DrawPoints(points, color);
}

void GLVideoDriver::DrawPoints(const std::vector<Point>& points, const Color& color, bool) {
  std::vector<float> floatPoints(points.size() * 2);
  ConvertPointList(points, floatPoints);
  DrawPoints(floatPoints, color);
}

void GLVideoDriver::DrawPoints(const std::vector<SDL_Point>& sdlPoints, const SDL_Color& color, bool) {
  std::vector<float> points(sdlPoints.size() * 2);
  ConvertPointList(sdlPoints, points);
  DrawPoints(points, reinterpret_cast<const Color&>(color));
}

void GLVideoDriver::DrawPoints(const std::vector<float>& points, const Color& color) {
  DrawPrimitives(GL_POINTS, color, points);
}

void GLVideoDriver::DrawPrimitives(
  GLenum primitiveType,
  const Color& color,
  const std::vector<float>& elements
) {
  assert(elements.size() % 2 == 0);

  GLSLProgram *primitiveProgram = GetShaderProgram(PRIMITIVE);
  primitiveProgram->Use();

  assert(drawingBuffer);
  GLRenderBuffer *glRenderBuffer = static_cast<GLRenderBuffer*>(drawingBuffer);
  primitiveProgram->SetUniformMatrixValue("matrix", 4, 1, &(glRenderBuffer->Matrix()[0][0]));

  float floatColor[] = {
    color.r/255.0f,
    color.g/255.0f,
    color.b/255.0f,
    color.a/255.0f,
  };
  primitiveProgram->SetUniformValue("color", 4, 1, floatColor);

  GLuint buffer;
  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, elements.size() * sizeof(GLfloat), &(elements[0]), GL_STATIC_DRAW);

  GLint positionAttribute = primitiveProgram->GetAttribLocation("position");
  glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, 0);

  glEnableVertexAttribArray(positionAttribute);
  glDrawArrays(primitiveType, 0, elements.size()/2);

  glDisableVertexAttribArray(positionAttribute);
  glDeleteBuffers(1, &buffer);
}

void GLVideoDriver::DrawRect(
  const Region& region,
  const Color& color,
  bool fill, bool
) {
  std::vector<float> vertices(8);
  vertices[0] = region.x;
  vertices[1] = region.y;
  vertices[2] = region.x + region.w;
  vertices[3] = region.y;

  if (fill) {
    vertices[4] = region.x;
    vertices[5] = region.y + region.h;
    vertices[6] = region.x + region.w;
    vertices[7] = region.y + region.h;

    DrawPrimitives(GL_TRIANGLE_STRIP, color, vertices);
  } else {
    vertices[4] = region.x + region.w;
    vertices[5] = region.y + region.h;
    vertices[6] = region.x;
    vertices[7] = region.y + region.h;

    DrawPrimitives(GL_LINE_LOOP, color, vertices);
  }
}

Sprite2D* GLVideoDriver::GetScreenshot(Region r) {
  unsigned int w = r.w ? r.w : screenSize.w - r.x;
  unsigned int h = r.h ? r.h : screenSize.h - r.y;

  std::vector<Uint32> glPixels(w * h);

#ifdef USE_GL
  glReadBuffer(GL_BACK);
#endif
  glReadPixels(r.x, r.y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, &(glPixels[0]));

  std::vector<Uint32> spritePixels(glPixels.rbegin(), glPixels.rend());

  return
    new GLTextureSprite2D(
      w,
      h,
      32,
      &(spritePixels[0]),
      0x000000FF,
      0x0000FF00,
      0x00FF0000
    );
}

void GLVideoDriver::Flush() {
  if (drawingBuffer != NULL) {
    GLRenderBuffer *buffer = static_cast<GLRenderBuffer*>(drawingBuffer);
    buffer->PrepareToRender();
    buffer->RenderOnDisplay(NULL);
    buffer->Clear();
  }
}

void GLVideoDriver::PopDrawingBuffer() {
  if (drawingBuffers.size() > 1) {
    (static_cast<GLRenderBuffer*>(drawingBuffer))->Backup();
  }

  Video::PopDrawingBuffer();

  if (drawingBuffers.size() >= 1) {
    (static_cast<GLRenderBuffer*>(drawingBuffers.back()))->Enable();
  }
}

void GLVideoDriver::PushDrawingBuffer(VideoBuffer *buffer) {
  if (drawingBuffer != NULL) {
    (static_cast<GLRenderBuffer*>(drawingBuffer))->Backup();
  }

  Video::PushDrawingBuffer(buffer);

  (static_cast<GLRenderBuffer*>(buffer))->Enable();

  glEnable(GL_SCISSOR_TEST);
  glScissor(0, 0, buffer->Rect().w, buffer->Rect().h);
  glClearColor(1, 0, 1, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);
}

GLSLProgram* GLVideoDriver::GetShaderProgram(GlShaderProgram program) {
  if (shaderPrograms[program] == NULL) {
    CreateShaderProgram(program);
  }

  return shaderPrograms[program];
}

VideoBuffer* GLVideoDriver::NewVideoBuffer(const Region& r, BufferFormat format) {
  if (format == RGB555) {
    Log(FATAL, "SDL2 GL Driver", "16bit mode is not supported!");
    return NULL;
  }

  Region parentRegion(0, 0, screenSize.w, screenSize.h);
  if (drawingBuffers.size() > 1) {
    parentRegion = drawingBuffer->Rect();
  }

  return new GLRenderBuffer(r, parentRegion, format, *GetShaderProgram(SPRITE_BACKBUFFER));
}

void GLVideoDriver::SwapBuffers(VideoBuffers& swapBuffers) {
  for (VideoBuffers::iterator it = swapBuffers.begin(); it != swapBuffers.end(); ++it) {
    GLRenderBuffer *buffer = static_cast<GLRenderBuffer*>(*it);
    buffer->PrepareToRender();
  }
  glViewport(0, 0, screenSize.w, screenSize.h);

  glClearColor(0, 0, 0, 255);
  glClear(GL_COLOR_BUFFER_BIT);

  unsigned int i = 0;
  for (VideoBuffers::iterator it = swapBuffers.begin(); it != swapBuffers.end(); ++it) {
    GLRenderBuffer *buffer = static_cast<GLRenderBuffer*>(*it);
    /*if (i != 1) {
      i += 1;
      continue;
    }
    const Region& r = buffer->Rect();
    Log(MESSAGE, "GL", "At %u (%lu): %d,%d,%d,%d", i, (unsigned long)buffer->id, r.x, r.y, r.w, r.h);*/
    buffer->RenderOnDisplay(NULL);
    buffer->Clear();

    i += 1;
  }

  SDL_GL_SwapWindow(window);

  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
}

// --- Methods that are taken from SDL20Video. ---
bool GLVideoDriver::InTextInput() {
  return SDL_IsTextInputActive();
}

void GLVideoDriver::StopTextInput() {
  SDL_StopTextInput();
}

void GLVideoDriver::StartTextInput() {
#if ANDROID
  if (core->UseSoftKeyboard) {
    SDL_StartTextInput();
  } else {
    Event e = EvntManager->CreateTextEvent(L"");
    EvntManager->DispatchEvent(e);
  }
#else
  SDL_StartTextInput();
#endif
}

bool GLVideoDriver::TouchInputEnabled() {
  return SDL_GetNumTouchDevices() > 0;
}

bool GLVideoDriver::SetFullscreenMode(bool set) {
  Uint32 flags = 0;

  if (set) {
    flags = SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS;
  }

  if (SDL_SetWindowFullscreen(window, flags) == GEM_OK) {
    fullscreen = set;

    return true;
  }

  return false;
}

void GLVideoDriver::SetGamma(int brightness, int) {
  SDL_SetWindowBrightness(window, static_cast<float>(brightness)/10.0f);
}

bool GLVideoDriver::ToggleGrabInput() {
  bool isGrabbed = SDL_GetWindowGrab(window);
  SDL_SetWindowGrab(window, static_cast<SDL_bool>(!isGrabbed));

  return (isGrabbed != SDL_GetWindowGrab(window));
}

#include "plugindef.h"

GEMRB_PLUGIN(0xDBAAB53, "SDL2 GL Video Driver")
PLUGIN_DRIVER(GLVideoDriver, "sdl")
END_PLUGIN()
