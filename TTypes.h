#pragma once

#include <SPI.h>
#include <GD.h>


namespace TGuts
{
	void	Assert(bool Condition,const char* Error,const char* Function);
};

#define assert(condition,ErrorString)	TGuts::Assert( condition, ErrorString, __FUNCTION__ )



#define limit(x,_min,_max)    max( _min, min( x, _max ) )
//#define null                  NULL

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef int8_t		s8;
typedef int16_t		s16;
typedef int32_t		s32;


namespace TGuts
{
	u16		GetStringLength(const char* String);
}

namespace TColour
{
	inline u16		GetColour5(const float& c)	{	return static_cast<u16>(c*31.f);	}
	inline u16		GetColour5(const u8& c)		{	return c >> 3;	}
};

class TColour16
{
public:
	TColour16() :
		mRgba	( GetRedBits<u8>(255)|GetGreenBits<u8>(255)|GetBlueBits<u8>(255)|GetAlphaBits(true) )
	{
	}
	TColour16(u8 r,u8 g,u8 b,bool a=true) :
		mRgba	( GetRedBits(r)|GetGreenBits(g)|GetBlueBits(b)|GetAlphaBits(a) )
	{
	}
		/*
	explicit TColour16(float r,float g,float b,bool a=true) :
		mRgba	( GetRedBits(r)|GetGreenBits(g)|GetBlueBits(b)|GetAlphaBits(a) )
	{
	}
	*/

	template<typename T> static u16		GetRedBits(T c)			{	return TColour::GetColour5(c) << 10;	}
	template<typename T> static u16		GetGreenBits(T c)		{	return TColour::GetColour5(c) << 5;	}
	template<typename T> static u16		GetBlueBits(T c)		{	return TColour::GetColour5(c) << 0;	}
	static u16							GetAlphaBits(bool a)	{	return static_cast<u16>(!a)<<15;	}

public:
	u16		mRgba;	//	bgra
};

//	dumb vector base
template<typename T>
class TVector2Base
{
protected:
	typedef T TYPE;

public:
	TVector2Base() :
		x	( 0 ),
		y	( 0 )
	{
	}
	TVector2Base(const T& xx,const T& yy) :
		x	( xx ),
		y	( yy )
	{
	}

public:
	T		x;
	T		y;
};

//	additonal trig funcs
template<typename T>
class TVector2Trig : public TVector2Base<T>
{
public:
	TVector2Trig()
	{
	}
	TVector2Trig(const T& xx,const T& yy) :
		TVector2Base<T>	( xx, yy )
	{
	}

public:
	T				DotProduct(const TVector2Trig& v) const		{	return (x*v.x) + (y*v.y);	}
	T				DotProduct() const							{	return (x*x) + (y*y);	}
	void			Normalise(float NormalLength=1.f)			{	T h = NormalLength/GetLength();	x*=h;	y*=h;	};			//	normalises vector
	//TVector2Trig	Normal(float NormalLength=1.f) const		{	return (*this) * (NormalLength/GetLength());	};	//	returns the normal of thsi vector
	T				GetLength() const							{	return sqrt( GetLengthSq() );	}
	T				GetLengthSq() const							{	return DotProduct();	}
	/*
	void	Normalise(T Length=1)
	{
		T s = GetLength();
		if ( s != 0 )
			s = 1 / s;

		x *= s;
		y *= s;
		
	}
	*/
};


template<typename T,class VECTORBASE=TVector2Base<T>>
class Type2 : public VECTORBASE
{
protected:
	//typedef typename VECTORBASE::TYPE T;
	typedef typename Type2<T,VECTORBASE> THIS;

public:
	Type2()
	{
	}
	Type2(const T& xx,const T& yy) :
		VECTORBASE	( xx, yy )
	{
	}

	template<class VECTYPE>	bool	operator==(const VECTYPE& Value) const	{	return (x==Value.x) && (y==Value.y);	}
	template<class VECTYPE>	bool	operator!=(const VECTYPE& Value) const	{	return (x!=Value.x) || (y!=Value.y);	}
	
	template<class VECTYPE>	THIS	operator+(const VECTYPE& Value) const	{	return THIS( x + Value.x, y + Value.y );	}
	template<class VECTYPE>	THIS	operator-(const VECTYPE& Value) const	{	return THIS( x - Value.x, y - Value.y );	}
	template<class VECTYPE>	THIS	operator*(const VECTYPE& Value) const	{	return THIS( x * Value.x, y * Value.y );	}
	template<class VECTYPE>	THIS	operator/(const VECTYPE& Value) const	{	return THIS( x / Value.x, y / Value.y );	}
	template<>				THIS	operator+(const T& Value) const			{	return THIS( x + Value, y + Value );	}
	template<>				THIS	operator-(const T& Value) const			{	return THIS( x - Value, y - Value );	}
	template<>				THIS	operator*(const T& Value) const			{	return THIS( x * Value, y * Value );	}
	template<>				THIS	operator/(const T& Value) const			{	return THIS( x / Value, y / Value );	}
	
