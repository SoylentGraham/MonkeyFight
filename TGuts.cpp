#include "TGuts.h"

#define MAX_DEPTH	0xffff




TSpritePool::TSpritePool(bool DefferedBake) :
	mDefferedBake		( DefferedBake ),
	mDebug_ChangeCount	( 0 )
{
	for ( int i=0;	i<256;	i++ )
		mFreeSprites.PushBack(i);
}

void TSpritePool::BakeHardwareSprite(const TSpriteRef& Sprite)
{
	assert( Sprite.IsValid(), "Sprite invalid" );
	auto& SpriteDef = mSprites[Sprite.GetIndex()];
	auto& SpriteDepth = mDepthInfo[SpriteDef.mDepthIndex];
	TGameDuino::SetSprite( SpriteDepth.mHardwareSprite, SpriteDef.mCache );
}

void TSpritePool::BakeHardwareChanges(TFrameDebug& Debug)
{
	if ( mDefferedBake )
		mDebug_ChangeCount = mChangedSprites.GetSize();

	for ( int i=0;	i<mChangedSprites.GetSize();	i++ )
		BakeHardwareSprite( mChangedSprites[i] );
	
	mChangedSprites.Clear();

	auto& DebugString = Debug.PushBackString();
	DebugString << "Sprite Changes: " << mDebug_ChangeCount << "       ";

	mDebug_ChangeCount = 0;
}

void TSpritePool::OnSpriteChanged(const TSpriteRef& Sprite)
{
	if ( mDefferedBake )
	{
		mChangedSprites.PushBackUnique( Sprite );
	}
	else
	{
		BakeHardwareSprite( Sprite );
		mDebug_ChangeCount++;
	}
}

void TSpritePool::MoveSpriteDepth(u16 FromIndex,u16 ToIndex,bool ForceHardwareAlignment)
{
	//	no change to depth data
	if ( FromIndex != ToIndex )
	{
		//	save a copy of the depth info (will get overridden when shifted)
		TSpriteDepthInfo DepthInfo = mDepthInfo[FromIndex];

		//	shift depth array
		if ( FromIndex > 0 && ToIndex < FromIndex )
		{
			ShiftSpriteDepthsDown( ToIndex, FromIndex-1 );
		}
		else if ( ToIndex > FromIndex )
		{
			ShiftSpriteDepthsUp( FromIndex+1, ToIndex );
		}

		//	place in moving depth info
		auto& NewDepthInfo = mDepthInfo[ToIndex];
		NewDepthInfo = DepthInfo;
		mSprites[DepthInfo.mSpriteRef.GetIndex()].mDepthIndex = ToIndex;
		OnSpriteChanged( DepthInfo.mSpriteRef );

		Debug_VerifySync( false, true );
	}

	//	re-align hardware sprites (sometimes forced when a new sprite is introduced
	if ( ForceHardwareAlignment || FromIndex != ToIndex )
	{

		//	re-order hardware sprites
		//	bubble sort our new hardware sprite
		int NewHardwareSpriteIndex = ToIndex;
		for ( ; NewHardwareSpriteIndex>0;	NewHardwareSpriteIndex-- )
		{
			auto& NewDepth = mDepthInfo[NewHardwareSpriteIndex];
			auto& PrevDepth = mDepthInfo[NewHardwareSpriteIndex-1];
			if ( NewDepth.mHardwareSprite > PrevDepth.mHardwareSprite )
				break;

			//	bubble-swap
			u8 Temp = PrevDepth.mHardwareSprite;
			PrevDepth.mHardwareSprite = NewDepth.mHardwareSprite;
			NewDepth.mHardwareSprite = Temp;
			OnSpriteChanged( PrevDepth.mSpriteRef );
			OnSpriteChanged( NewDepth.mSpriteRef );
		}
		for ( ; NewHardwareSpriteIndex<mDepthInfo.GetTailIndex();	NewHardwareSpriteIndex++ )
		{
			auto& NewDepth = mDepthInfo[NewHardwareSpriteIndex];
			auto& NextDepth = mDepthInfo[NewHardwareSpriteIndex+1];
			if ( NewDepth.mHardwareSprite < NextDepth.mHardwareSprite )
				break;

			//	bubble-swap
			u8 Temp = NextDepth.mHardwareSprite;
			NextDepth.mHardwareSprite = NewDepth.mHardwareSprite;
			NewDepth.mHardwareSprite = Temp;
			OnSpriteChanged( NextDepth.mSpriteRef );
			OnSpriteChanged( NewDepth.mSpriteRef );
		}
	}

	Debug_VerifySync();
}

