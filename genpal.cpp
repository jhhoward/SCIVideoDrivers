#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <memory.h>
#include "lodepng.cpp"

#define USE_ALL_DITHERS 1
#define USE_MAGENTA_PALETTE 0
#define USE_BRIGHT_PALETTE 1

#define MAX_COMPOSITE_DITHER_DISTANCE 180 //160

#define MAX_RGB_INTENSITY 0xaa

using namespace std;

bool dumpPalettes = false;

struct PaletteHeader 
{
	char riff[4];
	uint32_t size;
	char form[4];
};

struct PaletteEntry 
{
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t flags;
};

struct ChunkHeader 
{
  char chunk[4];
  uint32_t size;
  uint16_t version;
  uint16_t numEntries;
};

// black / red / cyan / white
uint8_t palette_brcw[] =
{
	0, 0x0, 0x0,
	0x55, 0xff, 0xff,
	0xff, 0x55, 0x55,
	0xff, 0xff, 0xff
};

// blue / red / cyan / grey
uint8_t palette_brcg[] =
{
	0, 0x0, 0x00,
	0x55, 0xff, 0xff,
	0xff, 0x55, 0x55,
	0xff, 0xff, 0xff
};

// black / magenta / cyan / white
uint8_t palette_bmcw[] =
{
	0, 0, 0x0,
	0x55, 0xff, 0xff,
	0xff, 0x55, 0xff,
	0xff, 0xff, 0xff
	
//	0, 0xaa, 0x0,
//	0x55, 0xff, 0xff,
//	0xff, 0x55, 0xff,
//	0xff, 0xff, 0xff
//	0xff, 0xff, 0x55,
//	0x00, 0xaa, 0xaa,
//	0xaa, 0x00, 0xaa,
//	0xaa, 0xaa, 0xaa
};

// yellow / green / red / brown
uint8_t palette_cgrb[] =
{
	0xff, 0xff, 0x55,
	0x00, 0xaa, 0x00,
	0xaa, 0x00, 0x00,
	0xaa, 0x55, 0x00
};


// blue / green / red / brown
uint8_t palette_bgrb[] =
{
	0, 0, 0xaa,
	0x00, 0xaa, 0x00,
	0xaa, 0x00, 0x00,
	0xaa, 0x55, 0x00
};

// blue / green / red / yellow
uint8_t palette_bgry[] =
{
	0, 0, 0x55,
	0x55, 0xff, 0x55,
	0xff, 0x55, 0x55,
	0xff, 0xff, 0x55
};

// black / green / red / yellow
uint8_t palette_bkgry[] =
{
	0, 0, 0,
	0x55, 0xff, 0x55,
	0xff, 0x55, 0x55,
	0xff, 0xff, 0x55
};


#if 0
#if USE_BRIGHT_PALETTE
uint8_t palette[] =
{
	0, 0, 0x0,
	0x55, 0xff, 0xff,
#if USE_MAGENTA_PALETTE
	0xff, 0x55, 0xff,
#else
	0xff, 0x55, 0x55,
#endif
	0xff, 0xff, 0xff
};
#else
uint8_t palette[] =
{
	0, 0, 0x0,
	0x00, 0xaa, 0xaa,
#if USE_MAGENTA_PALETTE
	0xaa, 0x00, 0xaa,
#else
	0xaa, 0x00, 0x00,
#endif
	0xaa, 0xaa, 0xaa
};
#endif
#endif

#define NUM_EXTENDED_DITHER_PATTERNS (sizeof(extendedDitherPatterns) / 8)
#define EXTENDED_DITHER_PATTERN_SIZE 8
uint8_t extendedDitherPatterns[] =
{
	0, 0, 0, 0,
	0, 0, 0, 0,

	1, 1, 1, 1,
	1, 1, 1, 1,

	2, 2, 2, 2,
	2, 2, 2, 2,
	
	3, 3, 3, 3,
	3, 3, 3, 3,
		
	0, 0, 0, 1,
	0, 1, 0, 0,

	1, 0, 1, 0,
	0, 1, 0, 1,

	1, 1, 1, 0,
	1, 0, 1, 1,

	1, 1, 1, 3,
	1, 3, 1, 1,
	
	1, 3, 1, 3,
	3, 1, 3, 1,
	
	3, 3, 3, 1,
	3, 1, 3, 3,
	
	0, 0, 0, 2,
	0, 2, 0, 0,
	
	2, 0, 2, 0,
	0, 2, 0, 2,
	
	2, 2, 2, 0,
	2, 0, 2, 2,
	
	2, 2, 2, 3,
	2, 3, 2, 2,
	
	2, 3, 2, 3,
	3, 2, 3, 2,
	
	3, 3, 3, 2,
	3, 2, 3, 3,
	
	0, 0, 0, 3,
	0, 3, 0, 0,

	3, 0, 3, 0,
	0, 3, 0, 3,

	3, 3, 3, 0,
	3, 0, 3, 3,
	
	/*
	1, 2, 1, 3,
	1, 3, 1, 2,

	2, 1, 2, 3,
	2, 3, 2, 1,

	1, 2, 1, 0,
	1, 0, 1, 2,

	2, 1, 2, 0,
	2, 0, 2, 1,

	3, 1, 3, 1,
	2, 3, 2, 3,

	0, 1, 0, 2,
	0, 2, 0, 1,

	1, 2, 1, 2,
	2, 1, 2, 1,

	3, 0, 3, 1,
	3, 1, 3, 0,

	0, 3, 0, 1,
	0, 1, 0, 3,

	3, 0, 3, 2,
	3, 2, 3, 0,

	0, 3, 0, 2,
	0, 2, 0, 3,

	0, 3, 1, 2,
	1, 2, 0, 3,
	*/
};


