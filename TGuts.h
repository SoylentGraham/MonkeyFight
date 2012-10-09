#pragma once
#include "TTypes.h"



#define GD_MAP_WIDTH	64
#define GD_MAP_HEIGHT	64
#define GD_CHAR_WIDTH	8
#define GD_CHAR_HEIGHT	8
#define GD_CHAR_DATA_SIZE	((GD_CHAR_WIDTH*GD_CHAR_HEIGHT)/4)
#define GD_SPRITE_WIDTH		16
#define GD_SPRITE_HEIGHT	16
#define GD_SPRITE_DATA_SIZE	(GD_SPRITE_WIDTH*GD_SPRITE_HEIGHT)
#define GD_PAL256_SIZE		(2*256)
#define GD_SPRITE_OFFSCREEN_Y	400


class TFrameDebug
{
public:
	u16					GetMaxLineCount() const		{	return mStrings.MaxSize();	}
	BufferString<100>&	PushBackString()			{	return mStrings.PushBack();	}

public:
	BufferArray<BufferString<100>,2>	mStrings;
};


template<u16 BufferWidth,u16 BufferHeight>
class TIndexMap
{
public:
	TIndexMap() :
		mMap	( BufferWidth * BufferHeight )
	{
	}

	void		Set(const u8& x,const u8& y,u8 Character)
	{
		int Index = x + (y * BufferWidth );
		mMap[ Index ] = Character;
	}

	void		SetAll(u8 Character)
	{
		mMap.SetAll( Character );
	}

public:
	BufferArray<u8,BufferWidth*BufferWidth>	mMap;
};

class TBackgroundMap : public TIndexMap<GD_MAP_WIDTH,GD_MAP_HEIGHT>
{
public:
};


class TSpriteCharacter : public TIndexMap<GD_SPRITE_WIDTH,GD_SPRITE_HEIGHT>
{
public:
};




class TSpriteInfo
{
public:
	TSpriteInfo() :
		mPosition	( 0, GD_SPRITE_OFFSCREEN_Y ),
		mImage		( 0 ),
		mPalette	( 0 )
	{
	}
	TSpriteInfo(const TPoint& Position,u8 Image,u8 Palette=0) :
		mPosition	( Position ),
		mImage		( Image ),
		mPalette	( Palette )
	{
	}

	u16		GetDepth() const	{	return mPosition.y;	}

public:
	u8		mImage;		//	character index
	u8		mPalette;	//	palette for image
	TPoint	mPosition;
};


//	reference to our internal TSpriteDef
class TSpriteRef
{
public:
	TSpriteRef() :
		mIndex	( -1 )
	{
	}

	bool			IsValid() const		{	return mIndex >= 0;	}
	u8				GetIndex() const	{	return static_cast<u8>( mIndex );	}

	inline bool		operator==(const TSpriteRef& That) const	{	return (this->mIndex == That.mIndex);	}

public:
	s16		mIndex;
};


class TSpriteDepthInfo
{
public:
	TSpriteDepthInfo() :
		mDepth			( 0xffff ),
		mHardwareSprite	( 0xff )
	{
	}

public:
	u16			mDepth;			//	desired depth
	u8			mHardwareSprite;	//	hardware sprite index (real depth order, back to front)
	TSpriteRef	mSpriteRef;		//	which sprite is this
};

class TSpriteDef
{
public:
	TSpriteDef() :
		mDepthIndex	( 0xff )
	{
	}

public:
	TSpriteInfo	mCache;			//	cached info
	u8			mDepthIndex;	//	index to spritedepth array
};

class TSpritePool
{
public:
	TSpritePool(bool DefferedBake);
	TSpriteRef				AllocSprite(const TSpriteInfo& Info);
	void					FreeSprite(const TSpriteRef& Sprite);
	void					MoveSprite(const TSpriteRef& Sprite,const TPoint& Position);
	void					SetSpriteDepth(const TSpriteRef& Sprite,u16 Depth);
	void					BakeHardwareChanges(TFrameDebug& Debug);

private:
//	u8						GetHardwareSpriteIndex(const TSpriteRef& Sprite)	{	return mSprites[Sprite.mIndex].mHardwareIndex;	}
//	int						FindSpriteDef(u8 HardwareIndex)						{	return mSprites.FindIndex( HardwareIndex );	}
//	int						GetSpriteDepthIndex(u8 StartingIndex,u16 Depth);
	void					OnSpriteChanged(const TSpriteRef& Sprite);
	u8						AllocSpriteDepth(u16 Depth,const TSpriteRef& SpriteRef,u8 HardwareSprite);
	void					MoveSpriteDepth(u16 FromIndex,u16 ToIndex,bool ForceHardwareAlignment);
	void					ShiftSpriteDepthsDown(u16 First,u16 Last);
	void					ShiftSpriteDepthsUp(u16 First,u16 Last);
	void					BakeHardwareSprite(const TSpriteRef& Sprite);

