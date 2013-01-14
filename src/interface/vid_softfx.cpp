// Software blitter effects

#include "burner.h"
#include "vid_softfx.h"

typedef unsigned long uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

#ifndef _WIN32
 typedef unsigned long DWORD;
#endif

void _2xpm_lq(void *SrcPtr, void *DstPtr, unsigned long SrcPitch, unsigned long DstPitch, unsigned long SrcW, unsigned long SrcH, int nDepth);
void _2xpm_hq(void *SrcPtr, void *DstPtr, unsigned long SrcPitch, unsigned long DstPitch, unsigned long SrcW, unsigned long SrcH, int nDepth);

extern void hq2xS_init(unsigned bits_per_pixel);
extern void hq2xS(unsigned char*, unsigned int, unsigned char*, unsigned char*, unsigned int, int, int);
extern void hq2xS32(unsigned char*, unsigned int, unsigned char*, unsigned char*, unsigned int, int, int);
#if defined _MSC_VER && defined BUILD_X86_ASM
extern void hq3xS(unsigned char*,unsigned int,unsigned char*,unsigned char*,unsigned int,int,int);
extern void hq3xS32(unsigned char*,unsigned int,unsigned char*,unsigned char*,unsigned int,int,int);
#endif

extern int Init_2xSaI(unsigned int BitFormat, unsigned int systemColorDepth);
extern void _2xSaI32(unsigned char*, unsigned int, unsigned char*, unsigned char*, unsigned int, int, int);
extern void Super2xSaI32(unsigned char*, unsigned int, unsigned char*, unsigned char*, unsigned int, int, int);
extern void SuperEagle32(unsigned char*, unsigned int, unsigned char*, unsigned char*, unsigned int, int, int);

extern void InitLUTs();
extern void RenderHQ2XS(unsigned char*, unsigned int, unsigned char*, unsigned int, int, int, int type);
extern void RenderHQ3XS(unsigned char*, unsigned int, unsigned char*, unsigned int, int, int, int type);

void RenderEPXB(unsigned char*, unsigned int, unsigned char*, unsigned int, int, int, int);
void RenderEPXC(unsigned char*, unsigned int, unsigned char*, unsigned int, int, int, int);

#if defined __GNUC__
 #include "scale2x.h"
#elif defined _MSC_VER && defined BUILD_X86_ASM
 #include "scale2x_vc.h"
 #define scale2x_16_mmx internal_scale2x_16_mmx
 #define scale2x_32_mmx internal_scale2x_32_mmx
#endif
#include "scale3x.h"

#if defined BUILD_X86_ASM
extern "C" void __cdecl superscale_line(UINT16 *src0, UINT16 *src1, UINT16 *src2, UINT16 *dst, UINT32 width, UINT64 *mask);
extern "C" void __cdecl  superscale_line_75(UINT16 *src0, UINT16 *src1, UINT16 *src2, UINT16 *dst, UINT32 width, UINT64 *mask);

#ifdef __ELF__
 #define LUT16to32 _LUT16to32
 #define RGBtoYUV _RGBtoYUV
 #define hq2x_32 _hq2x_32
 #define hq3x_32 _hq3x_32
 #define hq4x_32 _hq4x_32
#endif

extern "C" void __cdecl _eagle_mmx16(unsigned long* lb, unsigned long* lb2, short width, unsigned long* screen_address1, unsigned long* screen_address2);

extern "C" void __cdecl _2xSaISuperEagleLine(uint8* srcPtr, uint8* deltaPtr, uint32 srcPitch, uint32 width, uint8* dstPtr, uint32 dstPitch, uint16 dstBlah);
extern "C" void __cdecl _2xSaILine(uint8* srcPtr, uint8* deltaPtr, uint32 srcPitch, uint32 width, uint8* dstPtr, uint32 dstPitch, uint16 dstBlah);
extern "C" void __cdecl _2xSaISuper2xSaILine(uint8* srcPtr, uint8* deltaPtr, uint32 srcPitch, uint32 width, uint8* dstPtr, uint32 dstPitch, uint16 dstBlah);
extern "C" void __cdecl Init_2xSaIMMX(uint32 BitFormat);

