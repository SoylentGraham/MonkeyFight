#include "Game.h"


TSpritePool gSpritePool( true );

namespace TButton
{
	enum Type
	{
		Up = 0,
		Down,
		Left,
		Right,
		One,
		Two,
		Three,
		Four
	};

	u8		GetBit(TButton::Type Button)	{	return 1<<static_cast<u8>(Button);	}
};

class TCollisionShape
{
public:
	TCollisionShape() :
		mRadius		( -1.f ),
		mStatic		( false )
	{
	}
	TCollisionShape(const TPointf& Position,float Radius,bool Static) :
		mPosition	( Position ),
		mRadius		( Radius ),
		mStatic		( Static )
	{
	}

	bool			IsValid() const		{	return mRadius > 0.f;	}

public:
	TPointf		mPosition;	//	offset from parent's pos
	float		mRadius;
	bool		mStatic;	//	if static, object will not move
};

class TInputSource
{
public:
	virtual u8		GetButtonDownBits()=0;
};

class TInputSource_GDEmu : public TInputSource
{
public:
	#define GDEMU_DIGITAL_LEFT  6
	#define GDEMU_DIGITAL_RIGHT 3
	#define GDEMU_DIGITAL_UP    4
	#define GDEMU_DIGITAL_DOWN  5
	#define GDEMU_DIGITAL_SHOOT 2
	#define GDEMU_ANALOG_X      0
	#define GDEMU_ANALOG_Y      1

	virtual u8		GetButtonDownBits()
	{
		u8 Buttons = 0x0;

		if ( !digitalRead(GDEMU_DIGITAL_DOWN) )
			Buttons |= TButton::GetBit( TButton::Down );
		if ( !digitalRead(GDEMU_DIGITAL_UP) )
			Buttons |= TButton::GetBit( TButton::Up );
		if ( !digitalRead(GDEMU_DIGITAL_LEFT) )
			Buttons |= TButton::GetBit( TButton::Left );
		if ( !digitalRead(GDEMU_DIGITAL_RIGHT) )
			Buttons |= TButton::GetBit( TButton::Right );
		if ( !digitalRead(GDEMU_DIGITAL_SHOOT) )
			Buttons |= TButton::GetBit( TButton::One );
		
		return Buttons;
	}
};

class TInput
{
public:
	TInput() :
		mButtonDown		( 0x0 ),
		mButtonPressed	( 0x0 ),
		mButtonReleased	( 0x0 ),
		mInputSource	( NULL )
	{
	}
	~TInput()
	{
		SetInputSource(NULL);
	}


	void	Update()
	{
		u8 NewButtonDownBits = mInputSource ? mInputSource->GetButtonDownBits() : 0x0;
		OnNewButtonBits( NewButtonDownBits );
	}

	void	SetInputSource(TInputSource* pInputSource)
	{
		if ( mInputSource )
		{
			delete mInputSource;
			mInputSource = NULL;
		}
		mInputSource = pInputSource;
	}

	bool	IsDown(TButton::Type Button) const			{	return (mButtonDown & TButton::GetBit(Button))!=0;	}
	bool	IsUp(TButton::Type Button) const			{	return !IsDown( Button );	}
	bool	IsPressed(TButton::Type Button) const		{	return (mButtonPressed & TButton::GetBit(Button))!=0;	}
	bool	IsReleased(TButton::Type Button) const		{	return (mButtonReleased & TButton::GetBit(Button))!=0;	}

	TPointf	GetDirectionVector() const
	{
		//T s = static_cast<T>(sqrt(v.x*v.x + v.y*v.y));
		float Hyp_1x1 = 1.4142135623730950488016887242097f;
		float OpAd_Hyp_1x1 = 1.f / Hyp_1x1;

		TPointf Dir(0,0);
		Dir.y += IsDown( TButton::Down ) ? 1.f : 0.f;
		Dir.y -= IsDown( TButton::Up ) ? 1.f : 0.f;
		Dir.x += IsDown( TButton::Right ) ? 1.f : 0.f;
		Dir.x -= IsDown( TButton::Left ) ? 1.f : 0.f;
		
		//	normalise so up-left doesn't move 2 spaces
		if ( Dir.x != 0.f && Dir.y != 0.f )
			Dir *= OpAd_Hyp_1x1;
		
		return Dir;
	}

