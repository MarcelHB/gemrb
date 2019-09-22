#ifndef GLTEXTURESPRITE2D_H
#define GLTEXTURESPRITE2D_H

#include "Sprite2D.h"

namespace GemRB
{
	class GLPaletteManager;

	class GLTextureSprite2D : public Sprite2D
	{
	private:
		GLuint glTexture;
		GLuint glPaletteTexture;
		GLuint glMaskTexture;
		Palette* currentPalette;
		Uint32 rMask, gMask, bMask, aMask;
		ieDword colorKeyIndex;
		GLPaletteManager* paletteManager;
		std::vector<unsigned char> maskPixels;

		void createGlTexture();
		void createGlTextureForPalette();
		void createGLMaskTexture();
	public:
		GLTextureSprite2D (int Width, int Height, int Bpp, void* pixels, Uint32 rmask=0, Uint32 gmask=0, Uint32 bmask=0, Uint32 amask=0);
		~GLTextureSprite2D();
		GLTextureSprite2D(const GLTextureSprite2D &obj);
		GLTextureSprite2D* copy() const;

		GLuint GetTexture();
		GLuint GetPaletteTexture();
		GLuint GetMaskTexture();
		void SetPaletteTexture(int texture);
		Palette* GetPalette() const;
		const Color* GetPaletteColors() const { return currentPalette->col; }
		void SetPalette(Palette *pal);
		Color GetPixel(unsigned short x, unsigned short y) const;
		ieDword GetColorKey() const { return colorKeyIndex; }
		void SetColorKey(ieDword);
		bool IsPaletted() const { return Bpp == 8; }
		void SetPaletteManager(GLPaletteManager* manager) { paletteManager = manager; }
		void MakeUnused();
		bool HasTransparency() const;
		virtual void* LockSprite();
		const virtual void* LockSprite() const;
	};
}

#endif