	void					Debug_VerifySync(bool CheckHardwareOrder=true,bool CheckDepthOrder=true);				//	verify all arrays are sync'd up correctly

public:
	bool								mDefferedBake;		//	if deffered we update all sprites in one batch
	u16									mDebug_ChangeCount;		//	count how many sprite changes we make
	BufferArray<u8,256>					mFreeSprites;		//	unused hardware sprite indexes
	BufferArray<TSpriteDepthInfo,256>	mDepthInfo;			//	depth info (sorted by depth)
	BufferArray<TSpriteDef,256>			mSprites;			//	allocated sprites
	BufferArray<TSpriteRef,256>			mChangedSprites;	//	sprites that need re-baking
};


namespace TGameDuino
{
	namespace TSpritePal
	{
		enum Type
		{
			Pal256 = 0,
			Pal16,
			Pal4,

			_Max
		};
	};

	void				SetMapPalette(const BufferArray<TColour16,4>& Palette,u8 FirstColour=0);
	void				SetMapScroll(const Type2<u16>& Pos);
	void				SetMap(const TBackgroundMap& Map);
	template<class ARRAY>	//	Array<TCharacter>
	void				SetMapCharacters(const ARRAY& Characters,u8 FirstCharacter=0);
	u16					GetSpritePaletteRamAddr(TSpritePal::Type PalType,u8 PaletteIndex);
	u8					GetSpritePaletteMaxIndex(TSpritePal::Type PalType);
	void				SetSpritePalette(const BufferArray<TColour16,256>& Palette,TSpritePal::Type PalType,u8 PaletteIndex=0);
	void				SetSpriteCharacter(const TSpriteCharacter& Character,u8 Index);
	template<class ARRAY>
	void				SetSpriteCharacters(const ARRAY& Characters,u8 FirstIndex=0);
	void				SetSprite(u8 SpriteIndex,const TSpriteInfo& Sprite);
	void				HideSprite(u8 SpriteIndex);
};



class TCharacter
{
public:
	TCharacter()
	{
		mMap.SetSize( mMap.MaxSize() );
	}

	void		Set(u8 x,u8 y,u8 PaletteIndex)
	{
		//	2 bits per pixel
		u8 Mask = 0x3;
		PaletteIndex &= Mask;		//	2 bits
		int Shift = (3-(x % 4)) * 2;	//	how much do we shift by for pixel x of 4

		int mapx = x / 4;
		int mapy = y * (8/4);
		int Index = mapx + mapy;
		mMap[ Index ] &= ~(Mask << Shift);
		mMap[ Index ] |= PaletteIndex << Shift;
	}

	void		SetAll(u8 PaletteIndex)
	{
		//	pack palette index into a byte (4 pixels)
		PaletteIndex &= 0x3;		//	2 bits
		u8 FourPixels = 0;
		FourPixels |= PaletteIndex << 0;
		FourPixels |= PaletteIndex << 2;
		FourPixels |= PaletteIndex << 4;
		FourPixels |= PaletteIndex << 6;
		mMap.SetAll( FourPixels );
	}

public:
	//	4 palette indexes/pixels per byte
	BufferArray<u8,GD_CHAR_DATA_SIZE>	mMap;
};






template<class ARRAY>	//	Array<TCharacter>
void TGameDuino::SetMapCharacters(const ARRAY& Characters,u8 FirstCharacter)
{
	//	limit here -> count = min( Characters.GetSize()-FirstCharacter, 256 )
	for ( int c=0;	c<Characters.GetSize();	c++ )
	{
		const auto& Char = Characters[c];
		u16 RamAddr = RAM_CHR;
		RamAddr += (FirstCharacter+c) * (GD_CHAR_DATA_SIZE);
		int DataSize = Char.mMap.GetDataSize();
		GD.copy( RamAddr, const_cast<prog_uchar*>( Char.mMap.GetRawData() ), DataSize );
	}
}