	void	OnNewButtonBits(u8 NewButtonDown)
	{
		mButtonPressed = (NewButtonDown ^ mButtonDown) & NewButtonDown;
		mButtonReleased = (NewButtonDown ^ mButtonDown) & mButtonDown;
		mButtonDown = NewButtonDown;
	}

private:
	TInputSource*	mInputSource;
	u8				mButtonDown;
	u8				mButtonPressed;
	u8				mButtonReleased;
};

class TPhysicsObject
{
public:
	TCollisionShape	GetWorldCollisionShape() const			
	{	
		return TCollisionShape( mCollision.mPosition + mVelocity + mForce, mCollision.mRadius, mCollision.mStatic );	
	}

	void	PostUpdate(float Friction)
	{
		//	move with velocity
		mCollision.mPosition += mVelocity;
		
		//	dampen velocity
		mVelocity *= 1.f - Friction;
	}

public:
	TCollisionShape	mCollision;
	TPointf			mForce;
	TPointf			mVelocity;
};

class TPlayer
{
public:
	TPlayer()
	{
	}
	TPlayer(const TSpriteInfo& Sprite,const TCollisionShape& Collision) :
		mPlayerSpriteInfo		( Sprite ),
		mGloveAngleDeg			( 0.f ),
		mGloveDistance			( 20.f )
	{
		mPlayerPhysics.mCollision.mPosition.x = Sprite.mPosition.x;
		mPlayerPhysics.mCollision.mPosition.y = Sprite.mPosition.y;
		mPlayerPhysics.mCollision.mRadius = Collision.mRadius;
		mPlayerSpriteOffset.x = -Collision.mPosition.x;
		mPlayerSpriteOffset.y = -Collision.mPosition.y;
	}

	void			SetInputSource(TInputSource* pInputSource)	{	mInput.SetInputSource( pInputSource );	}
	TCollisionShape	GetPlayerWorldCollisionShape() const		{	return mPlayerPhysics.GetWorldCollisionShape();	}
	TCollisionShape	GetGloveWorldCollisionShape() const			{	return mGlovePhysics.GetWorldCollisionShape();	}
	const TPointf&	GetPlayerPosition() const					{	return mPlayerPhysics.mCollision.mPosition;	}
	const TPointf&	GetGlovePosition() const					{	return mGlovePhysics.mCollision.mPosition;	}


public:
	TInput			mInput;
	
	TPhysicsObject	mPlayerPhysics;
	TPhysicsObject	mGlovePhysics;

	TPointf			mPlayerSpriteOffset;	//	main sprite offset from collision shape
	TSpriteInfo		mPlayerSpriteInfo;
	TSpriteRef		mPlayerSpriteRef;
	
	TSpriteInfo		mGloveSpriteInfo;
	TSpriteRef		mGloveSpriteRef;
	float			mGloveAngleDeg;			//	current direction of glove
	float			mGloveDistance;			//	current distance of glove
};

BufferArray<TPlayer,256> gPlayers;


u8 Lerp(const u8& From,const u8& To,float Time)
{
	float Fromf = static_cast<float>( From );
	float Tof = static_cast<float>( To );
	float Valuef = ((Tof - Fromf) * Time) + Fromf;
	return static_cast<u8>( Valuef );
}