#define NUM_PATTERNS (sizeof(patterns) / 4)
uint8_t patterns[] =
{
#if USE_ALL_DITHERS
	0, 0, 0, 0,
	1, 1, 1, 1,
	2, 2, 2, 2,
	3, 3, 3, 3,

	0, 3, 0, 0,
	0, 3, 0, 3,
	3, 3, 0, 3,

	0, 1, 0, 0,
	0, 1, 0, 1,
	1, 1, 0, 1,

	0, 2, 0, 0,
	0, 2, 0, 2,
	2, 2, 0, 2,

	1, 1, 3, 1,
	3, 1, 3, 1,
	3, 1, 3, 3,

	2, 2, 3, 2,
	3, 2, 3, 2,
	3, 2, 3, 3,

	1, 2, 1, 1,
	1, 2, 1, 2,
	2, 2, 1, 2,


//	0, 0, 0, 0,
//	0, 1, 0, 0,
//	0, 1, 0, 1,
//	1, 1, 0, 1,
//	1, 1, 1, 1,
//	1, 1, 3, 1,
//	3, 1, 3, 1,
//	3, 1, 3, 3,
//	0, 2, 0, 0,
//	0, 2, 0, 2,
//	2, 2, 0, 2,
//	2, 2, 2, 2,
//	2, 2, 3, 2,
//	3, 2, 3, 2,
//	3, 2, 3, 3,
//	3, 3, 3, 3,
//	0, 0, 0, 0,
//	0, 3, 0, 0,
//	0, 3, 0, 3,
//	3, 3, 0, 3,
//	1, 2, 1, 2,
//	1, 2, 1, 1,
//	2, 2, 1, 2,

#elif USE_NO_DITHERS
	0, 0, 0, 0,
	1, 1, 1, 1,
	2, 2, 2, 2,
	3, 3, 3, 3,
#else
	0, 0, 0, 0,
	0, 1, 0, 1,
	3, 1, 3, 1,
	1, 1, 1, 1,
	0, 2, 0, 2,
	3, 2, 3, 2,
	2, 2, 2, 2,
	0, 3, 0, 3,
	3, 3, 3, 3,
#endif
};

#define NUM_GREY_PATTERNS (sizeof(greyPatterns) / 4)
uint8_t greyPatterns[] =
{
	0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 1, 0, 0, 0,
	1, 0, 1, 0, 1, 0, 0, 0,
	1, 0, 1, 0, 1, 0, 1, 0,
	1, 1, 1, 0, 1, 0, 1, 0,
	1, 1, 1, 0, 1, 1, 1, 0,
	1, 1, 1, 1, 1, 1, 1, 0,
	1, 1, 1, 1, 1, 1, 1, 1
};

int greyscaleKernel[] =
{
	1, 13, 3, 15, 5, 7, 11, 9,
	6, 10, 8, 12, 2, 14, 4, 16,
};

// When using 640x200 mode with colour burst enabled
uint8_t compositePalette[] =
{
	0x00, 0x00, 0x00,
	0x00, 0x6e, 0x31,
	0x31, 0x09, 0xff,
	0x00, 0x8a,	0xff,
	0xa7, 0x00, 0x31,
	0x76, 0x76, 0x76,
	0xec, 0x11, 0xff,
	0xbb, 0x92, 0xff,
	0x31, 0x5a, 0x00,
	0x00, 0xdb, 0x00,
	0x76, 0x76, 0x76,
	0x45, 0xf7, 0xbb,
	0xec, 0x63, 0x00,
	0xbb, 0xe4, 0x00,
	0xff, 0x7f, 0xbb,
	0xff, 0xff, 0xff
};

