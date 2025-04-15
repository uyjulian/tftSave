#pragma once

#include <windows.h>
#include <dwrite.h>
#include <vector>
#include <dwrite_3.h>

#pragma comment( lib, "dwrite.lib" )

class DLLHolder {
	HMODULE handle;
public:
	DLLHolder() : handle(NULL) {}
	DLLHolder(LPCWSTR name) : handle(NULL) { load(name); }
	virtual ~DLLHolder() { unload(); }
	void unload() {
		if (handle) ::FreeLibrary(handle);
		/**/handle = NULL;
	}
	bool load(LPCWSTR name) {
		unload();
		handle = name ? ::LoadLibraryW(name) : NULL;
		return handle != NULL;
	}

	template <typename T>
	bool operator()(T &ptr, const char *name) const {
		ptr = handle ? (T)::GetProcAddress(handle, name) : (T)nullptr;
		return ptr != nullptr;
	}
};

class DWriteUtil : protected DLLHolder {
	HRESULT (WINAPI *CreateFactory)(DWRITE_FACTORY_TYPE, REFIID, IUnknown**);
protected:
	IDWriteFactory *Factory;
	IDWriteGdiInterop *InterOp;
public:
	DWriteUtil() : DLLHolder(L"dwrite.dll"), CreateFactory(nullptr), Factory(nullptr), InterOp(nullptr) {
		(*this)(CreateFactory, "DWriteCreateFactory");
	}
	~DWriteUtil() {
		if (InterOp) InterOp->Release();
		if (Factory) Factory->Release();
		/**/Factory = nullptr;
		CreateFactory = nullptr;
	}
	IDWriteFactory* factory() {
		if (!Factory && CreateFactory) {
			if (FAILED(CreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&Factory)))) {
				// ignore after all
				CreateFactory = nullptr;
			}
		}
		return Factory;
	}

	IDWriteGdiInterop* gdi() {
		if (!InterOp && factory()) Factory->GetGdiInterop(&InterOp);
		return InterOp;
	}
};

struct DWriteGlyphBitmap {
	LONG left, top;
	UINT width, height;
	float ascent, descent, advance;
	std::vector<BYTE> image;
};