void TGame::Init()
{
	float CharacterRadius = 8.f;


	GD.ascii();
	GD.putstr(0, 0, "Hi");
	
	//	generate palette
	BufferArray<TColour16,4> Palette;
	Palette.PushBack( TColour16( 22, 0, 0 ) );
	Palette.PushBack( TColour16( 0,	22, 0,	1 ) );
	Palette.PushBack( TColour16( 0,	0, 22, 1 ) );
	Palette.PushBack( TColour16( 22, 22, 0, 1 ) );
	TGameDuino::SetMapPalette( Palette );

	//	generate characters
	BufferArray<TCharacter,1> Chars;
	for ( int i=0;	i<Chars.MaxSize();	i++ )
	{
		auto& Char = Chars.PushBack();
		int Pal = (i) % Palette.GetSize();
		int Pal2 = (Pal+1) % Palette.GetSize();
		int Pal3 = (Pal+2) % Palette.GetSize();
		int Pal4 = (Pal+3) % Palette.GetSize();
		//Char.SetAll( Pal );
		
		for ( int x=0;	x<4;	x++ )
		for ( int y=0;	y<4;	y++ )
			Char.Set( x,y,Pal );

		for ( int x=4;	x<8;	x++ )
		for ( int y=0;	y<4;	y++ )
			Char.Set( x,y,Pal2 );

		for ( int x=0;	x<4;	x++ )
		for ( int y=4;	y<8;	y++ )
			Char.Set( x,y,Pal3 );

		for ( int x=4;	x<8;	x++ )
		for ( int y=4;	y<8;	y++ )
			Char.Set( x,y,Pal4 );

	}
	
	TGameDuino::SetMapCharacters( Chars );
	
	
	//	generate background character map
	//	only 400x300 pixels are visible or 50x37
	TBackgroundMap Map;
	int i=0;
	for ( int y=0;	y<37;	y++ )
	{
		for ( int x=0;	x<50;	x++ )
		{
			u8 Char = i % Chars.GetSize();			
			i++;
			Map.Set( x, y, Char );
		}
	}
	TGameDuino::SetMapScroll( Type2<u16>(0,0) );
	TGameDuino::SetMap( Map );
	//GD.ascii();
	GD.putstr(0, 0, "Hi");
	

	//	setup rainbow sprite palette
	BufferArray<TColour16,256> RedPal;
	BufferArray<TColour16,256> GreenPal;
	BufferArray<TColour16,256> BluePal;
	BufferArray<TColour16,256> GreyPal;
	for ( int i=0;	i<256;	i++ )
	{
		bool Solid = i>0;
		u8 Component = 255-i;
		RedPal.PushBack( TColour16( Component, 0, 0, Solid ) );
		GreenPal.PushBack( TColour16( 0, Component, 0, Solid ) );
		BluePal.PushBack( TColour16( 0, 0, Component, Solid ) );
		GreyPal.PushBack( TColour16( Component, Component, Component, Solid ) );
	}
	
	TGameDuino::SetSpritePalette( RedPal, TGameDuino::TSpritePal::Pal256, 0 );
	TGameDuino::SetSpritePalette( GreenPal, TGameDuino::TSpritePal::Pal256, 1 );
	TGameDuino::SetSpritePalette( BluePal, TGameDuino::TSpritePal::Pal256, 2 );
	TGameDuino::SetSpritePalette( GreyPal, TGameDuino::TSpritePal::Pal256, 3 );

	//	make up a sprite graphic per palette (these are exactly the same)
	TSpriteCharacter SphereCharactera;
	TSpriteCharacter SphereCharacterb;
	TSpriteCharacter SphereCharacterc;
	TSpriteCharacter SphereCharacterd;
	for ( int x=0;	x<GD_SPRITE_WIDTH;	x++ )
	{
		for ( int y=0;	y<GD_SPRITE_HEIGHT;	y++ )
		{
			bool Transparent = false;
			u8 FullColour = x+y*GD_SPRITE_WIDTH;
			TPointf DistToCenter( x-8, y-8 );
			Transparent = ( DistToCenter.GetLengthSq() >= CharacterRadius*CharacterRadius );
			
			u8 Coloura = Lerp(0,64, static_cast<float>(x+y*GD_SPRITE_WIDTH)/256.f );
			u8 Colourb = Lerp(64,128, static_cast<float>(x+y*GD_SPRITE_WIDTH)/256.f );
			u8 Colourc = Lerp(128,192, static_cast<float>(x+y*GD_SPRITE_WIDTH)/256.f );
			u8 Colourd = Lerp(192,256, static_cast<float>(x+y*GD_SPRITE_WIDTH)/256.f );

			SphereCharactera.Set( x, y, Transparent ? 0 : Coloura );
			SphereCharacterb.Set( x, y, Transparent ? 0 : Colourb );
			SphereCharacterc.Set( x, y, Transparent ? 0 : Colourc );
			SphereCharacterd.Set( x, y, Transparent ? 0 : Colourd );
		}
	}
	TGameDuino::SetSpriteCharacter( SphereCharactera, 0 );
	TGameDuino::SetSpriteCharacter( SphereCharacterb, 1 );
	TGameDuino::SetSpriteCharacter( SphereCharacterc, 2 );
	TGameDuino::SetSpriteCharacter( SphereCharacterd, 3 );


	//	sphere, pal 123
	BufferArray<Type2<u8>,20> PlayerSpriteCharPal;
	for ( int pal=1;	pal<=3;	pal++ )
	{
		PlayerSpriteCharPal.PushBack( Type2<u8>( 0, pal ) );
		PlayerSpriteCharPal.PushBack( Type2<u8>( 1, pal ) );
		PlayerSpriteCharPal.PushBack( Type2<u8>( 2, pal ) );
		PlayerSpriteCharPal.PushBack( Type2<u8>( 3, pal ) );
	}

	//	0, red pal
	Type2<u8> GloveSpriteCharPal;
	GloveSpriteCharPal = Type2<u8>( 0, 0 );

	//	make up players
	int PlayerCount = 10;
	BufferArray<float,4> Speeds;
	TCollisionShape PlayerCollision( TPointf(8,8), CharacterRadius, false );
	Speeds.PushBack( 0.1f );
	Speeds.PushBack( 0.2f );
	Speeds.PushBack( 0.5f );
	Speeds.PushBack( 1.0f );
	for ( int p=0;	p<PlayerCount;	p++)
	{
		auto& CharPal = PlayerSpriteCharPal[ p%PlayerSpriteCharPal.GetSize() ];
		u8 Character = CharPal.x;
		u8 Palette = CharPal.y;
		TPoint Pos( 100 + 10*p, 60 + 10*p );
		TPlayer& Player = gPlayers.PushBack( TPlayer( TSpriteInfo( Pos, Character, Palette ), PlayerCollision ) );

		if ( p==0 || p==3 || p==8 )
			Player.mPlayerPhysics.mCollision.mStatic = true;

		Player.mPlayerSpriteRef = gSpritePool.AllocSprite( Player.mPlayerSpriteInfo );
	}
	
	//	force an out of order one
	{
		auto& CharPal = PlayerSpriteCharPal[0];
		u8 Character = CharPal.x;
		u8 Palette = CharPal.y;
		TPlayer& Player = gPlayers.PushBack( TPlayer( TSpriteInfo( TPoint(200,200), Character, Palette ), PlayerCollision ) );
		Player.mPlayerSpriteRef = gSpritePool.AllocSprite( Player.mPlayerSpriteInfo );
		Player.SetInputSource( new TInputSource_GDEmu );
	}
	
}