// When using cyan / magenta low intensity with white for border colour
uint8_t compositePaletteAlt[] =
{
	0xef, 0xef, 0xef,
	0xe6, 0xbf, 0x8d,
	0xff, 0x50, 0x9f,
	0xef, 0xca,	0x93,
	0x00, 0xb5, 0xc0,
	0x00, 0x8e, 0x5a,
	0x0a, 0x1a, 0x74,
	0x00, 0x8e, 0x67,
	0x6c, 0xa9, 0xff,
	0x68, 0x7b, 0xd4,
	0x98, 0x14, 0xf6,
	0x6c, 0x84, 0xd9,
	0xb0, 0xd4, 0xff,
	0xa1, 0x9c, 0x9d,
	0xca, 0x34, 0xb6,
	0xab, 0xab, 0xab
};

// When using cyan / magenta in composite mode
uint8_t compositePaletteAlt2[] =
{
	0x00, 0x00, 0x00,
	0x00, 0x9a, 0xff,
	0x00, 0x42, 0xff,
	0x00, 0x90,	0xff,
	0xaa, 0x4c, 0x00,
	0x84, 0xfa, 0xd2,
	0xb9, 0xa2, 0xad,
	0x96, 0xf0, 0xff,
	0xcd, 0x1f, 0x00,
	0xa7, 0xcd, 0xff,
	0xdc, 0x7f, 0xff,
	0xb9, 0xc3, 0xff,
	0xff, 0x5c, 0x00,
	0xed, 0xff, 0xcc,
	0xff, 0xb2, 0xa6,
	0xff, 0xff, 0xff
};


uint8_t patternsRGB[NUM_PATTERNS * 3];
uint8_t patternsRGBWeights[NUM_PATTERNS];
uint8_t convertLUT[256];

uint8_t compositePatternRGB[256 * 3];
uint8_t compositePatternRGBWeights[256];

uint8_t greyPatternRGB[256 * 3];

uint8_t extendedDitherPatternRGB[NUM_EXTENDED_DITHER_PATTERNS * 3];
uint8_t extendedDitherPatternRGBWeights[NUM_EXTENDED_DITHER_PATTERNS];

void GenerateExtendedDitherPatternsRGB(uint8_t* palette)
{
	for(int n = 0; n < NUM_EXTENDED_DITHER_PATTERNS; n++)
	{
		int r = 0, g = 0, b = 0;
		uint8_t weight = 0;
		int differences = 0;
		
		for(int i = 0; i < EXTENDED_DITHER_PATTERN_SIZE; i++)
		{
			int index = extendedDitherPatterns[n * EXTENDED_DITHER_PATTERN_SIZE + i];
			r += palette[index * 3];
			g += palette[index * 3 + 1];
			b += palette[index * 3 + 2];
			
			if(index != extendedDitherPatterns[n * EXTENDED_DITHER_PATTERN_SIZE])
			{
				differences++;
			}
		}
		
		switch(differences)
		{
			case 0:
			weight = 1;
			break;
			case EXTENDED_DITHER_PATTERN_SIZE / 2:
			weight = 3;
			break;
			default:
			weight = 6;
			break;
		}

		weight = differences ? 10 : 7;
		
		
		r /= EXTENDED_DITHER_PATTERN_SIZE;
		g /= EXTENDED_DITHER_PATTERN_SIZE;
		b /= EXTENDED_DITHER_PATTERN_SIZE;
		
		extendedDitherPatternRGB[n * 3] = (uint8_t) r;
		extendedDitherPatternRGB[n * 3 + 1] = (uint8_t) g;
		extendedDitherPatternRGB[n * 3 + 2] = (uint8_t) b;
		extendedDitherPatternRGBWeights[n] = weight;
		
	}
}


void GeneratePatternsRGB(uint8_t* palette)
{
	for(int n = 0; n < NUM_PATTERNS; n++)
	{
		int red = 0, green = 0, blue = 0;
		
		for(int i = 0; i < 4; i++)
		{
			uint8_t p = patterns[n * 4 + i];
			red += palette[p * 3];
			green += palette[p * 3 + 1];
			blue += palette[p * 3 + 2];
		}
		patternsRGB[n * 3] = (uint8_t)(red / 4);
		patternsRGB[n * 3 + 1] = (uint8_t)(green / 4);
		patternsRGB[n * 3 + 2] = (uint8_t)(blue / 4);
		
		int weight = 1;
		if(patterns[n * 4] != patterns[n * 4 + 2] || patterns[n * 4 + 1] != patterns[n * 4 + 3])
		{
		//	weight ++;
		}
		if(patterns[n * 4] != patterns[n * 4 + 1])
		{
			//weight ++;
		}			
		patternsRGBWeights[n] = weight;
	}
	
	vector<uint8_t> palettePixels;
	for(int n = 0; n < NUM_PATTERNS; n++)
	{
		palettePixels.push_back(patternsRGB[n * 3]);
		palettePixels.push_back(patternsRGB[n * 3 + 1]);
		palettePixels.push_back(patternsRGB[n * 3 + 2]);
		palettePixels.push_back(255);
	}
	
	if(dumpPalettes)
	{
		lodepng::encode("palette.png", palettePixels, NUM_PATTERNS, 1);
	}
}

