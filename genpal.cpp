#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <memory.h>
#include "lodepng.cpp"

#define USE_ALL_DITHERS 1
#define USE_MAGENTA_PALETTE 0
#define USE_BRIGHT_PALETTE 1

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

#define NUM_PATTERNS (sizeof(patterns) / 4)
uint8_t patterns[] =
{
#if USE_ALL_DITHERS
	0, 0, 0, 0,
	0, 1, 0, 0,
	0, 1, 0, 1,
	1, 1, 0, 1,
	1, 1, 1, 1,
	1, 1, 3, 1,
	3, 1, 3, 1,
	3, 1, 3, 3,
	0, 2, 0, 0,
	0, 2, 0, 2,
	2, 2, 0, 2,
	2, 2, 2, 2,
	2, 2, 3, 2,
	3, 2, 3, 2,
	3, 2, 3, 3,
	3, 3, 3, 3,
	0, 0, 0, 0,
	0, 3, 0, 0,
	0, 3, 0, 3,
	3, 3, 0, 3,
	1, 2, 1, 2,
	1, 2, 1, 1,
	2, 2, 1, 2,
#else
	0, 0, 0, 0,
	1, 1, 1, 1,
	2, 2, 2, 2,
	3, 3, 3, 3,
#endif
};

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

uint8_t patternsRGB[NUM_PATTERNS * 3];
uint8_t patternsRGBWeights[NUM_PATTERNS];
uint8_t convertLUT[256];

uint8_t compositePatternRGB[256 * 3];
uint8_t compositePatternRGBWeights[256];

void GeneratePatternsRGB()
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

int FindClosestPaletteEntry(uint8_t* rgb, uint8_t* pal, int palSize, uint8_t* weights = NULL)
{
	int closest = -1;
	int closestDistance = 0;
	
	for(int n = 0; n < palSize; n++)
	{
		int distance = 0;
		
		for(int i = 0; i < 3; i++)
		{
			int diff = (rgb[i] - pal[n * 3 + i]);
			distance += diff * diff;
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

void GeneratePaletteLookupTables()
{
	vector<uint8_t> srcPalOut;
	vector<uint8_t> dstPalOut;
	uint8_t patternLookup[256];
	uint8_t compositePatternLookup[256];
	
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
				
				index++;
			}
		}
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
	}
	fclose(fs);

}

void GenerateCompositePaletteRGB()
{
	int numCompositePatterns = 0;
	
	for(int x = 0; x < 16; x++)
	{
		for(int y = 0; y < 16; y++)
		{
			int index = y * 16 + x;
			int red = (compositePalette[x * 3] + compositePalette[y * 3]) / 2;
			int green = (compositePalette[x * 3 + 1] + compositePalette[y * 3 + 1]) / 2;
			int blue = (compositePalette[x * 3 + 2] + compositePalette[y * 3 + 2]) / 2;
			compositePatternRGB[index * 3] = (uint8_t) red;
			compositePatternRGB[index * 3 + 1] = (uint8_t) green;
			compositePatternRGB[index * 3 + 2] = (uint8_t) blue;
			compositePatternRGBWeights[index] = (x == y) ? 1 : 3;
			
			int pairDistance = 0;
			for(int n = 0; n < 3; n++)
			{
				int dist = compositePalette[x * 3 + n] - compositePalette[y * 3 + n];
				pairDistance += dist * dist;
			}
			if(pairDistance > 240 * 240)
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

int main(int argc, char* argv[])
{
	GeneratePatternsRGB();
	GenerateCompositePaletteRGB();
	
	//LoadPalette("sq1pal.pal");
	
	GeneratePaletteLookupTables();
	return 0;
}