struct TIntersection
{
	TPointf		mMidPoint;
	float		mDistance;
	TPointf		mIntersection;
	TPointf		mOtherIntersection;
	float		mForceWeight;		//	intersection weight 0..1 0.5/0.5	if both objects have the same force impant
};

namespace TLMaths
{
	const float	g_NearZero = 0.0001f;
};

bool GetIntersection(TCollisionShape& a,TCollisionShape& b,TIntersection& NodeAIntersection,TIntersection& NodeBIntersection,TPhysicsObject& ObjectA,TPhysicsObject& ObjectB)
{
	//	get the vector between the spheres
	TPointf Diff( b.mPosition - a.mPosition );
	//float2 Diff = NodeAIntersection.mDistance;

	//	too embedded to do anything with it 
	float DiffLengthSq = Diff.GetLengthSq();
	if ( DiffLengthSq < TLMaths::g_NearZero )     
		return false;   

	float TotalRadSq = a.mRadius + b.mRadius;
	TotalRadSq *= TotalRadSq;

	//	too far away to intersect
	if ( DiffLengthSq > TotalRadSq )
		return false;

	//	save distance
	//	gr: should this be a vector?
	NodeAIntersection.mDistance = sqrtf(DiffLengthSq);

	//	calc impact weighting
	if ( a.mStatic != b.mStatic )
	{
		NodeAIntersection.mForceWeight = a.mStatic ? 1.f : 0.f;
	}
	else
	{
		//	work out relative weights of object
		float ObjectAWeight = 0.5f;

		//	get force relativity (both weighting and then dotproduct relevance)
		TPointf ObjectAForce = ObjectA.mForce + ObjectA.mVelocity;
		TPointf ObjectBForce = ObjectB.mForce + ObjectB.mVelocity;
		
		//	force weight
		NodeAIntersection.mForceWeight = ObjectAWeight;
	}


	//	intersected, work out the intersection points
	NodeAIntersection.mIntersection = Diff;
	NodeAIntersection.mIntersection.Normalise( a.mRadius );
	NodeAIntersection.mIntersection += a.mPosition;
	
	NodeAIntersection.mOtherIntersection = Diff;
	NodeAIntersection.mOtherIntersection.Normalise( b.mRadius );
	NodeAIntersection.mOtherIntersection += b.mPosition;	
	
	NodeAIntersection.mMidPoint = Diff;
	NodeAIntersection.mMidPoint *= 0.5f;
	NodeAIntersection.mMidPoint += a.mPosition;

	//	copy to intersection B
	NodeBIntersection.mDistance = -NodeAIntersection.mDistance;
	NodeBIntersection.mMidPoint = NodeAIntersection.mMidPoint;
	NodeBIntersection.mIntersection = NodeAIntersection.mOtherIntersection;
	NodeBIntersection.mOtherIntersection = NodeAIntersection.mIntersection;
	NodeBIntersection.mForceWeight = 1.f - NodeAIntersection.mForceWeight;

	return true;
}