void GenerateGreyPatternsRGB()
{
	for(int n = 0; n < NUM_GREY_PATTERNS; n++)
	{
		int intensity = 0;
		for(int i = 0; i < 8; i++)
		{
			uint8_t p = greyPatterns[n * 8 + i];
			
			if(p)
			{
				intensity++;
			}
		}
		
		intensity = (intensity * 255) / 8;
		
		greyPatternRGB[n * 3] = (uint8_t)(intensity);
		greyPatternRGB[n * 3 + 1] = (uint8_t)(intensity);
		greyPatternRGB[n * 3 + 2] = (uint8_t)(intensity);
	}
}

float max(float a, float b, float c) {
   return ((a > b)? (a > c ? a : c) : (b > c ? b : c));
}
float min(float a, float b, float c) {
   return ((a < b)? (a < c ? a : c) : (b < c ? b : c));
}
int RGBtoHSV(uint8_t* rgb, float* hsv) {
   float r = rgb[0] / 255.0f;
   float g = rgb[1] / 255.0f;
   float b = rgb[2] / 255.0f;
   float cmax = max(r, g, b); // maximum of r, g, b
   float cmin = min(r, g, b); // minimum of r, g, b
   float diff = cmax-cmin; // diff of cmax and cmin.
   if (cmax == cmin)
      hsv[0] = 0;
   else if (cmax == r)
      hsv[0] = fmod((60 * ((g - b) / diff) + 360), 360.0);
   else if (cmax == g)
      hsv[0] = fmod((60 * ((b - r) / diff) + 120), 360.0);
   else if (cmax == b)
      hsv[0] = fmod((60 * ((r - g) / diff) + 240), 360.0);
  
  hsv[0] /= 360.0f;
  
   // if cmax equal zero
  if (cmax == 0)
	 hsv[1] = 0;
  else
	 hsv[1] = (diff / cmax);
   // compute v
   hsv[2] = cmax;

   return 0;
}

#define USE_HSV_COMPARISON 0

int FindClosestPaletteEntry(uint8_t* rgb, uint8_t* pal, int palSize, uint8_t* weights = NULL)
{
	int closest = -1;
	float closestDistance = 0;
	float inHSV[3];
	
	if(USE_HSV_COMPARISON)
	{
		RGBtoHSV(rgb, inHSV);
	}
	
	for(int n = 0; n < palSize; n++)
	{
		float distance = 0;

		if(USE_HSV_COMPARISON)
		{
			//const float hWeight = 4.0f * inHSV[1];
			//const float sWeight = 1.0f;
			//const float vWeight = 2.0f;

			const float hWeight = 0.5f * inHSV[1];
			const float sWeight = 1.0f;
			const float vWeight = 5.0f;
			
			float palHSV[3];
			RGBtoHSV(pal + n * 3, palHSV);
			
			float hDiff = fabs(inHSV[0] - palHSV[0]);
			if(hDiff > 0.5f)
				hDiff = 1.0f - hDiff;
			
			distance += hDiff * hWeight;
			
			float sDiff = fabs(inHSV[1] - palHSV[1]);
			distance += sDiff * sWeight;
			
			float vDiff = fabs(inHSV[2] - palHSV[2]);
			distance += vDiff * vWeight;
		}
		else
		{
			for(int i = 0; i < 3; i++)
			{
				int diff = rgb[i] - pal[n * 3 + i];
				distance += diff * diff;
			}
		}
		
		if(weights)
		{
			distance *= weights[n];
		}
		
		if(distance < 0)
		{
			continue;
		}
		
		if(closest == -1 || distance < closestDistance)
		{
			closest = n;
			closestDistance = distance;
		}
	}
	
	return closest;
}

void GenerateLUT(PaletteEntry* entries)
{
	vector<uint8_t> srcPalOut;
	vector<uint8_t> dstPalOut;

	for(int index = 0; index < 256; index++)
	{
		uint8_t rgb[3];
		rgb[0] = entries[index].red;
		rgb[1] = entries[index].green;
		rgb[2] = entries[index].blue;
		
		srcPalOut.push_back(entries[index].red);
		srcPalOut.push_back(entries[index].green);
		srcPalOut.push_back(entries[index].blue);
		srcPalOut.push_back(255);
		
		int match = FindClosestPaletteEntry(rgb, patternsRGB, NUM_PATTERNS, patternsRGBWeights);
		uint8_t pattern = 0;
		
		for(int n = 0; n < 4; n++)
		{
			pattern <<= 2;
			pattern |= (uint8_t)(patterns[match * 4 + n]);
		}
		convertLUT[index] = pattern;
		
		dstPalOut.push_back(patternsRGB[match * 3]);
		dstPalOut.push_back(patternsRGB[match * 3 + 1]);
		dstPalOut.push_back(patternsRGB[match * 3 + 2]);
		dstPalOut.push_back(255);
	}
	
	if(dumpPalettes)
	{
		lodepng::encode("srcpal.png", srcPalOut, 16, 16);
		lodepng::encode("dstpal.png", dstPalOut, 16, 16);
	}
	
	FILE* fs = fopen("pal.txt", "w");
	if(fs)
	{
		for(int y = 0; y < 16; y++)
		{
			fprintf(fs, "\tdb\t");
			for(int x = 0; x < 16; x++)
			{
				int index = y * 16 + x;
				fprintf(fs, "%d,", convertLUT[index]);
			}
			fprintf(fs, "\n");
		}
	}
	fclose(fs);
}