	template<class VECTYPE>	void	operator+=(const VECTYPE& Value)	{	x += Value.x;	y += Value.y;	}
	template<class VECTYPE>	void	operator-=(const VECTYPE& Value)	{	x -= Value.x;	y -= Value.y;	}
	template<class VECTYPE>	void	operator*=(const VECTYPE& Value)	{	x *= Value.x;	y *= Value.y;	}
	template<class VECTYPE>	void	operator/=(const VECTYPE& Value)	{	x /= Value.x;	y /= Value.y;	}
	template<>				void	operator+=(const T& Value)			{	x += Value;		y += Value;	}
	template<>				void	operator-=(const T& Value)			{	x -= Value;		y -= Value;	}
	template<>				void	operator*=(const T& Value)			{	x *= Value;		y *= Value;	}
	template<>				void	operator/=(const T& Value)			{	x /= Value;		y /= Value;	}
};


typedef Type2<u16>	TPoint;
typedef Type2<float,TVector2Trig<float>>	TPointf;

template<typename T,u16 MAXSIZE,u16 BUFFERSIZE=MAXSIZE>
class BufferArray
{
public:
    BufferArray(u16 InitialSize=0) :
		mSize    ( 0 )
    {
		SetSize( InitialSize );
    }

    void		SetSize(int Size)	{	assert( Size <= MAXSIZE, "SetSize too large for buffer" );	mSize = Size;	}
    void		Clear()				{	SetSize(0);	}
	bool		IsEmpty() const		{	return GetSize() == 0;	}
    u16			GetSize() const		{	return mSize;	}
    u32			GetDataSize() const	{	return GetSize() * sizeof(T);	}
    u16			MaxSize() const		{	return MAXSIZE;	}

	const T*	GetData() const		{	assert( !IsEmpty(), "Data access in empty array" );	return &mData[0];	}
	const u8*	GetRawData() const	{	return reinterpret_cast<const u8*>( GetData() );	}
    T&			PushBack(const T& Item) 
    {
        T& Tail = Alloc();    
        Tail = Item;    
        return Tail;    
    }
    T&			PushBack()				{	return Alloc();	}
    T&			PushBackUnique(const T& Item)	
	{	
		T* pExisting = Find( Item );
		return pExisting ? *pExisting : PushBack( Item );
	}
    T&			GetTail()				{	return mData[ GetTailIndex() ];	}
    int			GetTailIndex() const	{	assert( !IsEmpty(), "Tail access in empty array" );	return GetSize()-1;	}

    void		PopBack(T& Item)     
    {    
        Item = GetTail();    
        mSize--;    
    }

    T&			operator[](u16 Index)		{	assert( Index < GetSize(), "Out of bounds" );	return mData[Index];	}
    const T&	operator[](u16 Index)const	{	assert( Index < GetSize(), "Out of bounds" );	return mData[Index];	}

    template<typename MATCH>
	int			FindIndex(const MATCH& Match) const
    {
        for ( int i=0;    i<mSize;    i++ )
        {
            if ( mData[i] == Match )
                return i;
        }
        return -1;
    }

	template<typename MATCH>
	const T*	Find(const MATCH& Match) const	
	{
		int Index = FindIndex( Match );
		return (Index == -1) ? NULL : &mData[Index];
    }

	template<typename MATCH>
	T*			Find(const MATCH& Match)
	{
		int Index = FindIndex( Match );
		return (Index == -1) ? NULL : &mData[Index];
    }

	void		SetAll(const T& Value)
	{
		for ( int i=0;	i<GetSize();	i++ )
			mData[i] = Value;
	}

protected:
    //    when we run out of space... we just overwrite the last item. yikes!
    T&			Alloc()
    {
		assert( GetSize() < MaxSize(), "Buffer array overflowed" );
        if ( GetSize() >= MaxSize() )
        {
            SetSize( MaxSize() );
            return GetTail();
        }
        return mData[mSize++];
    }

	void		SetBufferAll(const T& Value)
	{
		for ( int i=0;	i<BUFFERSIZE;	i++ )
			mData[i] = Value;
	}

protected:
    u16    mSize;
    T      mData[BUFFERSIZE];
};


//	really really basic string class for adding integers and has a terminator
template<u16 MAXSIZE>
class BufferString : public BufferArray<char,MAXSIZE,MAXSIZE+1>
{
public:
	BufferString()
	{
		SetBufferAll('\0');
	}
	BufferString(const char* String)
	{
		SetBufferAll('\0');
		(*this) << String;
	}

	void			SetLength(u16 StringLength)		
	{
		SetSize( StringLength+1 );	
		GetTail() = '\0';	
		SetSize( StringLength );	
	}

	BufferString& operator = (const char* String)
	{
		SetLength(0);
		(*this) << String;
		return *this;
	}
	operator		const char*() const		{	return GetData();	}
	BufferString&	operator<<(const char* String);
	BufferString&	operator<<(int Integer);
};

	
template<u16 MAXSIZE>
inline BufferString<MAXSIZE>& BufferString<MAXSIZE>::operator<<(const char* String)
{
	if ( !String || String[0]=='\0' )
		return *this;
	
	while ( *String )
	{
		PushBack( *String );
		String++;
	}

	return *this;
}


template<u16 MAXSIZE>
inline BufferString<MAXSIZE>& BufferString<MAXSIZE>::operator<<(int Integer)
{
	if ( Integer < 0 )
		PushBack('-');
	
	while ( true )
	{
		int Tenth = (Integer % 10);
		PushBack( '0' + Tenth );
		if ( Integer < 10 )
			break;
		Integer /= 10;
	}
	
	return *this;
}