class DWriteGlyphRenderer {
protected:
	DWriteUtil &Util;
	IDWriteFont *Font;
	IDWriteFontFace *Face;
	IDWriteBitmapRenderTarget *Target;
	IDWriteRenderingParams *Params;
	float FontSize;
	UINT32 BitmapSize;
	DWRITE_FONT_METRICS Metrics;
public:
	DWriteGlyphRenderer(HDC dc, DWriteUtil &util)
		: Util(util), Font(nullptr), Face(nullptr), Target(nullptr), Params(nullptr), FontSize(0.0f), BitmapSize(0)
	{
		IDWriteGdiInterop *op = util.gdi();
		if (op) {
			if (SUCCEEDED(op->CreateBitmapRenderTarget(dc, 1, 1, &Target))) {
				Target->SetPixelsPerDip(1.0f);
				HDC dc = Target->GetMemoryDC();
				::SetBoundsRect(dc, NULL, DCB_DISABLE);
			}
			/*
			IDWriteRenderingParams *test = nullptr;
			util.factory()->CreateRenderingParams(&test);
			if (test) {
				IDWriteRenderingParams1 *test1 = nullptr;
				if (SUCCEEDED(test->QueryInterface(__uuidof(IDWriteRenderingParams1), (void**)&test1))) {
					FLOAT grays = test1->GetGrayscaleEnhancedContrast();
					bool ok = false;
					test1->Release();
				}
				FLOAT gamma = test->GetGamma();
				FLOAT contr = test->GetEnhancedContrast();
				FLOAT clear = test->GetClearTypeLevel();
				bool done = true;
				test->Release();
			}
			 */
			IDWriteFactory *fa = util.factory();
			if (fa) {
				IDWriteFactory3 *fa3 = nullptr;
				if (SUCCEEDED(fa->QueryInterface(__uuidof(IDWriteFactory3), (void**)&fa3))) {
					IDWriteRenderingParams3 *params = nullptr;
					if (SUCCEEDED(fa3->CreateCustomRenderingParams(
						/*gamma*/1.8f, /*enhancedContrast*/0.5f, /*gsContrast*/1.0f, /*cleartype*/0.0f,
						DWRITE_PIXEL_GEOMETRY_FLAT,
						//DWRITE_RENDERING_MODE1_NATURAL_SYMMETRIC_DOWNSAMPLED, //DWRITE_RENDERING_MODE1_NATURAL_SYMMETRIC,
						//DWRITE_GRID_FIT_MODE_ENABLED,

						DWRITE_RENDERING_MODE1_NATURAL_SYMMETRIC,
						DWRITE_GRID_FIT_MODE_DISABLED,
						&params)))
					{
						// static-cast ‚ÅOK‚Ì‚Í‚¸
						Params = static_cast<IDWriteRenderingParams*>(params);
						/** NG‚Ìê‡
						params->QueryInterface(__uuidof(IDWriteRenderingParams), (void**)&Params);
						params->Release();
						 */
					}
					fa3->Release();
				}
				if (!Params) {
					fa->CreateCustomRenderingParams(
						/*gamma*/1.8f, /*enhancedContrast*/0.5f, /*cleartype*/0.0f,
						DWRITE_PIXEL_GEOMETRY_FLAT,
						/// Specifies that the rendering mode is determined automatically based on the font and size.
						//DWRITE_RENDERING_MODE_DEFAULT,
						/// Specifies that no anti-aliasing is performed. Each pixel is either set to the foreground color of the text or retains the color of the background.
						//DWRITE_RENDERING_MODE_ALIASED,
						/// Specifies ClearType rendering with the same metrics as aliased text. Glyphs can only be positioned on whole-pixel boundaries.
						//DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC,
						/// Specifies ClearType rendering with the same metrics as text rendering using GDI using a font created with CLEARTYPE_NATURAL_QUALITY.
						/// Glyph metrics are closer to their ideal values than with aliased text, but glyphs are still positioned on whole-pixel boundaries.
						//DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL,
						/// Specifies ClearType rendering with anti-aliasing in the horizontal dimension only.
						/// This is typically used with small to medium font sizes (up to 16 ppem).
						//DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL,
						/// Specifies ClearType rendering with anti-aliasing in both horizontal and vertical dimensions. 
						/// This is typically used at larger sizes to makes curves and diagonal lines look smoother, at the expense of some softness.
						DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC,

						&Params);
				}
			}
		}
	}
	~DWriteGlyphRenderer() {
		clear();
		if (Target) Target->Release();
		/**/Target = nullptr;
		if (Params) Params->Release();
		/**/Params = nullptr;
	}
	void clear() {
		if (Font) Font->Release();
		/**/Font = nullptr;
		if (Face) Face->Release();
		/**/Face = nullptr;
		FontSize = 0.0f;
		::ZeroMemory(&Metrics, sizeof(Metrics));
	}
	bool setFont(const LOGFONTW &logfont)  {
		clear();
		IDWriteGdiInterop *op = Util.gdi();
		if (op && SUCCEEDED(op->CreateFontFromLOGFONT(&logfont, &Font)) && SUCCEEDED(Font->CreateFontFace(&Face))) {
			Face->GetMetrics(&Metrics);
			LONG fh = logfont.lfHeight;
			if (fh < 0) FontSize = (float)-fh;
			else {
				float ch = (float)(Metrics.ascent + Metrics.descent) / Metrics.designUnitsPerEm;
				FontSize = (float)fh / ch;
			}
			BitmapSize = (UINT32)(FontSize * 3);
			if (Target) Target->Resize(BitmapSize, BitmapSize);
			return true;
		}
		return false;
	}
	bool render(UINT32 ucs4, DWriteGlyphBitmap &bitmap) {
		if (!Face || !Target || !Params) return false;

		UINT16 index = 0;
		if (FAILED(Face->GetGlyphIndices(&ucs4, 1, &index))) return false;

		DWRITE_GLYPH_METRICS glmet = {0};
		if (FAILED(Face->GetDesignGlyphMetrics(&index, 1, &glmet, FALSE))) return false;

		const float coef = FontSize / Metrics.designUnitsPerEm;
		bitmap.ascent  = Metrics.ascent * coef;
		bitmap.descent = Metrics.descent * coef;
		bitmap.advance = glmet.advanceWidth * coef;

		FLOAT advance = 0.0f;
		DWRITE_GLYPH_OFFSET glofs = { 0.0f, 0.0f };
		const DWRITE_GLYPH_RUN glrun = {
			Face, // fontFace
			FontSize, // fontEmSize
			1, // glyphCount
			&index, // glyphIndices
			&advance, // glyphAdvances
			&glofs, // glyphOffsets
			FALSE, // isSideways
			0 }; //bidiLevel
#if 1
		RECT bbox = {0};

		HDC dc = Target->GetMemoryDC();
		RECT rect = { 0, 0, (LONG)BitmapSize, (LONG)BitmapSize };
		::FillRect(dc, &rect, (HBRUSH)::GetStockObject(BLACK_BRUSH));
		LONG ox = (LONG)FontSize;
		LONG oy = (LONG)(coef * glmet.verticalOriginY + FontSize);
		if (FAILED(Target->DrawGlyphRun((FLOAT)ox, (FLOAT)oy, DWRITE_MEASURING_MODE_NATURAL, &glrun, Params, 0xFFFFFF, &bbox))) return false;
		if (bbox.left < 0 || bbox.top < 0 || bbox.right >= rect.right || bbox.bottom >= rect.bottom) return false;

		bitmap.left = bbox.left - ox;
		bitmap.top  = oy - bbox.top; // ã‰º‹t
		bitmap.width  = bbox.right - bbox.left;
		bitmap.height = bbox.bottom - bbox.top;
		size_t wh = bitmap.width * bitmap.height;
		if (wh > 0) {
			bitmap.image.resize(wh);

			DIBSECTION dib;
			if (::GetObject(::GetCurrentObject(dc, OBJ_BITMAP), sizeof(dib), &dib) != sizeof(dib)) return false;

			const int pitch = (dib.dsBm.bmWidthBytes / sizeof (DWORD));
			const DWORD *p = (const DWORD*)dib.dsBm.bmBits + bbox.left + (bbox.top * pitch);

			BYTE *q = &bitmap.image.front();
			for (UINT y = 0; y < bitmap.height; ++y, p+=pitch) {
				const DWORD *r = p;
				for (UINT x = 0; x < bitmap.width; ++x) *q++ = (*r++ >> 8) & 0xFF;
			}
		}


		return true;
#endif
	}
};