float RGBtoGreyscale(uint8_t rgb[3])
{
	return 0.299f * rgb[0] + 0.587f * rgb[1] + 0.114 * rgb[2];
}

void GeneratePaletteLookupTables()
{
	vector<uint8_t> srcPalOut;
	vector<uint8_t> dstPalOut;
	uint16_t patternLookup[256];
	uint16_t compositePatternLookup[256];
	uint16_t greyPatternLookup[256];
	uint8_t intensityMask[101];
	
	int index = 0;

	for(int r = 0; r < 8; r++)
	{
		for(int g = 0; g < 4; g++)
		{
			for(int b = 0; b < 8; b++)
			{
				uint8_t rgb[3];
				rgb[0] = (r * 255) / 7;
				rgb[1] = (g * 255) / 3;
				rgb[2] = (b * 255) / 7;
				
				srcPalOut.push_back(rgb[0]);
				srcPalOut.push_back(rgb[1]);
				srcPalOut.push_back(rgb[2]);
				srcPalOut.push_back(255);
				
				
				int match = FindClosestPaletteEntry(rgb, extendedDitherPatternRGB, NUM_EXTENDED_DITHER_PATTERNS, extendedDitherPatternRGBWeights);
				uint16_t pattern = 0;
				
				for(int n = 0; n < 8; n++)
				{
					pattern <<= 2;
					pattern |= (uint8_t)(extendedDitherPatterns[match * 8 + n]);
				}
				
				patternLookup[index] = pattern;
				//convertLUT[index] = pattern;
				
				dstPalOut.push_back(extendedDitherPatternRGB[match * 3]);
				dstPalOut.push_back(extendedDitherPatternRGB[match * 3 + 1]);
				dstPalOut.push_back(extendedDitherPatternRGB[match * 3 + 2]);
				dstPalOut.push_back(255);
				
				//int compositeMatch = FindClosestPaletteEntry(rgb, compositePalette, 16);
				//uint8_t compositePattern = (uint8_t)(compositeMatch | (compositeMatch << 4));
				int compositePattern = FindClosestPaletteEntry(rgb, compositePatternRGB, 256, compositePatternRGBWeights);
				uint16_t compositePatternOutput = (uint16_t) (compositePattern << 8);
				compositePatternOutput |= (compositePattern >> 4) | ((compositePattern & 0xf) << 4);
				
				compositePatternLookup[index] = compositePatternOutput;
				
				int greyscale = (int) (RGBtoGreyscale(rgb) * 16 / 255);
				
				uint16_t greyPattern = 0;
				
				for(int n = 0; n < 16; n++)
				{
					if(greyscale > greyscaleKernel[n])
					{
						greyPattern |= (1 << n);
					}
				}
				//int greyPatternIndex = FindClosestPaletteEntry(rgb, greyPatternRGB, 256);
				//uint8_t greyPattern = 0;
				//for(int n = 0; n < 8; n++)
				//{
				//	if(greyPatterns[greyPatternIndex * 8 + n])
				//	{
				//		greyPattern |= (0x80 >> n);
				//	}
				//}
				greyPatternLookup[index] = greyPattern;
				
				index++;
			}
		}
	}
	
	for(int n = 0; n <= 100; n++)
	{
		int r = (7 * n) / 100;
		int g = (3 * n) / 100;
		int b = (7 * n) / 100;
		
		intensityMask[n] = (uint8_t) (b | (g << 3) | (r << 5));
	}
	
	if(dumpPalettes)
	{
		lodepng::encode("srcpal6bpp.png", srcPalOut, 16, 16);
		lodepng::encode("dstpal6bpp.png", dstPalOut, 16, 16);
	}

	FILE* fs = fopen("cgargb_tables.i", "w");
	if(fs)
	{
		fprintf(fs, "convert_323_palette\t");
		for(int y = 0; y < 16; y++)
		{
			fprintf(fs, "\tdw\t");
			for(int x = 0; x < 16; x++)
			{
				int index = y * 16 + x;
				fprintf(fs, "%d,", patternLookup[index]);
			}
			fprintf(fs, "\n");
		}

		fprintf(fs, "\nintensity_mask\t");
		for(int y = 0; y < 10; y++)
		{
			fprintf(fs, "\tdb\t");
			for(int x = 0; x < 10; x++)
			{
				int index = y * 10 + x;
				fprintf(fs, "%d,", intensityMask[index]);
			}
			
			if(y == 9)
			{
				fprintf(fs, "%d", intensityMask[100]);
			}
			fprintf(fs, "\n");
		}
	}
	fclose(fs);

	fs = fopen("cgacomp_tables.i", "w");
	if(fs)
	{
		fprintf(fs, "convert_323_palette\t");
		for(int y = 0; y < 16; y++)
		{
			fprintf(fs, "\tdw\t");
			for(int x = 0; x < 16; x++)
			{
				int index = y * 16 + x;
				fprintf(fs, "%d,", compositePatternLookup[index]);
			}
			fprintf(fs, "\n");
		}
		fprintf(fs, "\nintensity_mask\t");
		for(int y = 0; y < 10; y++)
		{
			fprintf(fs, "\tdb\t");
			for(int x = 0; x < 10; x++)
			{
				int index = y * 10 + x;
				fprintf(fs, "%d,", intensityMask[index]);
			}

			if(y == 9)
			{
				fprintf(fs, "%d", intensityMask[100]);
			}
			fprintf(fs, "\n");
		}
	}
	fclose(fs);


	fs = fopen("cgagrey_tables.i", "w");
	if(fs)
	{
		fprintf(fs, "convert_323_palette\t");
		for(int y = 0; y < 16; y++)
		{
			fprintf(fs, "\tdw\t");
			for(int x = 0; x < 16; x++)
			{
				int index = y * 16 + x;
				fprintf(fs, "%d,", greyPatternLookup[index]);
			}
			fprintf(fs, "\n");
		}
		fprintf(fs, "\nintensity_mask\t");
		for(int y = 0; y < 10; y++)
		{
			fprintf(fs, "\tdb\t");
			for(int x = 0; x < 10; x++)
			{
				int index = y * 10 + x;
				fprintf(fs, "%d,", intensityMask[index]);
			}

			if(y == 9)
			{
				fprintf(fs, "%d", intensityMask[100]);
			}
			fprintf(fs, "\n");
		}
	}
	fclose(fs);
}