void TSpritePool::ShiftSpriteDepthsDown(u16 First,u16 Last)
{
	assert( First <= Last, "Shifting in wrong direction.");

	//	move all down 
	for ( int i=Last;	i>=First;	i-- )
	{
		u16 FromIndex = i;
		u16 ToIndex = i+1;
		auto& DepthFrom = mDepthInfo[FromIndex];
		auto& DepthTo = mDepthInfo[ToIndex];

		//	move
		DepthTo = DepthFrom;

		//	update spritedef
		auto& SpriteDef = mSprites[DepthTo.mSpriteRef.GetIndex()];

		//	from old index
		assert( SpriteDef.mDepthIndex == FromIndex, "Depth and Def's not sync'd" );
		//	to new index
		SpriteDef.mDepthIndex = ToIndex;

		//	notify change (hardware sprite has changed)
		OnSpriteChanged( DepthTo.mSpriteRef );
	}
}

void TSpritePool::ShiftSpriteDepthsUp(u16 First,u16 Last)
{
	assert( First <= Last, "Shifting in wrong direction.");

	//	move all up 
	for ( int i=First;	i<=Last;	i++ )
	{
		u16 FromIndex = i;
		u16 ToIndex = i-1;
		auto& DepthFrom = mDepthInfo[FromIndex];
		auto& DepthTo = mDepthInfo[ToIndex];

		//	move
		DepthTo = DepthFrom;

		//	update spritedef
		auto& SpriteDef = mSprites[DepthTo.mSpriteRef.GetIndex()];

		//	from old index
		assert( SpriteDef.mDepthIndex == FromIndex, "Depth and Def's not sync'd" );
		//	to new index
		SpriteDef.mDepthIndex = ToIndex;

		//	notify change (hardware sprite has changed)
		OnSpriteChanged( DepthTo.mSpriteRef );
	}
}

u8 TSpritePool::AllocSpriteDepth(u16 Depth,const TSpriteRef& SpriteRef,u8 HardwareSprite)
{
	//	find where to insert new depth with binary chop
	//	todo: binary chop
	int Index;
	for ( Index=0;	Index<mDepthInfo.GetSize();	Index++ )
	{
		auto& DepthInfo = mDepthInfo[Index];
		if ( Depth < DepthInfo.mDepth )
			break;
	}
	
	assert( Index <= mDepthInfo.GetSize(), "shouldn't be greater" );

	//	add a tail
	auto& NewDepthInfo = mDepthInfo.PushBack();
	NewDepthInfo.mDepth = Depth;
	NewDepthInfo.mHardwareSprite = HardwareSprite;
	NewDepthInfo.mSpriteRef = SpriteRef;
	mSprites[SpriteRef.GetIndex()].mDepthIndex = mDepthInfo.GetTailIndex();

	//	move into place
	MoveSpriteDepth( mDepthInfo.GetTailIndex(), Index, true );

	assert( Index >= 0 && Index < 256, "Out of bounds" );
	return static_cast<u8>( Index );
}

TSpriteRef TSpritePool::AllocSprite(const TSpriteInfo& Info)
{
	//	get a free hardware index...
	if ( mFreeSprites.IsEmpty() )
		return TSpriteRef();

	//	grab a free sprite index
	u8 HardwareSprite;
	mFreeSprites.PopBack( HardwareSprite );

	//	alloc a sprite def
	TSpriteDef& SpriteDef = mSprites.PushBack();
	SpriteDef.mCache = Info;
	TSpriteRef SpriteRef;
	SpriteRef.mIndex = static_cast<u8>( mSprites.GetTailIndex() );

	//	alloc & init a sprite depth
	u8 SpriteDepthIndex = AllocSpriteDepth( SpriteDef.mCache.GetDepth(), SpriteRef, HardwareSprite );

	//	sync depth & def
	SpriteDef.mDepthIndex = SpriteDepthIndex;

	OnSpriteChanged( SpriteRef );
	Debug_VerifySync();

	return SpriteRef;
}