#if 0
		/*
		static const DWRITE_MATRIX matrix = {
			1.0f, 0.0f,
			0.0f, 1.0f,
			0.0f, 0.0f };
		 */

		IDWriteGlyphRunAnalysis *analysis = nullptr;
		if (FAILED(Util.factory()->CreateGlyphRunAnalysis( // Face‚ª‘¶Ý‚·‚éê‡í‚Éfactory‚à‘¶Ý‚·‚é
			&glrun, 1.0f, NULL, //&matrix,
            DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC,
            DWRITE_MEASURING_MODE_GDI_CLASSIC,
//			vaa ? DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC : DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL,
//			DWRITE_MEASURING_MODE_NATURAL,
			0.0f, 0.0f, &analysis))) return false;


		RECT rect = {0};
		analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_ALIASED_1x1, &rect);
		LONG width  = rect.right - rect.left;
		LONG height = rect.bottom - rect.top;
		if (width <= 0 || height <= 0) {
			analysis->Release();
			bitmap.left = bitmap.top = 0;
			bitmap.width = bitmap.height = 0;
			return true;
		}

		size_t size = width * height;
		bitmap.left = rect.left;
		bitmap.top  = rect.top;
		bitmap.width = width;
		bitmap.height = height;
		bitmap.image.resize(size);
		BYTE *buf = &bitmap.image.front();
		::ZeroMemory(buf, size);
		bool r = SUCCEEDED(analysis->CreateAlphaTexture(DWRITE_TEXTURE_ALIASED_1x1,
														&rect, buf, size));
		analysis->Release();
#endif