extern "C" {
	void __cdecl hq2x_32(unsigned char*, unsigned char*, DWORD, DWORD, DWORD);
	void __cdecl hq3x_32(unsigned char*, unsigned char*, DWORD, DWORD, DWORD);
	void __cdecl hq4x_32(unsigned char*, unsigned char*, DWORD, DWORD, DWORD);

	unsigned int LUT16to32[65536];
	unsigned int RGBtoYUV[65536];
}
#endif

#define FXF_MMX		(1 << 31)

static struct { TCHAR* pszName; int nZoom; unsigned int nFlags; } SoftFXInfo[] = {
	{ _T("Plain Software Scale"),		2, 0	   },
};

static unsigned char* pSoftFXImage = NULL;
static int nSoftFXImageWidth = 0;
static int nSoftFXImageHeight = 0;
static int nSoftFXImagePitch = 0;

static unsigned char* pSoftFXXBuffer = NULL;

static int nSoftFXRotate = 0;
static int nSoftFXBlitter = 0;
static bool nSoftFXEnlarge = 0;

static bool MMXSupport()
{
#if defined BUILD_X86_ASM
	unsigned int nSignatureEAX = 0, nSignatureEBX = 0, nSignatureECX = 0, nSignatureEDX = 0;

	CPUID(1, nSignatureEAX, nSignatureEBX, nSignatureECX, nSignatureEDX);

	return (nSignatureEDX >> 23) & 1;						// bit 23 of edx ndicates MMX support
#else
	return 0;
#endif
}

TCHAR* VidSoftFXGetEffect(int nEffect)
{
	return SoftFXInfo[nEffect].pszName;
}

int VidSoftFXGetZoom(int nEffect)
{
	return SoftFXInfo[nEffect].nZoom;
}

int VidSoftFXCheckDepth(int nEffect, int nDepth)
{
	switch (nEffect) {
		case FILTER_PLAIN:
			return nDepth;
	}

	return 0;
}

void VidSoftFXExit()
{
	if (pSoftFXXBuffer) {
		free(pSoftFXXBuffer);
		pSoftFXXBuffer = NULL;
	}

	if (nSoftFXRotate) {
		free(pSoftFXImage);
		pSoftFXImage = NULL;
	}
	pSoftFXImage = NULL;

	nSoftFXRotate = 0;
	nSoftFXEnlarge = 0;
	nSoftFXBlitter = 0;

	return;
}