void TSpritePool::SetSpriteDepth(const TSpriteRef& Sprite,u16 NewDepth)
{
	assert( Sprite.IsValid(), "Invalid sprite" );
	
	//	bubble-find where we want to be placed to cause minimum disruption
	int CurrentDepthIndex = mSprites[Sprite.GetIndex()].mDepthIndex;
	int NewDepthIndex = CurrentDepthIndex;

	for ( ; NewDepthIndex>0;	NewDepthIndex-- )
	{
		auto& PrevDepth = mDepthInfo[NewDepthIndex-1].mDepth;
		if ( NewDepth >= PrevDepth )
			break;
	}
	for ( ; NewDepthIndex<mDepthInfo.GetTailIndex();	NewDepthIndex++ )
	{
		auto& NextDepth = mDepthInfo[NewDepthIndex+1].mDepth;
		if ( NewDepth <= NextDepth )
			break;
	}

	mDepthInfo[CurrentDepthIndex].mDepth = NewDepth;
	MoveSpriteDepth( CurrentDepthIndex, NewDepthIndex, false );
	
}


void TSpritePool::FreeSprite(const TSpriteRef& Sprite)
{
	assert(false,"todo");
}

void TSpritePool::MoveSprite(const TSpriteRef& Sprite,const TPoint& Position)
{
	assert( Sprite.IsValid(), "Invalid sprite" );

	auto& SpriteDef = mSprites[Sprite.GetIndex()];
	auto& SpriteDepth = mDepthInfo[SpriteDef.mDepthIndex];
	
	//	no change
	if ( SpriteDef.mCache.mPosition == Position )
		return;

	//	change sprite info
	u16 OldDepth = SpriteDepth.mDepth;
	SpriteDef.mCache.mPosition = Position;
	u16 NewDepth = SpriteDef.mCache.GetDepth();
	if ( OldDepth != NewDepth )
	{
		SetSpriteDepth( Sprite, NewDepth );
	}

	OnSpriteChanged( Sprite );
}


//	verify all arrays are sync'd up correctly
void TSpritePool::Debug_VerifySync(bool CheckHardwareOrder,bool CheckDepthOrder)
{
	assert( mSprites.GetSize() == mDepthInfo.GetSize(), "Sprite arrays are different size" );

	for ( int s=0;	s<mSprites.GetSize();	s++ )
	{
		auto& Sprite = mSprites[s];
		assert( Sprite.mDepthIndex < mDepthInfo.GetSize(), "Sprite's depth index out of bounds" );
		auto& SpriteDepth = mDepthInfo[Sprite.mDepthIndex];
		assert( SpriteDepth.mSpriteRef.IsValid(), "Sprite's depth info has invalid sprite ref" );
		assert( SpriteDepth.mSpriteRef.GetIndex() == s, "Sprite's depth info points at different sprite" );
	}

	for ( int d=0;	d<mDepthInfo.GetSize();	d++ )
	{
		auto& SpriteDepth = mDepthInfo[d];

		assert( SpriteDepth.mSpriteRef.IsValid(), "Sprite depth info has invalid sprite ref" );
		auto& Sprite = mSprites[SpriteDepth.mSpriteRef.GetIndex()];
		assert( Sprite.mDepthIndex == d, "Depth info's sprite points at different depth info" );

		//	check no duplicates
		for ( int e=d+1;	CheckHardwareOrder && e<mDepthInfo.GetSize();	e++ )
		{
			auto& NextSpriteDepth = mDepthInfo[e];
			assert( NextSpriteDepth.mHardwareSprite != SpriteDepth.mHardwareSprite, "Duplicated hardware sprite" );
		}

		//	check order
		if ( CheckDepthOrder && d < mDepthInfo.GetTailIndex() )
		{
			auto& NextSpriteDepth = mDepthInfo[d+1];
			assert( NextSpriteDepth.mDepth >= SpriteDepth.mDepth, "Depth info out of order (depth)" );

			if ( CheckHardwareOrder )
				assert( NextSpriteDepth.mHardwareSprite > SpriteDepth.mHardwareSprite, "Depth info out of order (hardware sprite)" );
		}
	}
}

void TGameDuino::SetMapPalette(const BufferArray<TColour16,4>& Palette,u8 FirstColour)
{
	/*
	//	count = min( Palette.Size - FirstColour, 4 )
	u16 Ram = RAM_PAL;
	Ram += FirstColour * sizeof(TColour16);
	GD.copy( Ram, const_cast<prog_uchar*>( Palette.GetRawData() ), Palette.GetDataSize() );
	*/
	for ( int i=0;	i<Palette.GetSize();	i++ )
		GD.setpal( FirstColour+i, Palette[i].mRgba );
}