class TCollisionTest
{
public:
	TCollisionTest()
	{
	}
	TCollisionTest(TPhysicsObject& ObjectA,TPhysicsObject& ObjectB) :
		mObjectA	( &ObjectA ),
		mObjectB	( &ObjectB ),
		mHit		( false )
	{
	}

	bool		IsValid() const		{	return mObjectA && mObjectB;	}

public:
	TPhysicsObject*	mObjectA;
	TPhysicsObject*	mObjectB;

	bool		mHit;				//	was hit?
	
	//TPointf		mHitPosition;		//	world space
	//TPointf		mResponseForceA;	//	force to apply to object A
	//TPointf		mResponseForceB;	//	force to apply to object A
	TIntersection	mIntersectionA;
	TIntersection	mIntersectionB;
};


void OnCollision(TPhysicsObject& Player,const TIntersection& Intersection)
{
	//	we want to move our intersection point up to the edge of where the other object intersected
	TPointf Delta = Intersection.mOtherIntersection - Intersection.mIntersection;

	//	if the force weight is 1 then we don't move as all the power is on OUR side.
	//	if it's zero, we take ALL the impact
	Delta *= 1.f - Intersection.mForceWeight;

	//	gr: should not be neccessary...
	float Bounce = 10.f;
	Delta*=1.f/Bounce;//0.5f;

	//	change the movement delta if we're against a static object (so definately is moved)
	//	if it's a soft object (not static) then change the velocity
	//	gr: had to NEGATE this from tootle code... WHY???
	Player.mForce -= Delta;
}