/*void GeneratePaletteLookupTables()
{
	vector<uint8_t> srcPalOut;
	vector<uint8_t> dstPalOut;
	uint8_t patternLookup[256];
	uint8_t compositePatternLookup[256];
	uint8_t greyPatternLookup[256];
	uint8_t intensityMask[101];
	
	int index = 0;

	for(int r = 0; r < 8; r++)
	{
		for(int g = 0; g < 4; g++)
		{
			for(int b = 0; b < 8; b++)
			{
				uint8_t rgb[3];
				rgb[0] = (r * 255) / 7;
				rgb[1] = (g * 255) / 3;
				rgb[2] = (b * 255) / 7;
				
				srcPalOut.push_back(rgb[0]);
				srcPalOut.push_back(rgb[1]);
				srcPalOut.push_back(rgb[2]);
				srcPalOut.push_back(255);
				
				
				int match = FindClosestPaletteEntry(rgb, patternsRGB, NUM_PATTERNS, patternsRGBWeights);
				uint8_t pattern = 0;
				
				for(int n = 0; n < 4; n++)
				{
					pattern <<= 2;
					pattern |= (uint8_t)(patterns[match * 4 + n]);
				}
				
				patternLookup[index] = pattern;
				//convertLUT[index] = pattern;
				
				dstPalOut.push_back(patternsRGB[match * 3]);
				dstPalOut.push_back(patternsRGB[match * 3 + 1]);
				dstPalOut.push_back(patternsRGB[match * 3 + 2]);
				dstPalOut.push_back(255);
				
				//int compositeMatch = FindClosestPaletteEntry(rgb, compositePalette, 16);
				//uint8_t compositePattern = (uint8_t)(compositeMatch | (compositeMatch << 4));
				uint8_t compositePattern = (uint8_t) FindClosestPaletteEntry(rgb, compositePatternRGB, 256, compositePatternRGBWeights);
				compositePatternLookup[index] = compositePattern;
				
				int greyPatternIndex = FindClosestPaletteEntry(rgb, greyPatternRGB, 256);
				uint8_t greyPattern = 0;
				for(int n = 0; n < 8; n++)
				{
					if(greyPatterns[greyPatternIndex * 8 + n])
					{
						greyPattern |= (0x80 >> n);
					}
				}
				greyPatternLookup[index] = greyPattern;
				
				index++;
			}
		}
	}
	
	for(int n = 0; n <= 100; n++)
	{
		int r = (7 * n) / 100;
		int g = (3 * n) / 100;
		int b = (7 * n) / 100;
		
		intensityMask[n] = (uint8_t) (b | (g << 3) | (r << 5));
	}
	
	if(dumpPalettes)
	{
		lodepng::encode("srcpal6bpp.png", srcPalOut, 16, 16);
		lodepng::encode("dstpal6bpp.png", dstPalOut, 16, 16);
	}

	FILE* fs = fopen("cgargb_tables.i", "w");
	if(fs)
	{
		fprintf(fs, "convert_323_palette\t");
		for(int y = 0; y < 16; y++)
		{
			fprintf(fs, "\tdb\t");
			for(int x = 0; x < 16; x++)
			{
				int index = y * 16 + x;
				fprintf(fs, "%d,", patternLookup[index]);
			}
			fprintf(fs, "\n");
		}

		fprintf(fs, "\nintensity_mask\t");
		for(int y = 0; y < 10; y++)
		{
			fprintf(fs, "\tdb\t");
			for(int x = 0; x < 10; x++)
			{
				int index = y * 10 + x;
				fprintf(fs, "%d,", intensityMask[index]);
			}
			
			if(y == 9)
			{
				fprintf(fs, "%d", intensityMask[100]);
			}
			fprintf(fs, "\n");
		}
	}
	fclose(fs);

	fs = fopen("cgacomp_tables.i", "w");
	if(fs)
	{
		fprintf(fs, "convert_323_palette\t");
		for(int y = 0; y < 16; y++)
		{
			fprintf(fs, "\tdb\t");
			for(int x = 0; x < 16; x++)
			{
				int index = y * 16 + x;
				fprintf(fs, "%d,", compositePatternLookup[index]);
			}
			fprintf(fs, "\n");
		}
		fprintf(fs, "\nintensity_mask\t");
		for(int y = 0; y < 10; y++)
		{
			fprintf(fs, "\tdb\t");
			for(int x = 0; x < 10; x++)
			{
				int index = y * 10 + x;
				fprintf(fs, "%d,", intensityMask[index]);
			}

			if(y == 9)
			{
				fprintf(fs, "%d", intensityMask[100]);
			}
			fprintf(fs, "\n");
		}
	}
	fclose(fs);


	fs = fopen("cgagrey_tables.i", "w");
	if(fs)
	{
		fprintf(fs, "convert_323_palette\t");
		for(int y = 0; y < 16; y++)
		{
			fprintf(fs, "\tdb\t");
			for(int x = 0; x < 16; x++)
			{
				int index = y * 16 + x;
				fprintf(fs, "%d,", greyPatternLookup[index]);
			}
			fprintf(fs, "\n");
		}
		fprintf(fs, "\nintensity_mask\t");
		for(int y = 0; y < 10; y++)
		{
			fprintf(fs, "\tdb\t");
			for(int x = 0; x < 10; x++)
			{
				int index = y * 10 + x;
				fprintf(fs, "%d,", intensityMask[index]);
			}

			if(y == 9)
			{
				fprintf(fs, "%d", intensityMask[100]);
			}
			fprintf(fs, "\n");
		}
	}
	fclose(fs);
}
*/

