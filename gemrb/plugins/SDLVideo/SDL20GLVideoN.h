#ifndef GLVideoDRIVER_H
#define GLVideoDRIVER_H

#include <glm/glm.hpp>

#include "SDLVideo.h"
#include "GLPaletteManager.h"
#include "GLSLProgram.h"

namespace GemRB {

enum GlShaderProgram {
  SPRITE_RGBA = 0,
  SPRITE_BACKBUFFER,
  SPRITE_PAL,
  SPRITE_PAL_GRAY,
  SPRITE_PAL_SEPIA,
  PRIMITIVE
};

#define NUM_SHADER_PROGRAMS 6

//static bool stopDrawing;

class GLRenderBuffer : public VideoBuffer {
  public:
    size_t id;
    GLRenderBuffer(const Region&, const Region& parent, Video::BufferFormat, GLSLProgram&);
    ~GLRenderBuffer();

    void Backup();
    void Clear();
    void CopyPixels(const Region&, const void*, const int*, ...);
    void Enable();
    const glm::mat4& Matrix() const;
    void PrepareToRender();
    bool RenderOnDisplay(void*) const;
    void Reuse();
  private:
    static size_t lastId;
    bool shotted;
    GLSLProgram& spriteProgram;
    std::vector<Uint32> backBuffer;
    Video::BufferFormat format;
    GLuint glTexture;
    glm::mat4 matrix;
    const Region parentRegion;
    enum { NEW, ACTIVE, BACKUP, TEXTURIZED } state;

    void CreateTexture();
    void Draw() const;
};

class GLVideoDriver : public SDLVideoDriver {
  public:
    GLVideoDriver();
    ~GLVideoDriver();

    int CreateDriverDisplay(const Size&, int bpp, const char* title);
    Sprite2D* CreateSprite(int w, int h, int bpp, ieDword rMask, ieDword gMask, ieDword bMask, ieDword aMask, void* pixels, bool cK = false, int index = 0);
    Sprite2D* CreateSprite8(int w, int h, void* pixels, Palette* palette, bool cK, int index);
    Sprite2D* CreatePalettedSprite(int w, int h, int bpp, void* pixels, Color* palette, bool cK = false, int index = 0);

    void DrawLine(const Point&, const Point&, const Color&, bool needsMask = false);
    void DrawLines(const std::vector<Point>&, const Color&, bool needsMask = false);
    void DrawRect(const Region&, const Color&, bool fill, bool needsMask = false);
    void DrawPoint(const Point&, const Color&, bool needsMask = false);
    void DrawPoints(const std::vector<Point>&, const Color&, bool needsMask = false);

    void Flush();
    Sprite2D* GetScreenshot(Region);
    bool InTextInput();

    void PushDrawingBuffer(VideoBuffer*);
    void PopDrawingBuffer();

    void SetGamma(int brightness, int contrast);
    bool SetFullscreenMode(bool);

    void StartTextInput();
    void StopTextInput();

    void SwapBuffers(VideoBuffers&);
    bool ToggleGrabInput();
    bool TouchInputEnabled();

  private:
    SDL_Window *window;
    SDL_GLContext context;
    SDL_Renderer *renderer;
    GLPaletteManager paletteManager;

    std::vector<GLSLProgram*> shaderPrograms;

    vid_buf_t* CurrentRenderBuffer();

    void BlitSpriteBAMClipped(const Sprite2D *sprite, const Sprite2D *mask, const Region& source, const Region& dest, unsigned int flags = 0, const Color *tint = NULL);
    void BlitSpriteNativeClipped(const Sprite2D *sprite, const Sprite2D *mask, const SDL_Rect& src, const SDL_Rect& dst, unsigned int flags = 0, const SDL_Color *tint = NULL);

    void CreateShaderProgram(GlShaderProgram);
    void ConvertSDLPointsList(const std::vector<SDL_Point>&, std::vector<Point>&);

    void DrawLines(const std::vector<float>&, const Color&);
    void DrawLines(const std::vector<SDL_Point>&, const SDL_Color&, bool needsMask = false);
    void DrawPoints(const std::vector<float>&, const Color&);
    void DrawPoints(const std::vector<SDL_Point>&, const SDL_Color&, bool needsMask = false);
    void DrawPrimitives(GLenum primitiveType, const Color&, const std::vector<float>& elements);

    GLSLProgram* GetShaderProgram(GlShaderProgram);

    VideoBuffer* NewVideoBuffer(const Region&, BufferFormat);
};

template <typename T>
void ConvertPointList(const std::vector<T>& input, std::vector<float>& output) {
  assert(input.size() * 2 == output.size());

  for (typename std::vector<T>::size_type i = 0; i < input.size(); ++i) {
    const T& p = input[i];
    output[i*2] = static_cast<float>(p.x);
    output[i*2+1] = static_cast<float>(p.y);
  }
}

}

#endif