void TGameDuino::SetMapScroll(const Type2<u16>& Pos)
{
	GD.wr16( SCROLL_X, Pos.x );
	GD.wr16( SCROLL_Y, Pos.y );
}

void TGameDuino::SetMap(const TBackgroundMap& Map)
{
	u8 MapX = 0;
	u8 MapY = 0;
	u16 Ram = RAM_PIC;
	Ram += MapX + (MapY * GD_MAP_WIDTH);
	GD.copy( Ram, const_cast<prog_uchar*>( Map.mMap.GetRawData() ), Map.mMap.GetDataSize() );
}


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

u16 TGameDuino::GetSpritePaletteRamAddr(TSpritePal::Type PalType,u8 PaletteIndex)
{
	u16 Addr_256[4] = 
	{ 
		RAM_SPRPAL+(GD_PAL256_SIZE*0), 
		RAM_SPRPAL+(GD_PAL256_SIZE*1), 
		RAM_SPRPAL+(GD_PAL256_SIZE*2), 
		RAM_SPRPAL+(GD_PAL256_SIZE*3) 
	};
	u16 Addr_16[2] = { PALETTE16A, PALETTE16B };
	u16 Addr_4[2] = { PALETTE4A, PALETTE4B };
	u16* AddrPal[ TSpritePal::_Max ] = { Addr_256, Addr_16, Addr_4 };
	u16* Addr = AddrPal[ PalType ];
	return Addr[ PaletteIndex ];
}
	
u8 TGameDuino::GetSpritePaletteMaxIndex(TSpritePal::Type PalType)
{
	u8 PalMax[ TSpritePal::_Max ] = { 255, 15, 3 };
	return PalMax[ PalType ];
}
	
void TGameDuino::SetSpritePalette(const BufferArray<TColour16,256>& Palette,TSpritePal::Type PalType,u8 PaletteIndex)
{
	u16 RamAddr = GetSpritePaletteRamAddr( PalType, PaletteIndex );
	//u8 Count = min( GetSpritePaletteMaxCount(PalType)-PaletteIndex, Palette.GetSize() );
	GD.copy( RamAddr + (PaletteIndex*sizeof(TColour16)), const_cast<prog_uchar*>( Palette.GetRawData() ), Palette.GetDataSize() );
}


void TGameDuino::SetSpriteCharacter(const TSpriteCharacter& Character,u8 Index)
{
	//	limit
	u16 RamAddr = RAM_SPRIMG;
	RamAddr += Index * (GD_SPRITE_DATA_SIZE);
	//RamAddr -= Index;
	GD.copy( RamAddr, const_cast<prog_uchar*>( Character.mMap.GetData() ), Character.mMap.GetDataSize() );
}

template<class ARRAY>
void TGameDuino::SetSpriteCharacters(const ARRAY& Characters,u8 FirstIndex)
{
	for ( u16 i=0;	(i+FirstIndex<256) && (i<Characters.GetSize());	i++ )
	{
		SetSpriteCharacter( Characters[i], FirstIndex+i );	
	}
}

void TGameDuino::SetSprite(u8 SpriteIndex,const TSpriteInfo& Sprite)
{
	GD.sprite( SpriteIndex, Sprite.mPosition.x, Sprite.mPosition.y, Sprite.mImage, Sprite.mPalette );
}


void TGameDuino::HideSprite(u8 SpriteIndex)
{
	//	gr: don't care about the rest, just Y. could make this a GD.wr16()...
	GD.sprite( SpriteIndex, 0, GD_SPRITE_OFFSCREEN_Y, 0, 0 );
}

u16 TGuts::GetStringLength(const char* String)
{
	u16 Length = 0;
	while ( *String++ )
	{
		Length++;
	}
	return Length;
}

void TGuts::Assert(bool Condition,const char* Error,const char* Function)
{
	//	no error!
	if ( Condition )
		return;

	GD.begin();
	GD.ascii();
	
	GD.putstr( 0, 1, "Assert!" );
	GD.putstr( 2, 3, Error );

	GD.putstr( 0, 6, "in function: " );
	
	int MaxStringWidth = 49 - 2;
	int Line = 8;
	for ( int StringOffset=0;	StringOffset<GetStringLength(Function);	StringOffset+=MaxStringWidth,Line++ )
	{
		GD.putstr( 2, Line, &Function[StringOffset] );
	}
	

	while( true )
	{
		delay(1000);
	}
}