void GenerateCompositePaletteRGB(uint8_t* palette)
{
	int numCompositePatterns = 0;
	
	for(int x = 0; x < 16; x++)
	{
		for(int y = 0; y < 16; y++)
		{
			int index = y * 16 + x;
			int red = (palette[x * 3] + palette[y * 3]) / 2;
			int green = (palette[x * 3 + 1] + palette[y * 3 + 1]) / 2;
			int blue = (palette[x * 3 + 2] + palette[y * 3 + 2]) / 2;
			compositePatternRGB[index * 3] = (uint8_t) red;
			compositePatternRGB[index * 3 + 1] = (uint8_t) green;
			compositePatternRGB[index * 3 + 2] = (uint8_t) blue;
			compositePatternRGBWeights[index] = (x == y) ? 2 : 3;
			
			int pairDistance = 0;
			for(int n = 0; n < 3; n++)
			{
				int dist = palette[x * 3 + n] - palette[y * 3 + n];
				pairDistance += dist * dist;
			}
			if(pairDistance > MAX_COMPOSITE_DITHER_DISTANCE * MAX_COMPOSITE_DITHER_DISTANCE)
			{
				compositePatternRGBWeights[index] = -1;	
			}
			else
			{
				numCompositePatterns++;
			}
		}
	}
	
	//printf("Num composite patterns: %d\n", numCompositePatterns);
	

	vector<uint8_t> palettePixels;
	for(int n = 0; n < 256; n++)
	{
		palettePixels.push_back(compositePatternRGB[n * 3]);
		palettePixels.push_back(compositePatternRGB[n * 3 + 1]);
		palettePixels.push_back(compositePatternRGB[n * 3 + 2]);
		palettePixels.push_back(255);
	}
	
	if(dumpPalettes)
	{
		lodepng::encode("compopalette.png", palettePixels, 16, 16);
	}
}