int VidSoftFXInit(int nBlitter, int nRotate)
{
	nSoftFXBlitter = nBlitter;
	nSoftFXEnlarge = true;
	
	if ((MMXSupport() == false && (SoftFXInfo[nSoftFXBlitter].nFlags & FXF_MMX)) || VidSoftFXCheckDepth(nSoftFXBlitter, nVidImageDepth) == 0) {
		VidSoftFXExit();
		return 1;
	}

	pSoftFXImage = pVidImage + nVidImageLeft * nVidImageBPP;

	nSoftFXImageWidth = nVidImageWidth; nSoftFXImageHeight = nVidImageHeight;

	nSoftFXRotate = 0;
    if (bDrvOkay) {
		BurnDrvGetFullSize(&nSoftFXImageWidth, &nSoftFXImageHeight);

		if ((nRotate & 1) && (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL)) {
			nSoftFXRotate |= 1;

			BurnDrvGetFullSize(&nSoftFXImageHeight, &nSoftFXImageWidth);
		}

		if ((nRotate & 2) && (BurnDrvGetFlags() & BDF_ORIENTATION_FLIPPED)) {
			nSoftFXRotate |= 2;
		}

		if (nSoftFXRotate) {
			pSoftFXImage = (unsigned char*)malloc(nSoftFXImageWidth * nSoftFXImageHeight * nVidImageBPP);
			if (pSoftFXImage == NULL) {
				VidSoftFXExit();
				return 1;
			}
		}
	}
	nSoftFXImagePitch = nSoftFXImageWidth * nVidImageBPP;

	

#if defined BUILD_X86_ASM
	if (nSoftFXBlitter >= FILTER_HQ2X && nSoftFXBlitter <= FILTER_HQ4X) {
		int i, j, k, r, g, b, Y, u, v;

		if (nVidImageDepth == 15) {
			for (i = 0; i < 32768; i++) {
				LUT16to32[i] = ((i & 0x7C00) << 9) + ((i & 0x03E0) << 6) + ((i & 0x001F) << 3);
			}

			for (i = 0; i < 32; i++) {
				for (j = 0; j < 32; j++) {
					for (k = 0; k < 32; k++) {
						r = i << 3;
						g = j << 3;
						b = k << 3;
						Y = (r + g + b) >> 2;
						u = 128 + ((r - b) >> 2);
						v = 128 + ((-r + 2 * g - b) >> 3);
						RGBtoYUV[(i << 10) + (j << 5) + k] = (Y << 16) + (u << 8) + v;
					}
				}
			}
		} else {
			for (i = 0; i < 65536; i++) {
				LUT16to32[i] = ((i & 0xF800) << 8) + ((i & 0x07E0) << 5) + ((i & 0x001F) << 3);
			}

			for (i = 0; i < 32; i++) {
				for (j = 0; j < 64; j++) {
					for (k = 0; k < 32; k++) {
						r = i << 3;
						g = j << 2;
						b = k << 3;
						Y =	(r + g + b) >> 2;
						u = 128 + ((r - b) >> 2);
						v = 128 + ((-r + 2 * g - b) >> 3);
						RGBtoYUV[(i << 11) + (j << 5) + k] = (Y << 16) + (u << 8) + v;
					}
				}
			}
		}
	}
#endif
	
	

#ifdef PRINT_DEBUG_INFO
   	dprintf(_T("  * SoftFX initialised: using %s in %i-bit mode.\n"), SoftFXInfo[nSoftFXBlitter].pszName, nVidImageDepth);
#endif

	return 0;
}

int VidSoftFXScale(RECT* pRect, int nGameWidth, int nGameHeight)
{
	int nWidth = pRect->right - pRect->left;
	int nHeight = pRect->bottom - pRect->top;

	nSoftFXEnlarge = false;

	if ((nWidth >= (nGameWidth * SoftFXInfo[nSoftFXBlitter].nZoom)) && (nHeight >= (nGameHeight * SoftFXInfo[nSoftFXBlitter].nZoom))) {
		nWidth = nGameWidth * SoftFXInfo[nSoftFXBlitter].nZoom;
		nHeight = nGameHeight * SoftFXInfo[nSoftFXBlitter].nZoom;
		nSoftFXEnlarge = true;
	}

	if (!nSoftFXEnlarge) {
		nWidth = nGameWidth;
		nHeight = nGameHeight;
	}

	pRect->left = (pRect->right + pRect->left) / 2;
	pRect->left -= nWidth / 2;
	pRect->right = pRect->left + nWidth;

	pRect->top = (pRect->bottom + pRect->top) / 2;
	pRect->top -= nHeight / 2;
	pRect->bottom = pRect->top + nHeight;

	return 0;
}

