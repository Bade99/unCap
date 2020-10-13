#pragma once
#include "unCap_helpers.h"
#include "unCap_math.h"
#include <Windows.h>

#define UNCAP_GDIPLUS /*For lazy bilinear filtered drawing*/

#ifdef UNCAP_GDIPLUS
#include <objidl.h>
#include <gdiplus.h>
#pragma comment (lib,"Gdiplus.lib")
#endif

#ifdef _DEBUG
#define COBJMACROS

#include <Objbase.h>
#include <wincodec.h>
#include <Windows.h>
#include <Winerror.h>

#pragma comment(lib, "Windowscodecs.lib")

HRESULT WriteBitmap(HBITMAP bitmap, const wchar_t* pathname = L"C:/Users/Brenda-Vero-Frank/Desktop/t.bmp") {
	//TODO(fran): solve limitations https://stackoverflow.com/questions/24720451/save-hbitmap-to-bmp-file-using-only-win32
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	HRESULT hr = S_OK;

	// (1) Retrieve properties from the source HBITMAP.
	BITMAP bm_info = { 0 };
	if (!GetObject(bitmap, sizeof(bm_info), &bm_info))
		hr = E_FAIL;

	// (2) Create an IWICImagingFactory instance.
	IWICImagingFactory* factory = NULL;
	if (SUCCEEDED(hr))
		hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
			IID_IWICImagingFactory, (LPVOID*)&factory);

	// (3) Create an IWICBitmap instance from the HBITMAP.
	IWICBitmap* wic_bitmap = NULL;
	if (SUCCEEDED(hr))
		hr = factory->CreateBitmapFromHBITMAP(bitmap, NULL,
			WICBitmapIgnoreAlpha,
			&wic_bitmap);

	// (4) Create an IWICStream instance, and attach it to a filename.
	IWICStream* stream = NULL;
	if (SUCCEEDED(hr))
		hr = factory->CreateStream(&stream);
	if (SUCCEEDED(hr))
		hr = stream->InitializeFromFilename(pathname, GENERIC_WRITE);

	// (5) Create an IWICBitmapEncoder instance, and associate it with the stream.
	IWICBitmapEncoder* encoder = NULL;
	if (SUCCEEDED(hr))
		hr = factory->CreateEncoder(GUID_ContainerFormatBmp, NULL,
			&encoder);
	if (SUCCEEDED(hr))
		hr = encoder->Initialize((IStream*)stream,
			WICBitmapEncoderNoCache);

	// (6) Create an IWICBitmapFrameEncode instance, and initialize it
	// in compliance with the source HBITMAP.
	IWICBitmapFrameEncode* frame = NULL;
	if (SUCCEEDED(hr))
		hr = encoder->CreateNewFrame(&frame, NULL);
	if (SUCCEEDED(hr))
		hr = frame->Initialize(NULL);
	if (SUCCEEDED(hr))
		hr = frame->SetSize(bm_info.bmWidth, bm_info.bmHeight);
	if (SUCCEEDED(hr)) {
		GUID pixel_format;
		if (bm_info.bmBitsPixel == 32)		pixel_format = GUID_WICPixelFormat32bppBGRA;
		else if (bm_info.bmBitsPixel == 24)	pixel_format = GUID_WICPixelFormat24bppBGR;
		else if (bm_info.bmBitsPixel == 1)	pixel_format = GUID_WICPixelFormatBlackWhite;
		else Assert(0);

		hr = frame->SetPixelFormat(&pixel_format);
	}

	// (7) Write bitmap data to the frame.
	if (SUCCEEDED(hr))
		hr = frame->WriteSource((IWICBitmapSource*)wic_bitmap,
			NULL);

	// (8) Commit frame and data to stream.
	if (SUCCEEDED(hr))
		hr = frame->Commit();
	if (SUCCEEDED(hr))
		hr = encoder->Commit();

	// Cleanup
	if (frame)
		frame->Release();
	if (encoder)
		encoder->Release();
	if (stream)
		stream->Release();
	if (wic_bitmap)
		wic_bitmap->Release();
	if (factory)
		factory->Release();

	return hr;
	CoUninitialize();
}
#else
HRESULT WriteBitmap(HBITMAP bitmap, const wchar_t* pathname = L"");
#endif

struct img {
	i32 width;
	i32 height;
	i32 pitch;
	u8 bits_per_pixel;
	void* mem;
};