void DoCollision(TCollisionTest& CollisionTest)
{
	CollisionTest.mHit = false;
	if ( !CollisionTest.IsValid() )
		return;
	auto& ObjA = *CollisionTest.mObjectA;
	auto& ObjB = *CollisionTest.mObjectB;

	TCollisionShape ColShapeA = ObjA.GetWorldCollisionShape();
	TCollisionShape ColShapeB = ObjB.GetWorldCollisionShape();
	if ( !ColShapeA.IsValid() || !ColShapeB.IsValid() )
		return;

	if ( !GetIntersection( ColShapeA, ColShapeB, CollisionTest.mIntersectionA, CollisionTest.mIntersectionB, ObjA, ObjB ) )
		return;

	/*
	//	get the mid-collision point
	float TotalRadius = ColShapeA.mRadius + ColShapeB.mRadius;
	float TotalRadiusSq = TotalRadius*TotalRadius;
	TPointf DirectionAB = ColShapeB.mPosition;
	DirectionAB -= ColShapeA.mPosition;
	float DistanceSq = DirectionAB.GetLengthSq();
	if ( DistanceSq > TotalRadiusSq )
		return;
	*/
	CollisionTest.mHit = true;

	OnCollision( ObjA, CollisionTest.mIntersectionA );
	OnCollision( ObjB, CollisionTest.mIntersectionB );


	/*
	//	work out intersection point
	float DistanceWeightA = ColShapeA.mRadius / TotalRadius;
	float DistanceWeightB = 1.f - DistanceWeightA;
	CollisionTest.mHitPosition = ColShapeA.mPosition;
	CollisionTest.mHitPosition += DirectionAB * DistanceWeightA;
	float Overlap = -(DirectionAB.GetLength() - TotalRadius);
	assert( Overlap >= 0.f, "expected overlap to be positive" );

	TPointf DirectionABNormal = DirectionAB;
	DirectionABNormal.Normalise();

	//	move objects away from each other so they don't overlap
	TPointf MoveA = CollisionTest.mHitPosition - (DirectionABNormal*(Overlap));
	MoveA -= ColShapeA.mPosition;
	TPointf MoveB = CollisionTest.mHitPosition + (DirectionABNormal*(Overlap));
	MoveB -= ColShapeB.mPosition;

	ObjA.mPosition += MoveA;
	ObjB.mPosition += MoveB;
	/*
	//	basic amount to move objects away (this should NOT be velocity and instead a position change!)
	float BasicForce = 0.01f;

	//	push objects away from each other
	DirectionAB.Normalise();
	float PowerA = ObjA.mVelocity.GetLength();
	float PowerB = ObjB.mVelocity.GetLength();
	CollisionTest.mResponseForceA = DirectionAB * -(DistanceWeightA + PowerB);
	CollisionTest.mResponseForceB = DirectionAB * (DistanceWeightB + PowerA);

	//	apply to objects
	ObjA.mVelocity += CollisionTest.mResponseForceA;
	ObjB.mVelocity += CollisionTest.mResponseForceB;
	*/
}


void Update_Input()
{
	float InputForce = 0.6f;

	//	update input
	for ( int p=0;	p<gPlayers.GetSize();	p++ )
	{
		auto& Player = gPlayers[p];
		Player.mInput.Update();

		//	get direction vector
		TPointf InputDirection = Player.mInput.GetDirectionVector();
		InputDirection *= InputForce;
		
		//	apply input
		Player.mPlayerPhysics.mForce += InputDirection;
	}
}

void Update_PhysicsPreUpdate()
{
	for ( int p=0;	p<gPlayers.GetSize();	p++ )
	{
		auto& Player = gPlayers[p];

		//	update velocity
		Player.mPlayerPhysics.mVelocity += Player.mPlayerPhysics.mForce;
		Player.mPlayerPhysics.mForce = TPointf(0,0);

		//	try and position glove here relative to our direction
		//	spring distance of glove to desired length
		//	this is for movement, when rotating, and after our glove has been pushed out of place

		//	update velocity
		Player.mGlovePhysics.mVelocity += Player.mGlovePhysics.mForce;
		Player.mGlovePhysics.mForce = TPointf(0,0);
	}
}