void LoadPalette(const char* filename)
{
	FILE* fs = fopen(filename, "rb");
	
	PaletteHeader header;
	ChunkHeader chunkHeader;
	PaletteEntry entries[256];
	
	fread(&header, sizeof(PaletteHeader), 1, fs);
	fread(&chunkHeader, sizeof(ChunkHeader), 1, fs);
	int numEntries = fread(entries, sizeof(PaletteEntry), 256, fs);

	fclose(fs);

	char temp[5];
	temp[4] = 0;
	
	memcpy(temp, header.riff, 4);
	printf("RIFF: %s\n", temp);
	memcpy(temp, header.form, 4);
	printf("Form: %s\n", temp);
	memcpy(temp, chunkHeader.chunk, 4);
	printf("Chunk: %s\n", temp);
	
	printf("Version: %d\nExpected entries: %d\n", chunkHeader.version, chunkHeader.numEntries);
	printf("Num palette entries: %d\n", numEntries);

	GenerateLUT(entries);

	for(int n = 0; n < numEntries; n++)
	{
	//	printf("%d:\tR: %02x G: %02x B: %02x\n", n, entries[n].red, entries[n].green, entries[n].blue);
	}

}

void ProcessExtendedDitherPatterns(uint8_t* palette)
{
	vector<uint8_t> imageOutput;
	vector<uint8_t> ditherRGB;
	int chunkSize = 32;
	int width = NUM_EXTENDED_DITHER_PATTERNS * chunkSize;
	int height = chunkSize * 2;
	

	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			int patternIndex = x / chunkSize;
			
			if(y < chunkSize)
			{
				int i = (x - (patternIndex * chunkSize)) % 4;
				int j = y % 2;
				int paletteIndex = extendedDitherPatterns[patternIndex * EXTENDED_DITHER_PATTERN_SIZE + (j * 4) + i];
				
				uint8_t r = palette[paletteIndex * 3];
				uint8_t g = palette[paletteIndex * 3 + 1];
				uint8_t b = palette[paletteIndex * 3 + 2];
				
				imageOutput.push_back(r);
				imageOutput.push_back(g);
				imageOutput.push_back(b);
				imageOutput.push_back(255);
			}
			else
			{
				imageOutput.push_back(extendedDitherPatternRGB[patternIndex * 3]);
				imageOutput.push_back(extendedDitherPatternRGB[patternIndex * 3 + 1]);
				imageOutput.push_back(extendedDitherPatternRGB[patternIndex * 3 + 2]);
				imageOutput.push_back(255);
			}
		}
	}
	
	lodepng::encode("extendeddither.png", imageOutput, width, height);
	
	imageOutput.clear();
	
	width = chunkSize * 16;
	height = chunkSize * 16;
	
	float error = 0;
	
	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			int index = (y / chunkSize) * 16 + (x / chunkSize);
			uint8_t rgb[3];
			rgb[0] = (uint8_t)((255 * (index >> 5)) / 7);
			rgb[1] = (uint8_t)((255 * ((index >> 3) & 3)) / 3);
			rgb[2] = (uint8_t)((255 * (index & 7)) / 7);
			
			int matchingPattern = FindClosestPaletteEntry(rgb, extendedDitherPatternRGB, NUM_EXTENDED_DITHER_PATTERNS, extendedDitherPatternRGBWeights);
			
			if((y % chunkSize) >= chunkSize / 2)
			{
				int i = x % 4;
				int j = y % 2;
				int paletteIndex = extendedDitherPatterns[matchingPattern * EXTENDED_DITHER_PATTERN_SIZE + (j * 4) + i];

				for(int n = 0; n < 3; n++)
				{
					if((x % chunkSize) == 0 && (y % chunkSize) == chunkSize / 2)
					{
						float diff = (rgb[n] - palette[paletteIndex * 3 + n]) / 255.0f;
						error += (diff * diff);
					}
					
					rgb[n] = palette[paletteIndex * 3 + n];
				}
			}
			
			if((y % chunkSize) == (chunkSize - 1))
			{
				rgb[0] = rgb[1] = rgb[2] = 0;
			}
				
			
			imageOutput.push_back(rgb[0]);
			imageOutput.push_back(rgb[1]);
			imageOutput.push_back(rgb[2]);
			imageOutput.push_back(255);
		}
	}
	
	printf("Total error: %f\n", error);
	lodepng::encode("palette323.png", imageOutput, width, height);
}

int main(int argc, char* argv[])
{
	GenerateExtendedDitherPatternsRGB(palette_brcg);
	ProcessExtendedDitherPatterns(palette_brcg);
	//
	//GenerateExtendedDitherPatternsRGB(palette_bgry);
	//ProcessExtendedDitherPatterns(palette_bgry);

	//GenerateExtendedDitherPatternsRGB(palette_bgrb);
	//ProcessExtendedDitherPatterns(palette_bgrb);
	
	GeneratePatternsRGB(palette_brcg);
	GenerateCompositePaletteRGB(compositePalette);
	GenerateGreyPatternsRGB();
	
	//LoadPalette("sq1pal.pal");
	
	GeneratePaletteLookupTables();
	return 0;
}