struct mask_bilinear_sample { u8 sample[4]; };
mask_bilinear_sample sample_bilinear_mask(img* mask, i32 x, i32 y) {
	//NOTE: handmade 108 1:20:00 interesting, once you scale an img to half size or more you get sampling errors cause you're no longer sampling all the pixels involved, that's when you need MIPMAPPING

	u8 idx = 7-(x % 8);

	x /= 8;

	u8* texel_ptr = ((u8*)mask->mem + y * mask->pitch + x * 1);

	u8 tex[4];

	tex[0] = (*(u8*)texel_ptr) & (1 << idx); //NOTE: For a better blend we pick the colors around the texel, movement is much smoother
	tex[1]= idx==0 ? ( x<mask->width ? (*(u8*)(texel_ptr + 1)) & (1 << 7) : 0xFF) : tex[0];
	tex[2]= y<mask->height ? (*(u8*)(texel_ptr + mask->pitch)) & (1 << idx) : 0xFF;
	tex[3]= idx == 0 ? (x < mask->width ? (*(u8*)(texel_ptr + mask->pitch + 1)) & (1 << 7) : 0xFF) : tex[2];

	mask_bilinear_sample sample;
	for(int i=0;i<4;i++)
		sample.sample[i] = tex[i];

	return sample;
}

namespace urender {
	static ULONG_PTR gdiplusToken;//Horrible HACK, that gdi+ shouldnt need in the first place but the programmers had no idea what they were doing

	void init() {
#ifdef UNCAP_GDIPLUS
		// Initialize GDI+
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
#endif
	}

	void uninit() {
#ifdef UNCAP_GDIPLUS
		//Uninitialize GDI+
		Gdiplus::GdiplusShutdown(gdiplusToken);
#endif
	}

	//NOTE: the caller is in charge of handling the hbitmap, including deletion
//NOTE: we add an extra limitation, no offsets, we scale the entire bitmap, at least for now to make it simpler to write
	HBITMAP scale_mask(HBITMAP mask_bmp, int destW, int destH) {

		BITMAP masknfo; GetObject(mask_bmp, sizeof(masknfo), &masknfo); Assert(masknfo.bmBitsPixel == 1);
		img mask;
		mask.width = masknfo.bmWidth;
		mask.height = masknfo.bmHeight;
		mask.bits_per_pixel = 1;
		mask.pitch = masknfo.bmWidthBytes;
		int mask_sz = mask.height*mask.pitch;
		mask.mem = malloc(mask_sz); defer{ free(mask.mem); };

		int getbits = GetBitmapBits(mask_bmp, mask_sz, mask.mem);
		Assert(getbits == mask_sz);

		img scaled_mask;
		scaled_mask.width = destW;
		scaled_mask.height = destH;
		scaled_mask.bits_per_pixel = 1;
		scaled_mask.pitch = round2up((i32)roundf(((f32)scaled_mask.width * (1.f / 8.f/*bytes per pixel*/))));//NOTE: windows expects word aligned bmps
		int scaled_mask_sz = scaled_mask.height*scaled_mask.pitch;
		scaled_mask.mem = (u8*)malloc(scaled_mask_sz); defer{ free(scaled_mask.mem); };


		v2 origin{ 0,0 };
		v2 x_axis{ (f32)scaled_mask.width,0 };
		v2 y_axis{ 0,(f32)scaled_mask.height };

		i32 width_max = scaled_mask.width - 1;
		i32 height_max = scaled_mask.height - 1;

		i32 x_min = scaled_mask.width;
		i32 x_max = 0;
		i32 y_min = scaled_mask.height;
		i32 y_max = 0;

		v2 points[4] = { origin,origin + x_axis,origin + y_axis,origin + x_axis + y_axis };
		for (v2 p : points) {
			i32 floorx = (i32)floorf(p.x);
			i32 ceilx = (i32)ceilf(p.x);
			i32 floory = (i32)floorf(p.y);
			i32 ceily = (i32)ceilf(p.y);

			if (x_min > floorx)x_min = floorx;
			if (y_min > floory)y_min = floory;
			if (x_max < ceilx)x_max = ceilx;
			if (y_max < ceily)y_max = ceily;
		}

		if (x_min < 0)x_min = 0;
		if (y_min < 0)y_min = 0;
		if (x_max > scaled_mask.width) x_max = scaled_mask.width;
		if (y_max > scaled_mask.height)y_max = scaled_mask.height;

		u8* row = (u8*)scaled_mask.mem /*+(x_min *(i32)((f32)scaled_mask.bits_per_pixel/8.f))*/ + y_min * scaled_mask.pitch;

		for (int y = y_min; y < y_max; y++) {//n bits high
			u8* pixel = row;
			u8 index = 7;
			u8 pixels_to_write = 0;
			for (int x = x_min; x < x_max; x++) {//n bits wide

				f32 u = (f32)x / (f32)(scaled_mask.width - 1);
				f32 v = (f32)y / (f32)(scaled_mask.height - 1);

				f32 t_x = u * (f32)(mask.width - 1);
				f32 t_y = v * (f32)(mask.height - 1);

				i32 i_x = (i32)t_x;
				i32 i_y = (i32)t_y;

				f32 f_x = t_x - (f32)i_x;
				f32 f_y = t_y - (f32)i_y;

				Assert(i_x >= 0 && i_x < mask.width);
				Assert(i_y >= 0 && i_y < mask.height);

				//Bilinear filtering
				mask_bilinear_sample sample = sample_bilinear_mask(&mask, i_x, i_y);
				v4 f_sample;
				//TODO(fran): lerp and f_x f_y
				for (int i = 0; i < 4; i++) {
					f_sample.comp[i] = (sample.sample[i] ? 1.f : 0.f);
				}

				u8 texel = (u8)roundf(lerp(lerp(f_sample.x, f_x, f_sample.y), f_y, lerp(f_sample.z, f_x, f_sample.w)));

				pixels_to_write |= (texel << index);

				index--;
				if (index == (u8)-1 || (x + 1) == x_max) {
					index = 7;
					//pixels_to_write = ~pixels_to_write;
					*pixel++ = pixels_to_write;
					pixels_to_write = 0;
				}
			}
			row += scaled_mask.pitch;
		}

		//Create bitmap
		HBITMAP new_mask = CreateBitmap(scaled_mask.width, scaled_mask.height, 1, scaled_mask.bits_per_pixel, scaled_mask.mem);
		//TODO(fran): do I need to set the palette?
		return new_mask;
	}