void Update_Collisions(TFrameDebug& Debug)
{
	int CollisionIterationCount = 1;
	BufferArray<TCollisionTest,100> CollisionTests;

	//	generate collision tests
	//	(shorten this test list using the hardware collision - assuming a collision shape doesn't go outside)
	for ( int a=0;	a<gPlayers.GetSize();	a++ )
	{
		bool TestAgainstAllPlayers = true;
		//	TestAgainstAllPlayers = ( gPlayer[a].CollisionShapeGoesOutsideSprite );

		if ( TestAgainstAllPlayers )
		{
			for ( int b=a+1;	b<gPlayers.GetSize();	b++ )
			{
				CollisionTests.PushBack( TCollisionTest( gPlayers[a].mPlayerPhysics, gPlayers[b].mPlayerPhysics ) );
				//CollisionTests.PushBack( TCollisionTest( gPlayers[a].mGlovePhysics, gPlayers[b].mPlayerPhysics ) );
				//CollisionTests.PushBack( TCollisionTest( gPlayers[a].mPlayerPhysics, gPlayers[b].mGlovePhysics ) );
			}
		}
	}

	//	execute collision tests
	//	track which collision tests to re-execute for multiple iterations
	BufferArray<u16,100> IterateCollisionTests;

	for ( int c=0;	c<CollisionTests.GetSize();	c++ )
	{
		TCollisionTest& CollisionTest = CollisionTests[c];
		DoCollision( CollisionTest );
		if ( !CollisionTest.mHit )
			continue;
	
		//	re-iterate this collision
		IterateCollisionTests.PushBack( c );
	}

	//	note number of collisions
	auto& DebugString = Debug.PushBackString();
	DebugString << "Collision count: " << IterateCollisionTests.GetSize();
	/*
	//	multiple iterations
	for ( int Iteration=0;	!IterateCollisionTests.IsEmpty() && Iteration<CollisionIterationCount;	Iteration++ )
	{
		for ( int i=IterateCollisionTests.GetTailIndex();	i>=0;	i-- )
		{
			int CollisionTestIndex = IterateCollisionTests[i];
			TCollisionTest& CollisionTest = CollisionTests[CollisionTestIndex];
			DoCollision( CollisionTest );

			//	remove collision from iteration list if nothing happened
			
			//if ( !CollisionTest.mHit )
			//	IterateCollisionTests.RemoveAt( i );
		}
	}
	*/
}



void Update_PhysicsPostUpdate()
{
	float PlayerFriction = 0.3f;
	float GloveFriction = 0.6f;

	//	move sprites around
	for ( int p=0;	p<gPlayers.GetSize();	p++ )
	{
		auto& Player = gPlayers[p];
		
		Player.mPlayerPhysics.PostUpdate( PlayerFriction );
		Player.mGlovePhysics.PostUpdate( GloveFriction );
		
		//	update sprites
		Player.mPlayerSpriteInfo.mPosition.x = Player.GetPlayerPosition().x + Player.mPlayerSpriteOffset.x;
		Player.mPlayerSpriteInfo.mPosition.y = Player.GetPlayerPosition().y + Player.mPlayerSpriteOffset.y;
		Player.mGloveSpriteInfo.mPosition.x = Player.GetGlovePosition().x;
		Player.mGloveSpriteInfo.mPosition.y = Player.GetGlovePosition().y;
		gSpritePool.MoveSprite( Player.mPlayerSpriteRef, Player.mPlayerSpriteInfo.mPosition );
	//	gSpritePool.MoveSprite( Player.mGloveSpriteRef, Player.mGloveSpriteInfo.mPosition );
	}
}


void TGame::Update()
{
	TFrameDebug Debug;

	Update_Input();
	Update_PhysicsPreUpdate();
	Update_Collisions( Debug );
	Update_PhysicsPostUpdate();


	
	gSpritePool.BakeHardwareChanges( Debug );

	for ( int i=0;	i<Debug.GetMaxLineCount();	i++ )
	{
		//	clear line
		GD.putstr( 0, i, "                 " );
		if ( i < Debug.mStrings.GetSize() ) 
			GD.putstr( 0, i, Debug.mStrings[i] );
	}

}