static void VidSoftFXRotate()
{
	if (nSoftFXRotate) {
		unsigned char* ps;
		unsigned char* pd = pSoftFXImage;
		if (nSoftFXRotate & 2) {
			pd += nSoftFXImageHeight * nSoftFXImagePitch - nVidImageBPP;
		}

		switch (nVidImageBPP) {
			case 4:	{
				switch (nSoftFXRotate) {
					case 1: {
						for (int y = 0; y < nSoftFXImageHeight; y++) {
							ps = pVidImage + (nSoftFXImageHeight + nVidImageLeft - 1 - y) * 4;
							for (int x = 0; x < nSoftFXImageWidth; x++) {
								*(int*)pd = *(int*)ps;
								ps += nVidImagePitch;
								pd += 4;
							}
						}
						break;
					}
					case 2: {
						for (int y = 0; y < nSoftFXImageHeight; y++) {
							ps = pVidImage + y * nVidImagePitch + nVidImageLeft * 2;
							for (int x = 0; x < nSoftFXImageWidth; x++) {
								*(int*)pd = *(int*)ps;
								ps += 4;
								pd -= 4;
							}
						}
						break;
					}
					case 3: {
						for (int y = 0; y <nSoftFXImageHeight; y++) {
							ps = pVidImage + (nSoftFXImageHeight + nVidImageLeft - 1 - y) * 4;
							for (int x = 0; x < nSoftFXImageWidth; x++) {
								*(int*)pd = *(int*)ps;
								ps += nVidImagePitch;
								pd -= 4;
							}
						}
						break;
					}
				}
				break;
			}

			case 3: {
				switch (nSoftFXRotate) {
					case 1: {
						for (int y = 0; y < nSoftFXImageHeight; y++) {
							ps = pVidImage + (nSoftFXImageHeight + nVidImageLeft - 1 - y) * 3;
							for (int x = 0; x < nSoftFXImageWidth; x++) {
								pd[0] = ps[0];
								pd[1] = ps[2];
								pd[2] = ps[2];
								ps += nVidImagePitch;
								pd += 3;
							}
						}
						break;
					}
					case 2: {
						for (int y = 0; y < nSoftFXImageHeight; y++) {
							ps = pVidImage + y * nVidImagePitch + nVidImageLeft * 3;
							for (int x = 0; x < nSoftFXImageWidth; x++) {
								pd[0] = ps[0];
								pd[1] = ps[2];
								pd[2] = ps[2];
								ps += 3;
								pd -= 3;
							}
						}
						break;
					}
					case 3: {
						for (int y = 0; y < nSoftFXImageHeight; y++) {
							ps = pVidImage + (nSoftFXImageHeight + nVidImageLeft - 1 - y) * 3;
							for (int x = 0; x < nSoftFXImageWidth; x++) {
								pd[0] = ps[0];
								pd[1] = ps[2];
								pd[2] = ps[2];
								ps += nVidImagePitch;
								pd -= 3;
							}
						}
						break;
					}
				}
				break;
			}

			case 2:	{
				switch (nSoftFXRotate) {
					case 1: {
						for (int y = 0; y < nSoftFXImageHeight; y++) {
							ps = pVidImage + (nSoftFXImageHeight + nVidImageLeft - 1 - y) * 2;
							for (int x = 0; x < nSoftFXImageWidth; x++) {
								*(short*)pd = *(short*)ps;
								ps += nVidImagePitch;
								pd += 2;
							}
						}
						break;
					}
					case 2: {
						for (int y = 0; y < nSoftFXImageHeight; y++) {
							ps = pVidImage + y * nVidImagePitch + nVidImageLeft * 2;
							for (int x = 0; x < nSoftFXImageWidth; x++) {
								*(short*)pd = *(short*)ps;
								ps += 2;
								pd -= 2;
							}
						}
						break;
					}
					case 3: {
						for (int y = 0; y < nSoftFXImageHeight; y++) {
							ps = pVidImage + (nSoftFXImageHeight + nVidImageLeft - 1 - y) * 2;
							for (int x = 0; x < nSoftFXImageWidth; x++) {
								*(short*)pd = *(short*)ps;
								ps += nVidImagePitch;
								pd -= 2;
							}
						}
						break;
					}
				}
				break;
			}
		}
	}
}