	void draw_mask(HDC dest, int xDest, int yDest, int wDest, int hDest, HBITMAP mask, int xSrc, int ySrc, int wSrc, int hSrc, HBRUSH colorbr) {
		{BITMAP bmpnfo; GetObject(mask, sizeof(bmpnfo), &bmpnfo); Assert(bmpnfo.bmBitsPixel == 1); }

		//Mask Stretching Setup
#if 0 //You can never trust Gdi's stretching functions, they are simply terrible and unusable
		HDC src = CreateCompatibleDC(dest); defer{ DeleteDC(src); };
		//NOTE: src has a 1x1 _monochrome_ bitmap, we wanna be monochrome too
		HBITMAP srcbmp = CreateCompatibleBitmap(src, wDest, hDest); defer{ DeleteObject(srcbmp); };
		
		HBITMAP oldbmp = (HBITMAP)SelectObject(src, (HGDIOBJ)srcbmp); defer{ SelectObject(src, (HGDIOBJ)oldbmp); };
		
		HDC maskdc = CreateCompatibleDC(dest); defer{ DeleteDC(maskdc); };
		
		HBITMAP oldmaskdcbmp = (HBITMAP)SelectObject(maskdc, (HGDIOBJ)mask); defer{ SelectObject(maskdc, (HGDIOBJ)oldmaskdcbmp); };
		
		//Finally we can actually do the stretching
		int stretchres = StretchBlt(src, 0, 0, wDest, hDest, maskdc, xSrc, ySrc, wSrc, hSrc, SRCCOPY);

		HBRUSH oldbr = (HBRUSH)SelectObject(dest, colorbr); defer{ SelectObject(dest, (HGDIOBJ)oldbr); };

		//TODO(fran): I have no idea why I need to pass the "srcDC", no information from it is needed, and the function explicitly says it should be NULL, but if I do it returns false aka error (some error, cause it also doesnt tell you what it is)
		//NOTE: info on creating your own raster ops https://docs.microsoft.com/en-us/windows/win32/gdi/ternary-raster-operations?redirectedfrom=MSDN
		//Thanks https://stackoverflow.com/questions/778666/raster-operator-to-use-for-maskblt
		HBITMAP stretched_mask = (HBITMAP)GetCurrentObject(src, OBJ_BITMAP);//NOTE: we dont need to do this, we got the hbitmap from before
		BOOL maskres = MaskBlt(dest, xDest, yDest, wDest, hDest, dest, 0, 0, stretched_mask, 0, 0, MAKEROP4(0x00AA0029, PATCOPY));
#elif 0
		//TODO(fran): https://devblogs.microsoft.com/oldnewthing/20061114-01/?p=29013 raymond always to the rescue, why dont I bitblt to color first and then stretch with gdi+?
		HDC stretch32 = CreateCompatibleDC(dest); defer{ DeleteDC(stretch32); };
		HBITMAP bmp32 = CreateBitmap(wDest, hDest, 1, 32, nullptr); defer{ DeleteObject(bmp32); };
		HBITMAP oldbmp32 = (HBITMAP)SelectObject(stretch32, (HGDIOBJ)bmp32); defer{ SelectObject(stretch32, (HGDIOBJ)oldbmp32); };
		{
			Gdiplus::Graphics graphics(stretch32);//Yeah, who cares, icons are small, I just want bilinear filtering
			graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
			
			Gdiplus::Bitmap bmp(mask, 0);//NOTE: gdi converts bitmaps to 32bpp, so, once again, it is useless. Moreover you can ONLY draw to 32bpp dcs with this bmp
			graphics.DrawImage(&bmp,
				Gdiplus::Rect(0, 0, wDest, hDest),
				xSrc, ySrc, wSrc, hSrc,
				Gdiplus::Unit::UnitPixel);//TODO(fran): get the current unit from the device
		}

		HDC stretch24 = CreateCompatibleDC(dest); defer{ DeleteDC(stretch24); };
		HBITMAP bmp24 = CreateBitmap(wDest, hDest, 1, 24, nullptr); defer{ DeleteObject(bmp24); };
		HBITMAP oldbmp24 = (HBITMAP)SelectObject(stretch24, (HGDIOBJ)bmp24); defer{ SelectObject(stretch24, (HGDIOBJ)oldbmp24); };
		BitBlt(stretch24, 0, 0, wDest, hDest, stretch32, 0, 0, SRCCOPY);//32bit to 24bit //THIS DOESNT WOOOOOOOOOOOOOOOOOOOOOOOOOOOORRKKKKKK
		
		HDC stretch1 = CreateCompatibleDC(dest); defer{ DeleteDC(stretch1); };
		HBITMAP bmp1 = CreateBitmap(wDest, hDest, 1, 1, nullptr); defer{ DeleteObject(bmp1); };
		HBITMAP oldbmp1 = (HBITMAP)SelectObject(stretch1, (HGDIOBJ)bmp1); defer{ SelectObject(stretch1, (HGDIOBJ)oldbmp1); };

		SetBkColor(stretch24, RGB(0xff, 0xff, 0xff));
		SetBkColor(stretch1, RGB(0x00, 0x00, 0x00));
		BitBlt(stretch1, 0, 0, wDest, hDest, stretch24, 0, 0, SRCCOPY);//24bit to 1bit

		BITMAP nfo; GetObject(bmp1, sizeof(nfo), &nfo);
		char bits[128 * 128];
		int r = GetBitmapBits(bmp1, nfo.bmHeight*nfo.bmWidthBytes, bits);

		BOOL maskres = MaskBlt(dest, xDest, yDest, wDest, hDest, dest, 0, 0, bmp1, 0, 0, MAKEROP4(0x00AA0029, PATCOPY));
#else
		mask = (wDest!=wSrc || hDest!=hSrc) ? scale_mask(mask, wDest, hDest) : mask;
		HBRUSH oldbr = (HBRUSH)SelectObject(dest, colorbr); defer{ SelectObject(dest, (HGDIOBJ)oldbr); };
		BOOL maskres = MaskBlt(dest, xDest, yDest, wDest, hDest, dest, 0, 0, mask, 0, 0, MAKEROP4(0x00AA0029, PATCOPY));
#endif
		Assert(maskres);
	}

	void draw_icon(HDC dest, int xDest, int yDest, int wDest, int hDest, HICON icon, int xSrc, int ySrc, int wSrc, int hSrc) {
		Gdiplus::Graphics graphics(dest);//Yeah, who cares, icons are small, I just want bilinear filtering
		graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
		Gdiplus::Bitmap bmp(icon);
		//TODO?(fran): with gdi+ we can do subpixel placement (RectF)
		graphics.DrawImage(&bmp,
			Gdiplus::Rect(xDest, yDest, wDest, hDest),
			xSrc, ySrc, wSrc, hSrc,
			Gdiplus::Unit::UnitPixel);//TODO(fran): get the current unit from the device
	}
}