void VidSoftFXApplyEffect(unsigned char* ps, unsigned char* pd, int nPitch)
{
	// Apply effects to the image
	switch (nSoftFXBlitter) {

		case FILTER_PLAIN: {											// Software 2x zoom
			if (nVidImageBPP == 2) {						// 15/16-bit
				for (int y = 0; y < nSoftFXImageHeight; y++, pd += (nPitch << 1), ps += nSoftFXImagePitch) {
					unsigned short* psEnd = (unsigned short*)(ps + nSoftFXImagePitch);
					unsigned short* pdpc = (unsigned short*)pd;
					unsigned short* pdpn = (unsigned short*)(pd + nPitch);
					unsigned short* psp = (unsigned short*)ps;
					do {
						*pdpc++ = *psp;
						*pdpc++ = *psp;
						*pdpn++ = *psp;
						*pdpn++ = *psp++;
					} while (psp < psEnd);
				}
			} else {										// 32-bit
				if (nVidImageBPP == 4) {
					for (int y = 0; y < nSoftFXImageHeight; y++, pd += (nPitch << 1), ps += nSoftFXImagePitch) {
						unsigned int* psEnd = (unsigned int*)(ps + nSoftFXImagePitch);
						unsigned int* pdpc = (unsigned int*)pd;
						unsigned int* pdpn = (unsigned int*)(pd + nPitch);
						unsigned int* psp = (unsigned int*)ps;
						do {
							*pdpc++ = *psp;
							*pdpc++ = *psp;
							*pdpn++ = *psp;
							*pdpn++ = *psp++;
						} while (psp < psEnd);
					}
				} else {									// 24-bit
					for (int y = 0; y < nSoftFXImageHeight; y++, pd += (nPitch << 1), ps += nSoftFXImagePitch) {
						unsigned char* psEnd = (unsigned char*)(ps + nSoftFXImagePitch);
						unsigned char* pdpc = (unsigned char*)pd;
						unsigned char* pdpn = (unsigned char*)(pd + nPitch);
						unsigned char* psp = (unsigned char*)ps;
						do {
							pdpc[0] = psp[0];
							pdpc[3] = psp[0];
							pdpn[0] = psp[0];
							pdpn[3] = psp[0];
							pdpc[1] = psp[1];
							pdpc[4] = psp[1];
							pdpn[1] = psp[1];
							pdpn[4] = psp[1];
							pdpc[2] = psp[2];
							pdpc[5] = psp[2];
							pdpn[2] = psp[2];
							pdpn[5] = psp[2];
							psp += 3;
							pdpc += 3;
							pdpn += 3;
						} while (psp < psEnd);
					}
				}
			}
			break;
		}

	}
}

#ifdef SOFTFX_ENABLE_DIRECTX

int VidSoftFXApplyEffectDirectX(IDirectDrawSurface7* pSurf, RECT* pRect)
{
	DDSURFACEDESC2 ddsd;

	VidSoftFXRotate();

	// Lock the surface so we can write to it
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	if (FAILED(pSurf->Lock(pRect, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL))) {
		return 1;
	}

	VidSoftFXApplyEffect(pSoftFXImage, (unsigned char*)ddsd.lpSurface, ddsd.lPitch);

	pSurf->Unlock(NULL);

	return 0;
}

#endif

#ifdef SOFTFX_ENABLE_SDL

int VidSoftFXApplyEffectSDL(SDL_Surface* pSurf)
{
	VidSoftFXRotate();

	// Lock the surface so we can write to it
	if (SDL_LockSurface(pSurf)) {
		return 1;
	}

	VidSoftFXApplyEffect(pSoftFXImage, (unsigned char*)pSurf->pixels, pSurf->pitch);

	SDL_UnlockSurface(pSurf);

	return 0;
}

#endif

int VidFilterApplyEffect(unsigned char* pd, int pitch)
{
	if (!pd) {
		return 1;
	}

	VidSoftFXRotate();
	VidSoftFXApplyEffect(pSoftFXImage, pd, pitch);

	return 0;
}
